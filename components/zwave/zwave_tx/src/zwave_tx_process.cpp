
/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
// Includes from this component
#include "zwave_tx.h"
#include "zwave_tx_process.h"
#include "zwave_tx_process.hpp"
#include "zwave_tx_callbacks.h"
#include "zwave_tx_route_cache.h"
#include "zwave_tx_queue.hpp"
#include "zwave_tx_state_logging.h"
#include "zwave_tx_incoming_frames.hpp"

// Interfaces
#include "zwave_helper_macros.h"

// ZPC components
#include "zwave_tx_groups.h"
#include "zwave_controller_internal.h"
#include "zwave_controller_transport.h"
#include "zwave_network_management.h"
#include "ZW_classcmd.h"

#include "timer.hpp"

// ZPC components
#include "log.h"

// Standard includes
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <vector>
// Constants
static constexpr char LOG_TAG[] = "zwave_tx_process";
// Frames stuck in-flight longer than this are force-failed by the watchdog.
// Must exceed the zwave_api_transport emergency timer (65 s) so the watchdog
// only fires after the API-level recovery has already had a chance to run.
static constexpr clock_time_t TX_FRAME_IN_FLIGHT_WATCHDOG_MS = 90 * CLOCK_SECOND;

// Forward declaration for global instance
namespace zwave_component
{
    class zwave_tx_process;
}
// Global singleton instance for C API callbacks
// Declared here so it can be accessed from both C++ class methods and C wrapper functions
zwave_component::zwave_tx_process *zwave_tx_process_instance = nullptr;

// Our private variables.
namespace
{
    // Tx Queue state
    zwave_tx_state_t state;
    // back-off timer, used to decide when to send the next frame
    struct timer_handle_t backoff_timer {nullptr};
    // Private variable indicating if a Tx Queue flush is ongoing
    // Atomic to allow safe access from multiple threads
    std::atomic<bool> queue_flush_ongoing {false};
    // The frame currently being handled
    zwave_tx_session_id_t current_tx_session_id;
    // Map of NodeIDs and the number of frames that we expect from Nodes in our network.
    zwave_tx_incoming_frames expected_incoming_frames;
    // Reason for back-off (current_tx_session_id or we were told of extra frames)
    zwave_tx_backoff_reason_t backoff_reason;

    // LIFO of outgoing sessions whose BACKOFF_CURRENT_SESSION_ID was pre-empted
    // to send a back-off-bypass frame, so each can be re-armed when its bypass
    // chain completes. A stack (not a single slot) is required because pre-emption
    // can nest: a bypass frame that itself has number_of_responses > 0 enters its
    // own BACKOFF and can in turn be pre-empted by a further bypass frame. With a
    // single slot the outer session id is overwritten and never restored, which
    // leaves the outer session pinned at the queue head as an "IDLE waiting for
    // replies" wedge.
    std::vector<zwave_tx_session_id_t> preempted_backoff_stack;

    /**
     * @brief Returns true if the Z-Wave frame is an S2 transport-internal
     * frame (S2 NONCE_GET or S2 NONCE_REPORT).
     */
    bool is_s2_transport_internal_frame(const uint8_t *frame_data, uint16_t frame_length)
    {
        if ((frame_data == nullptr) || (frame_length < 2)) {
            return false;
        }
        if (frame_data[0] != COMMAND_CLASS_SECURITY_2) {
            return false;
        }
        return (frame_data[1] == SECURITY_2_NONCE_GET) || (frame_data[1] == SECURITY_2_NONCE_REPORT);
    }

    struct zwave_tx_frame_received_data_t {
            zwave_node_id_t node_id;
            uint16_t frame_length;
            uint8_t frame_data[ZWAVE_MAX_FRAME_SIZE];
    };

    void free_frame_received_event_data(void *data)
    {
        delete static_cast<zwave_tx_frame_received_data_t *>(data);
    }
}  // namespace

// The Z-Wave TX queue
zwave_tx_queue tx_queue;  // zwave_tx.cpp uses the tx_queue.

// Shared variable indicating if the ongoing session is for a protocol frame
// Atomic to allow safe access from multiple threads (process thread writes, callback threads read)
std::atomic<bool> is_protocol_frame {false};

// Forward declarations
static void zwave_tx_message_transmission_completed_step(zwave_tx_session_id_t session_id);

static void zwave_tx_process_backoff_timer_callback(void *ptr);

static void zwave_tx_restore_preempted_backoff_if_needed(zwave_tx_session_id_t completed_session_id);

/**
 * @brief Checks if the protocol is busy sending frames already
 *
 * @returns true if the protocol carries an operation that is sending or
 * expecting frames where we should back-off, false otherwise
 */
static bool zwave_tx_is_protocol_sending_frames()
{
    zwave_network_management_state_t zwave_network_management_state = zwave_network_management_get_state();
    if (NM_ASSIGNING_RETURN_ROUTE == zwave_network_management_state) {
        return true;
    }
    if (NM_NEIGHBOR_DISCOVERY_ONGOING == zwave_network_management_state) {
        return true;
    }

    return false;
}

/**
 * @brief Used if the current message should be dropped
 */
static void zwave_tx_drop_unsent_current_message()
{
    sl_log_warning(LOG_TAG, "Dropping frame (id=%p)\n", current_tx_session_id);
    // Ensure that it won't be requeued due to Fast Track option.
    tx_queue.disable_fasttack(current_tx_session_id);
    // Generate a callback for it, so that it will be cleaned up from the queue
    on_zwave_transport_send_data_complete(TRANSMIT_COMPLETE_FAIL, nullptr, current_tx_session_id);
}

/**
 * @brief Checks if we return to IDLE or stay in TRANSMISSION_ONGOING
 * when an element is completed. Removes the element from the queue.
 *
 * @param session_id The session ID to remove from the queue.
 */
static void zwave_tx_finalize_element(zwave_tx_session_id_t session_id)
{
    zwave_tx_queue_element_t completed_element = {};
    if (SL_STATUS_OK != tx_queue.get_by_id(&completed_element, session_id)) {
        sl_log_warning(LOG_TAG,
                       "Cannot find the following Session ID: %p, "
                       "it is already removed from the Tx Queue. "
                       "Nothing more will happen.\n",
                       session_id);
        state = ZWAVE_TX_STATE_IDLE;
        zwave_tx_process_check_queue();
        return;
    }

    // Stay in Transmission ongoing or go back to idle?
    if ((completed_element.options.transport.valid_parent_session_id) && (SL_STATUS_OK == tx_queue.get_by_id(&completed_element, completed_element.options.transport.parent_session_id))) {
        // Keep the transmission "ONGOING" if the frame has parents
        // that are still in the queue, set the parent as ongoing.
        state                 = ZWAVE_TX_STATE_TRANSMISSION_ONGOING;
        current_tx_session_id = completed_element.zwave_tx_session_id;
    } else {
        // Go back to idle if we are done with a "chain" of frames.
        state                 = ZWAVE_TX_STATE_IDLE;
        current_tx_session_id = nullptr;
    }
    // We are done (for ever!) with the element, so we delete it from the queue.
    sl_log_debug(LOG_TAG, "Removing id=%p from the queue", session_id);
    tx_queue.pop(session_id);

    // If the element we just popped was on the pre-empted-restore stack
    // (e.g. aborted, dropped, or fast-track failed), remove it so we do
    // not try to re-arm a back-off for a session that no longer exists.
    preempted_backoff_stack.erase(std::remove(preempted_backoff_stack.begin(), preempted_backoff_stack.end(), session_id), preempted_backoff_stack.end());

    // Did we just reach an empty queue while waiting for a queue flush?
    if (zwave_tx_process_queue_flush_is_ongoing() && tx_queue.empty()) {
        sl_log_info(LOG_TAG, "Reset step: Tx Queue flush completed");
        zwave_controller_on_reset_step_complete(ZWAVE_CONTROLLER_TX_FLUSH_RESET_STEP_PRIORITY);
    }
}

/**
 * @brief Initiates a Backoff for a given duration.
 *
 * @param backoff_time  The time to back-off until we get all the responses.
 * @param reason        Why are we backing-off
 */
static void zwave_tx_process_initiate_backoff(clock_time_t backoff_time, zwave_tx_backoff_reason_t reason)
{
    sl_log_debug(LOG_TAG, "Starting Z-Wave TX back off for %lu ms. Reason: %s\n", backoff_time, zwave_back_off_reason_name(reason));
    state          = ZWAVE_TX_STATE_BACKOFF;
    backoff_reason = reason;
    timer_stop(&backoff_timer);
    timer_set(&backoff_timer, backoff_time, zwave_tx_process_backoff_timer_callback, 0);
}

/**
 * @brief Resumes from back-off state.
 *
 */
static void zwave_tx_resume_from_backoff_step()
{
    // Ensure that the backoff timer is not running anymore
    timer_stop(&backoff_timer);

    // Snapshot for the pre-empted-session re-scheduling
    const zwave_tx_session_id_t resumed_session_id = current_tx_session_id;

    if (state == ZWAVE_TX_STATE_BACKOFF) {
        if (backoff_reason == BACKOFF_CURRENT_SESSION_ID) {
            // Remove from the queue and make the correct state transision.
            zwave_tx_finalize_element(current_tx_session_id);
        } else if (backoff_reason == BACKOFF_EXPECTED_ADDITIONAL_FRAMES) {
            // Give up on waiting, for any additional frame.
            expected_incoming_frames.clear();
            state = ZWAVE_TX_STATE_IDLE;
        } else if ((backoff_reason == BACKOFF_PROTOCOL_SENDING_FRAMES) || (backoff_reason == BACKOFF_INCOMING_UNSOLICITED_ROUTED_FRAME)) {
            // Move back to IDLE
            state = ZWAVE_TX_STATE_IDLE;
        }
    }
    // Reset the back-off reason:
    backoff_reason = BACKOFF_CURRENT_SESSION_ID;

    // Handles the case where the back-off- bypass frame that pre-empted
    // BACKOFF_CURRENT_SESSION_ID armed its own BACKOFF (i.e. a bypass frame
    // with `number_of_responses > 0` in the future) and that back-off has
    // just resolved.
    zwave_tx_restore_preempted_backoff_if_needed(resumed_session_id);

    // Check if there is more work to do
    zwave_tx_process_check_queue();
}

static void zwave_tx_process_backoff_timer_callback(void *ptr)
{
    sl_log_debug(LOG_TAG, "Tx backoff timer expired. Posting event to resume from back-off.\n");
    // Post event to the zwave_tx_process thread instead of directly modifying
    // shared state. Timer callbacks execute in the timer manager's worker thread,
    // so we must use the event queue to safely access shared state.
    if (zwave_tx_process_instance != nullptr) {
        zwave_tx_process_instance->post_event(ZWAVE_TX_BACKOFF_TIMER_EXPIRED, nullptr);
    }
}

/**
 * @brief Verifies if a frame is fully processed or needs
 * to be discarded.
 *
 * The following checks are performed:
 * 1. If frame transmission not initiated, has the discard timer expired ?
 * 2. Has frame transmission been initiated and we received a callback
 *    but it's still on top of the queue?
 * 3. Has frame transmission been initiated and we are waiting for a callback ?
 *
 * @returns true  if the frame is still to be sent.
 * @returns false if the frame has been processed and
 *                can be removed from the queue.
 */
static bool is_frame_to_be_sent(const zwave_tx_queue_element_t &e)
{
    // Verify if the frame has expired and was not sent yet.
    if ((e.options.discard_timeout_ms > 0) && (e.transmission_timestamp == 0) && ((e.queue_timestamp + e.options.discard_timeout_ms) < clock_time())) {
        sl_log_debug(LOG_TAG, "Frame discard timeout has expired.\n");
        zwave_tx_drop_unsent_current_message();
        return false;
    }

    // Has the frame been fully sent ?
    // Happens after a send_data callback
    if (e.transmission_time != 0) {
        if (tx_queue.transport_completion_step_is_pending(e.zwave_tx_session_id)) {
            zwave_tx_message_transmission_completed_step(e.zwave_tx_session_id);
        }
        return false;
    }

    // Has the frame been pushed to the Z-Wave transport or
    // Z-Wave API and we are waiting for a callback?
    if (e.transmission_timestamp != 0) {
        // Just wait, back to idle, and trust the transport to make its callback !!
        sl_log_debug(LOG_TAG,
                     "Frame (id=%p) waits for new elements or transport callback..."
                     "Tx Queue will not do anything until then.",
                     current_tx_session_id);
        const clock_time_t in_flight_ms = clock_time() - e.transmission_timestamp;
        if (in_flight_ms > 5 * CLOCK_SECOND) {
            // NOTE: state may still be IDLE here when is_frame_to_be_sent() is
            // called from the queue-scan path, before state is promoted to
            // TRANSMISSION_ONGOING.  This is expected and not an error.
            sl_log_debug(LOG_TAG, "TX in-flight wait: id=%p in_flight_ms=%lu state=%s tid=%lu", current_tx_session_id, (unsigned long)in_flight_ms, zwave_tx_state_name(state), sl_log_thread_id());
        }
        if (in_flight_ms > TX_FRAME_IN_FLIGHT_WATCHDOG_MS) {
            sl_log_debug(LOG_TAG,
                         "TX in-flight expired: id=%p in_flight_ms=%lu — "
                         "transport callback lost, force-failing frame",
                         current_tx_session_id,
                         (unsigned long)in_flight_ms);
            // Tell the transport layer to abort first so it stops tracking the
            // session and does not leave transmission_ongoing set.
            zwave_controller_transport_abort_send_data(current_tx_session_id);
            // Clear is_protocol_frame before the synthetic callback so that
            // on_zwave_transport_send_data_complete does not misinterpret
            // current_tx_session_id as a protocol_metadata_t pointer.
            is_protocol_frame.store(false);
            // zwave_tx_drop_unsent_current_message() is the right call here
            // even though the frame was already sent (the name is misleading).
            // The helper synthesises an on_zwave_transport_send_data_complete
            // callback with TRANSMIT_COMPLETE_FAIL, which posts
            // ZWAVE_TX_SEND_OPERATION_COMPLETE and properly finalises the
            // in-flight element regardless of whether state is still IDLE or
            // has already been promoted to TRANSMISSION_ONGOING.  The
            // set_transmissions_results path is idempotent for a missing
            // element, so the still-IDLE case is benign.
            zwave_tx_drop_unsent_current_message();
        }
        return false;
    }

    // Frame seems to be ready for transmission.
    return true;
}

// Frame finished and completion already ran; entry only waits for reply / back-off handling.
static bool idle_head_only_waiting_for_replies(const zwave_tx_queue_element_t &h)
{
    return (h.transmission_time != 0) && (!tx_queue.transport_completion_step_is_pending(h.zwave_tx_session_id));
}

// Handed to transport; completion not recorded in the queue element yet.
static bool idle_head_waiting_for_chip_completion(const zwave_tx_queue_element_t &h)
{
    return (h.transmission_timestamp != 0) && (h.transmission_time == 0);
}

static sl_status_t zwave_tx_process_fetch_next_element_for_ongoing_transmission(zwave_tx_queue_element_t *next_element)
{
    // Prefer the highest priority child of the current session. This is the
    // normal case: transport sub-frames (S2/S0/Supervision encapsulation)
    // attached as children via `options.transport.parent_session_id`.
    if (tx_queue.get_highest_priority_child(next_element, current_tx_session_id) == SL_STATUS_OK) {
        return SL_STATUS_OK;
    }

    // No child, look at the current session itself.
    zwave_tx_queue_element_t current = {};
    const sl_status_t current_status = tx_queue.get_by_id(&current, current_tx_session_id);
    if (current_status != SL_STATUS_OK) {
        // Current element does not exist?? Go back to IDLE.
        sl_log_warning(LOG_TAG, "current_tx_session_id (id=%p) is not in the Tx Queue while state is %s. Returning to IDLE.", static_cast<const void *>(current_tx_session_id), zwave_tx_state_name(state));
        state = ZWAVE_TX_STATE_IDLE;
        zwave_tx_process_check_queue();
        return SL_STATUS_NOT_FOUND;
    }

    // Current session has not been handed to a transport yet, send it now.
    if (current.transmission_timestamp == 0) {
        *next_element = current;
        return SL_STATUS_OK;
    }

    // Current session is in-flight waiting for a transport callback: if
    // the controller is idle, let a standalone back-off-bypass frame overtake
    // it so an urgent nonce re-sync does not miss its discard_timeout_ms.
    zwave_tx_queue_element_t *candidate = tx_queue.first_in_queue();
    if ((candidate != nullptr) && (candidate->zwave_tx_session_id != current_tx_session_id) && (candidate->transmission_timestamp == 0) && (candidate->options.transport.ignore_incoming_frames_back_off) && (!candidate->options.transport.valid_parent_session_id)
        && (!zwave_controller_transport_is_busy())) {
        *next_element = *candidate;
        return SL_STATUS_OK;
    }

    // No better candidate, stay with the current session.
    *next_element = current;
    return SL_STATUS_OK;
}

/**
 * @brief Checks the next element in the queue and tells us if a pending
 * back-off can be pre-empted to send this frame.
 *
 * @return true       if we should send the frame even with back-off pending
 * @return false      if we should keep backing-off
 */
static bool zwave_tx_can_ignore_incoming_frames()
{
    if (tx_queue.empty()) {
        return false;
    }

    zwave_tx_queue_element_t next_element = {};
    // Are we already sending? then try to see if we have children frame to send
    if (state == ZWAVE_TX_STATE_TRANSMISSION_ONGOING) {
        // Take the child with highest priority, else the current element if no child.
        if (SL_STATUS_OK != zwave_tx_process_fetch_next_element_for_ongoing_transmission(&next_element)) {
            return false;
        }
    } else if (state == ZWAVE_TX_STATE_IDLE) {
        // We are idle, take the highest priority element from the queue.
        next_element = *tx_queue.first_in_queue();
    } else if (state == ZWAVE_TX_STATE_BACKOFF) {
        // We are in back-off. Only look at the queue head and allow pre-empting
        // the back-off if it is a back-off-bypass frame.
        const zwave_tx_queue_element_t *candidate = tx_queue.first_in_queue();
        if ((candidate == nullptr) || (candidate->zwave_tx_session_id == current_tx_session_id) || (candidate->options.transport.valid_parent_session_id) || (zwave_controller_transport_is_busy())) {
            return false;
        }
        next_element = *candidate;
    }

    // Bypass if the frame has not been sent yet, and it is explicitly allowed
    // to bypass the back-off.
    return (next_element.options.transport.ignore_incoming_frames_back_off && (next_element.transmission_timestamp == 0));
}

/**
 * @brief Inspect the highest priority element in the queue and initiate sending.
 */
static void zwave_tx_process_send_next_message_step()
{
    if (tx_queue.empty()) {
        return;
    }

    zwave_tx_queue_element_t current_element = {};

    const zwave_tx_session_id_t previous_tx_session_id = current_tx_session_id;
    const zwave_tx_state_t previous_state              = state;

    // Are we already sending? then try to see if we have children frame to send
    if (state == ZWAVE_TX_STATE_TRANSMISSION_ONGOING) {
        // Take the child with highest priority, else the current element if no child.
        if (SL_STATUS_OK != zwave_tx_process_fetch_next_element_for_ongoing_transmission(&current_element)) {
            return;
        }
    } else if (state == ZWAVE_TX_STATE_IDLE) {
        // We are idle, take the highest priority element from the queue.
        // Do not set TRANSMISSION_ONGOING until we know is_frame_to_be_sent will
        // actually dispatch.
        current_element                    = *tx_queue.first_in_queue();
        zwave_tx_queue_element_t alternate = {};
        if (idle_head_only_waiting_for_replies(current_element)) {
            if (tx_queue.find_best_unsent_by_qos(&alternate) == SL_STATUS_OK) {
                sl_log_debug(LOG_TAG, "IDLE: queue head id=%p is waiting for replies; dispatching best unsent id=%p instead.", current_element.zwave_tx_session_id, alternate.zwave_tx_session_id);
                current_element = alternate;
            }
        } else if (idle_head_waiting_for_chip_completion(current_element) && (!zwave_controller_transport_is_busy())) {
            if (tx_queue.find_best_unsent_backoff_bypass(&alternate) == SL_STATUS_OK) {
                sl_log_debug(LOG_TAG, "IDLE: queue head id=%p is waiting for a transport callback; dispatching back-off-bypass frame id=%p instead.", current_element.zwave_tx_session_id, alternate.zwave_tx_session_id);
                current_element = alternate;
            }
        }
    } else {
        // Other tx queue states should not try to call this function!
        sl_log_warning(LOG_TAG,
                       "Trying to send next element while Tx Queue state = %s. "
                       "This should not happen. Ignoring.",
                       zwave_tx_state_name(state));
        return;
    }
    // Save the current Session ID.
    current_tx_session_id = current_element.zwave_tx_session_id;

    if (!is_frame_to_be_sent(current_element)) {
        return;
    }

    if (previous_state == ZWAVE_TX_STATE_IDLE) {
        state = ZWAVE_TX_STATE_TRANSMISSION_ONGOING;
    }

    void *user = current_tx_session_id;
    is_protocol_frame.store(false);
    if (current_element.options.transport.is_protocol_frame) {
        // `user` argument in `zwave_controller_transport_send_data` is used to transport the current TX session ID
        // which is not ideal so we need to transport the real user data in the protocol metadata structure to match
        // the queue element when `on_zwave_transport_send_data_complete` is called.
        protocol_metadata_t *protocol_metadata = (protocol_metadata_t *)current_element.user;
        protocol_metadata->tx_session_id       = current_tx_session_id;
        user                                   = current_element.user;
        is_protocol_frame.store(true);
    }

    tx_queue.set_transmission_timestamp(current_tx_session_id);
    sl_log_debug(LOG_TAG, "TX dispatch: id=%p node=%d ts=%lu state=%s tid=%lu", current_tx_session_id, (unsigned int)current_element.connection_info.remote.node_id, (unsigned long)clock_time(), zwave_tx_state_name(state), sl_log_thread_id());

    // Ask the transports to take the frame.
    sl_status_t transport_status = zwave_controller_transport_send_data(&(current_element.connection_info), current_element.data_length, current_element.data, &(current_element.options), &on_zwave_transport_send_data_complete, user, current_tx_session_id);

    if (transport_status == SL_STATUS_OK) {
        // Cancel any retry timer armed on a prior SL_STATUS_BUSY defer (preempted
        // safety timer or non-preempted poll). If the frame goes out before the
        // timer fires, leaving it armed would run zwave_tx_resume_from_backoff_step
        // and corrupt backoff_reason for an unrelated session.
        timer_stop(&backoff_timer);
        return;
    }
    if (transport_status == SL_STATUS_BUSY) {
        // Transport is still processing a previous frame. Keep the candidate
        // in the queue and roll back to the pre-dispatch state; the transport's
        // completion callback will re-enter check_queue() and retry this frame.
        sl_log_debug(LOG_TAG, "Transport busy. Deferring frame (id=%p); will retry when the transport's completion callback completes.", current_tx_session_id);
        tx_queue.reset_transmission_timestamp(current_tx_session_id);
        current_tx_session_id = previous_tx_session_id;
        state                 = previous_state;
        is_protocol_frame.store(false);
        if (!preempted_backoff_stack.empty()) {
            // If the transport stays busy indefinitely (e.g. the S2 FSM gets stuck),
            // the normal ZWAVE_TX_SEND_OPERATION_COMPLETE recovery path never fires and
            // the preempted session is abandoned. Arm a safety timer so zwave_tx_resume_from_backoff_step
            // restores the preempted backoff and retries the bypass frame regardless.
            sl_log_debug(LOG_TAG, "Bypass frame deferred with preempted session pending (id=%p). Arming safety retry timer.", preempted_backoff_stack.back());
            timer_set(&backoff_timer, 3 * CLOCK_SECOND, zwave_tx_process_backoff_timer_callback, 0);
        } else {
            // No transport completion will arrive for this defer; poll periodically so
            // the queue head is retried when S2 inclusion or an app session clears.
            timer_set(&backoff_timer, CLOCK_SECOND / 2, zwave_tx_process_backoff_timer_callback, 0);
        }
        return;
    }  // Persistent error from the transport layer. Drop the frame to avoid a stall.
    sl_log_warning(LOG_TAG, "Transport cannot handle new frame (id=%p). Discarding to avoid a stall.", current_tx_session_id);
    zwave_tx_drop_unsent_current_message();
}

/**
 * @brief Re-schedule BACKOFF_CURRENT_SESSION_ID for the outgoing session whose
 * back-off was pre-empted to send a back-off-bypass frame.
 *
 * @param completed_session_id The session whose completion just ran.
 */
static void zwave_tx_restore_preempted_backoff_if_needed(zwave_tx_session_id_t completed_session_id)
{
    if (preempted_backoff_stack.empty()) {
        return;
    }

    if (zwave_tx_process_queue_flush_is_ongoing()) {
        preempted_backoff_stack.clear();
        return;
    }

    // Unwind the bypass chain in LIFO order. Stale entries (no longer in the
    // queue, or with responses=0/transmission_time=0) are discarded and we keep
    // popping; otherwise an outer pre-emption would be silently abandoned and
    // reproduce the very wedge this stack is meant to prevent. We stop as soon
    // as one entry re-arms a back-off (only one BACKOFF_CURRENT_SESSION_ID can
    // be active at a time; remaining outer entries resume when this one ends).
    while (!preempted_backoff_stack.empty()) {
        const zwave_tx_session_id_t preempted_id = preempted_backoff_stack.back();
        preempted_backoff_stack.pop_back();

        zwave_tx_queue_element_t preempted = {};
        if (tx_queue.get_by_id(&preempted, preempted_id) != SL_STATUS_OK) {
            continue;
        }

        if ((preempted.options.number_of_responses == 0) || (preempted.transmission_time == 0)) {
            sl_log_debug(LOG_TAG, "Skipping re-scheduling BACKOFF_CURRENT_SESSION_ID for pre-empted session id=%p (number_of_responses=%u, transmission_time=%lu); finalizing to avoid a queue stall.", preempted_id, preempted.options.number_of_responses, preempted.transmission_time);
            zwave_tx_finalize_element(preempted_id);
            continue;
        }

        const clock_time_t backoff_time = preempted.options.number_of_responses * (preempted.transmission_time + CLOCK_SECOND);

        current_tx_session_id = preempted_id;
        sl_log_debug(LOG_TAG, "Re-scheduling BACKOFF_CURRENT_SESSION_ID for pre-empted session id=%p after back-off-bypass frame id=%p completed.", current_tx_session_id, completed_session_id);
        zwave_tx_process_initiate_backoff(backoff_time, BACKOFF_CURRENT_SESSION_ID);
        return;
    }
}

/**
 * @brief Looks at transmissions results and decides what to do next
 *
 * If the transmission was successful, 2 choices:
 * 1. If we expect some replies: Apply a backoff before the next transmission
 * 2. If we do not expect a reply: Remove the element from the queue and move on to the next one.
 *
 * If the transmission was not successful:
 * 1. If it was a fastrack qos: requeue without fasttrack
 * 2. Else just give up for this element, remove it from the queue.
 */
static void zwave_tx_message_transmission_completed_step(zwave_tx_session_id_t session_id)
{
    zwave_tx_queue_element_t completed_element = {};
    if (SL_STATUS_OK != tx_queue.get_by_id(&completed_element, session_id)) {
        // Item was already removed from the queue.(e.g due to processing the next frame)
        state = ZWAVE_TX_STATE_IDLE;
        zwave_tx_process_check_queue();
        return;
    }

    // This function can run twice for the same completion: once from the
    // normal completion event, and once from is_frame_to_be_sent. The flag
    // set in set_transmissions_results() makes sure we only run once.
    if (!tx_queue.consume_transport_completion_step_pending(session_id)) {
        sl_log_debug(LOG_TAG, "Redundant transmission completion event for id=%p (completion step already ran).", session_id);
        return;
    }

    // Did the element fail and need to be requeued ? (fasttrack)
    if ((completed_element.send_data_status != TRANSMIT_COMPLETE_OK) && (completed_element.send_data_status != TRANSMIT_COMPLETE_VERIFIED) && (completed_element.options.fasttrack) && (!zwave_tx_process_queue_flush_is_ongoing())) {
        sl_log_debug(LOG_TAG,
                     "Fastrack transmit attempt failed (status = %d). "
                     "Requeueing element %p\n",
                     completed_element.send_data_status,
                     completed_element.zwave_tx_session_id);
        tx_queue.disable_fasttack(completed_element.zwave_tx_session_id);
        // Reset the transmission timestamp, so that it gets discarded
        // if has spent too long in the queue from enqueuing to 2nd transmit attempt.
        tx_queue.reset_transmission_timestamp(completed_element.zwave_tx_session_id);
        zwave_tx_process_check_queue();
        return;
    }

    // Call the callback function, if there is one registered for the element
    if (completed_element.callback_function != nullptr) {
        completed_element.callback_function(completed_element.send_data_status, &(completed_element.send_data_tx_status), completed_element.user);
    }

    // Apply backoff if it was successful singlecast and we expect replies.
    if (IS_TRANSMISSION_SUCCESSFUL(completed_element.send_data_status) && completed_element.options.number_of_responses > 0 && !completed_element.connection_info.remote.is_multicast && (!zwave_tx_process_queue_flush_is_ongoing())
        && completed_element.connection_info.remote.node_id != ZWAVE_BROADCAST_NODE_ID && completed_element.connection_info.remote.node_id != ZWAVE_LR_BROADCAST_NODE_ID) {
        // The backoff time should be transmission_time + 1 second.
        // Details are available in the Role Type specification, section "Node interview and response timeouts"
        clock_time_t backoff_time = completed_element.options.number_of_responses * (completed_element.transmission_time + CLOCK_SECOND);
        current_tx_session_id     = session_id;
        sl_log_debug(LOG_TAG, "TX backoff start: id=%p node=%d backoff_ms=%lu responses=%d tx_time_ms=%lu", session_id, completed_element.connection_info.remote.node_id, backoff_time, completed_element.options.number_of_responses, completed_element.transmission_time);
        zwave_tx_process_initiate_backoff(backoff_time, BACKOFF_CURRENT_SESSION_ID);
        // Re-run the queue inspection now that we just entered BACKOFF.
        zwave_tx_process_check_queue();
        return;
    }

    // Remove from the queue and make the correct state transision.
    zwave_tx_finalize_element(completed_element.zwave_tx_session_id);

    // If this completion was the back-off-bypass frame that pre-empted an
    // in-flight BACKOFF_CURRENT_SESSION_ID, re-schedule the back-off for the
    // original session so that its pending reply is still handled.
    zwave_tx_restore_preempted_backoff_if_needed(completed_element.zwave_tx_session_id);

    // Check if there is more work to do
    zwave_tx_process_check_queue();
}

////////////////////////////////////////////////////////////////////////////////
// Functions shared among the component
////////////////////////////////////////////////////////////////////////////////
bool zwave_tx_process_queue_flush_is_ongoing()
{
    return queue_flush_ongoing.load();
}

void zwave_tx_process_open_tx_queue()
{
    if (queue_flush_ongoing.load()) {
        sl_log_info(LOG_TAG, "Re-opening the Z-Wave Tx Queue for new frames.");
        queue_flush_ongoing.store(false);
    }
}

static void zwave_tx_process_inspect_received_frame_step(zwave_node_id_t node_id, const uint8_t *frame_data, uint16_t frame_length)
{
    // Are we waiting for "spontaneous frames" ?
    if (backoff_reason == BACKOFF_EXPECTED_ADDITIONAL_FRAMES) {
        if (expected_incoming_frames.get_frames(node_id) > 0) {
            expected_incoming_frames.decrement_frames(node_id);

            if (expected_incoming_frames.empty()) {
                // We got the last frame we were waiting for.
                sl_log_debug(LOG_TAG, "Received all expected additional frames.");
                zwave_tx_resume_from_backoff_step();
            }
        }
        return;
    }

    // If we are idle and received an unsolicited routed frame, initiate a back-off
    // If we were backing off due to unsolicited routed frame, restart the back-off
    if ((state == ZWAVE_TX_STATE_IDLE) || ((state == ZWAVE_TX_STATE_BACKOFF) && (backoff_reason == BACKOFF_INCOMING_UNSOLICITED_ROUTED_FRAME))) {
        uint8_t repeaters = zwave_tx_route_cache_get_number_of_repeaters(node_id);
        if (repeaters > 0) {
            // Extra back-off will be 10ms + (number of links in the route) * 10ms.
            const clock_time_t backoff_time = 10 * (1 + (repeaters + 1) * 2);
            zwave_tx_process_initiate_backoff(backoff_time, BACKOFF_INCOMING_UNSOLICITED_ROUTED_FRAME);
        }
        return;
    }

    // Reply handling for the current outgoing session.
    zwave_tx_queue_element_t current_element;
    if (SL_STATUS_OK != tx_queue.get_by_id(&current_element, current_tx_session_id)) {
        return;
    }

    // S2 nonce frames (NONCE_GET / NONCE_REPORT) are part of the S2 handshake,
    // not application replies, so they must not decrement number_of_responses.
    // Exception: if the outgoing session is itself a nonce frame, the incoming
    // nonce frame may be its reply and is counted normally.
    if (is_s2_transport_internal_frame(frame_data, frame_length) && !is_s2_transport_internal_frame(current_element.data, current_element.data_length)) {
        return;
    }

    if ((backoff_reason == BACKOFF_CURRENT_SESSION_ID) && (node_id == current_element.connection_info.remote.node_id) && (current_element.options.number_of_responses > 0)) {
        // Is that the last reply we were waiting for?
        if (current_element.options.number_of_responses == 1) {
            sl_log_debug(LOG_TAG,
                         "Received all expected replies from NodeID "
                         "%d for frame id=%p.\n",
                         node_id,
                         current_element.zwave_tx_session_id);
            zwave_tx_resume_from_backoff_step();
        }

        // Decrement the number of expected responses and wait for the next one.
        tx_queue.decrement_expected_responses(current_tx_session_id);
    }
}

void zwave_tx_process_inspect_received_frame(zwave_node_id_t node_id, const uint8_t *frame_data, uint16_t frame_length)
{
    auto payload = std::unique_ptr<zwave_tx_frame_received_data_t>(new (std::nothrow) zwave_tx_frame_received_data_t());
    if (payload == nullptr) {
        sl_log_error(LOG_TAG, "Failed to allocate frame received event payload for NodeID %u.", node_id);
        return;
    }

    payload->node_id      = node_id;
    payload->frame_length = frame_length;
    if ((frame_data != nullptr) && (frame_length > 0)) {
        frame_length = std::min<int>(frame_length, ZWAVE_MAX_FRAME_SIZE);
        memcpy(payload->frame_data, frame_data, frame_length);
        payload->frame_length = frame_length;
    }

    if (zwave_tx_process_instance == nullptr) {
        sl_log_error(LOG_TAG, "TX process not running; dropping frame received event for NodeID %u.", node_id);
        return;
    }

    zwave_tx_process_instance->post_event(ZWAVE_TX_FRAME_RECEIVED, payload.release());
}

sl_status_t zwave_tx_process_abort_transmission(zwave_tx_session_id_t session_id)
{
    // Then check if it in the queue
    if (!tx_queue.contains(session_id)) {
        return SL_STATUS_NOT_FOUND;
    }

    // Ensure to remove fasttrack requeueing.
    tx_queue.disable_fasttack(session_id);

    // Ensure we are not stuck in a back-off for that frame
    if ((current_tx_session_id == session_id) && (state == ZWAVE_TX_STATE_BACKOFF) && (backoff_reason == BACKOFF_CURRENT_SESSION_ID)) {
        zwave_tx_resume_from_backoff_step();
        return SL_STATUS_OK;
    }

    // If we are idle at that point, just remove the element from the queue.
    // Pretending it failed
    if (state == ZWAVE_TX_STATE_IDLE) {
        current_tx_session_id = session_id;
        zwave_tx_drop_unsent_current_message();
        return SL_STATUS_OK;
    }

    // If we are transmitting, abort by asking the transports to abort.
    sl_status_t abort_status = zwave_controller_transport_abort_send_data(session_id);

    if (abort_status != SL_STATUS_OK) {
        // If the transport cannot abort, or are taking care of another frame,
        // mark it as sent/failed, so it just gets removed from the queue when
        // we get to look at the element.
        abort_status = tx_queue.set_transmissions_results(session_id, TRANSMIT_COMPLETE_FAIL, nullptr);
        zwave_tx_process_check_queue();
    }
    return abort_status;
}

/**
 * @brief Aborts all queued singlecast transmissions targeting `node_id`.
 */
static void zwave_tx_process_abort_transmissions_for_node(zwave_node_id_t node_id)
{
    std::vector<zwave_tx_session_id_t> sessions_to_abort;
    tx_queue.collect_session_ids_for_node(node_id, sessions_to_abort);

    if (sessions_to_abort.empty()) {
        return;
    }

    sl_log_info(LOG_TAG, "NodeID %u removed; aborting %zu queued transmission(s) targeting it.", node_id, sessions_to_abort.size());

    for (zwave_tx_session_id_t session_id: sessions_to_abort) {
        const sl_status_t status = zwave_tx_process_abort_transmission(session_id);
        if ((status != SL_STATUS_OK) && (status != SL_STATUS_NOT_FOUND)) {
            sl_log_debug(LOG_TAG, "Abort of transmission id=%p for removed NodeID %u returned status 0x%04x.", session_id, node_id, status);
        }
    }
}

void zwave_tx_process_on_node_deleted(zwave_node_id_t node_id)
{
    // Foreign-network exclusion
    if (node_id == 0) {
        return;
    }
    zwave_tx_process_post_event(ZWAVE_TX_NODE_DELETED, reinterpret_cast<void *>(static_cast<uintptr_t>(node_id)));
}

sl_status_t zwave_tx_process_flush_queue_reset_step()
{
    sl_log_info(LOG_TAG, "Reset step: Initiating flush of the Tx Queue");

    // Mark that we want to reach an empty queue.
    queue_flush_ongoing.store(true);
    sl_log_info(LOG_TAG, "Tx Queue will no longer accept new frames.");

    // Is it already empty?
    if (tx_queue.empty()) {
        // Tell the controller not to wait for any callback.
        sl_log_info(LOG_TAG, "Reset step: Tx Queue flush completed. (was empty)");
        return SL_STATUS_ALREADY_INITIALIZED;
    }

    // Shortcut any active back-off so remaining sessions can be aborted below.
    // Do NOT return early: other sessions may still be in the queue.
    if (state == ZWAVE_TX_STATE_BACKOFF) {
        zwave_tx_resume_from_backoff_step();
        // If this was the last session, zwave_tx_finalize_element already called
        // zwave_controller_on_reset_step_complete; sessions_to_abort will be
        // empty and we fall through to return SL_STATUS_OK safely.
    }

    // If transmitting, tell the transport to abort (best-effort), then
    // unconditionally force-drop the current frame. queue_flush_ongoing blocks
    // any child frames the transport needs to complete its own abort (e.g. S0
    // nonce frames), so we cannot rely on the transport's async callback.
    if (state == ZWAVE_TX_STATE_TRANSMISSION_ONGOING) {
        zwave_controller_transport_abort_send_data(current_tx_session_id);
        zwave_tx_drop_unsent_current_message();
    }

    // Force-abort every session still in the queue. Sessions that depend on
    // child frames (e.g. S0 nonce traffic) will never make progress on their
    // own once queue_flush_ongoing is set.
    std::vector<zwave_tx_session_id_t> sessions_to_abort;
    tx_queue.collect_all_session_ids(sessions_to_abort);
    for (zwave_tx_session_id_t session_id: sessions_to_abort) {
        zwave_tx_process_abort_transmission(session_id);
    }

    return SL_STATUS_OK;
}

void zwave_tx_process_check_queue()
{
    // Set when we break out of BACKOFF_CURRENT_SESSION_ID to send a bypass
    // frame. Used below to skip the PROTOCOL_SENDING_FRAMES branch, which
    // would otherwise start a new back-off on top of the pre-emption and
    // leave the pre-empted session stuck in the queue.
    bool just_preempted_current_session = false;

    // Check if we want to break a back-off:
    if ((ZWAVE_TX_STATE_BACKOFF == state) && (BACKOFF_EXPECTED_ADDITIONAL_FRAMES == backoff_reason) && (zwave_tx_can_ignore_incoming_frames())) {
        // Stop the timer so it cannot fire later and corrupt backoff_reason for
        // an unrelated session that starts a new BACKOFF_CURRENT_SESSION_ID back-off.
        timer_stop(&backoff_timer);
        sl_log_debug(LOG_TAG, "Breaking Tx back-off as the next frame allows it.");
        state = ZWAVE_TX_STATE_IDLE;
    }
    // Pre-empt BACKOFF_CURRENT_SESSION_ID to send a back-off-bypass frame
    else if ((ZWAVE_TX_STATE_BACKOFF == state) && (BACKOFF_CURRENT_SESSION_ID == backoff_reason) && (zwave_tx_can_ignore_incoming_frames())) {
        sl_log_debug(LOG_TAG, "Pre-empting BACKOFF_CURRENT_SESSION_ID to dispatch a back-off-bypass frame. Back-off will be re-armed for the original session (id=%p) when the bypass frame completes.", current_tx_session_id);
        timer_stop(&backoff_timer);
        // Push, do not overwrite: a previous pre-emption may still be pending
        // its own restore (nested bypass chain). LIFO order matches how the
        // bypass chain unwinds in zwave_tx_restore_preempted_backoff_if_needed.
        preempted_backoff_stack.push_back(current_tx_session_id);
        state                          = ZWAVE_TX_STATE_TRANSMISSION_ONGOING;
        just_preempted_current_session = true;
    }

    // Check if we want to back-off
    clock_time_t backoff_time = 1 * CLOCK_SECOND;
    if ((ZWAVE_TX_STATE_IDLE == state) && (!expected_incoming_frames.empty()) && (!zwave_tx_can_ignore_incoming_frames())) {
        // We are idle and just expect some frames. Just make an additional back-off
        // It could be nice here to determine the length of the back-off based on the
        // number of frames / average transmission time with the nodes that are about
        // to send us something.
        backoff_time = 3 * CLOCK_SECOND;
        zwave_tx_process_initiate_backoff(backoff_time, BACKOFF_EXPECTED_ADDITIONAL_FRAMES);
    } else if ((ZWAVE_TX_STATE_BACKOFF != state) && (zwave_tx_is_protocol_sending_frames()) && (!just_preempted_current_session)) {
        if (!zwapi_supports_nls()) {
            // Let's wait 50 ms for the protocol before we try again.
            // We do not want extremely small values (like 1ms), else it's just going to spin for nothing. Too large value will create Tx Delays.
            backoff_time = CLOCK_SECOND / 20;
            zwave_tx_process_initiate_backoff(backoff_time, BACKOFF_PROTOCOL_SENDING_FRAMES);
        }
    }

    if (ZWAVE_TX_STATE_BACKOFF != state && !tx_queue.empty()) {
        if (zwave_tx_process_instance != nullptr) {
            zwave_tx_process_instance->post_event(ZWAVE_TX_SEND_NEXT_MESSAGE, nullptr);
        }
    }
}

static void zwave_tx_process_service_in_flight_watchdog()
{
    if (state != ZWAVE_TX_STATE_TRANSMISSION_ONGOING) {
        return;
    }

    // current_tx_session_id tracks the deepest in-flight child of the chain
    // (the encapsulated frame actually on the air), so its timestamp is what we
    // measure the stall against.
    zwave_tx_queue_element_t in_flight_element = {};
    if (tx_queue.get_by_id(&in_flight_element, current_tx_session_id) != SL_STATUS_OK) {
        return;
    }

    if (in_flight_element.transmission_timestamp == 0) {
        return;
    }

    // transmission_time is set when the chip callback arrives. If it is already
    // non-zero, the chip completed the TX and the session is waiting for incoming
    // replies rather than a completion callback — not a stuck in-flight send.
    // After backoff preemption or rollback, current_tx_session_id can point at a
    // session whose transmit already finished, so this guards against a false-positive.
    if (in_flight_element.transmission_time != 0) {
        return;
    }

    const clock_time_t in_flight_ms = clock_time() - in_flight_element.transmission_timestamp;
    if (in_flight_ms <= TX_FRAME_IN_FLIGHT_WATCHDOG_MS) {
        return;
    }

    // Resolve the actual chain root before tearing down the family: the
    // in-flight child only has descendants below it, so collecting from it would
    // orphan the ancestor encapsulation frames and leave the queue stuck.
    const zwave_tx_session_id_t root = tx_queue.find_root(current_tx_session_id);
    sl_log_warning(LOG_TAG,
                   "TX in-flight watchdog: chain root id=%p stuck for %lu ms; "
                   "transport callback lost. Force-failing the chain.",
                   root,
                   (unsigned long)in_flight_ms);

    std::vector<zwave_tx_session_id_t> family;
    tx_queue.collect_descendants(root, family);
    is_protocol_frame.store(false);

    for (const zwave_tx_session_id_t session_id: family) {
        zwave_controller_transport_abort_send_data(session_id);
    }

    for (auto it = family.rbegin(); it != family.rend(); ++it) {
        tx_queue.disable_fasttack(*it);
        tx_queue.set_transmissions_results(*it, TRANSMIT_COMPLETE_FAIL, nullptr);
        zwave_tx_message_transmission_completed_step(*it);
    }
}

void zwave_tx_process_set_expected_frames(zwave_node_id_t remote_node_id, uint8_t number_of_incoming_frames)
{
    expected_incoming_frames.set_frames(remote_node_id, number_of_incoming_frames);
    if ((ZWAVE_TX_STATE_BACKOFF == state) && (BACKOFF_EXPECTED_ADDITIONAL_FRAMES == backoff_reason) && (expected_incoming_frames.empty())) {
        sl_log_debug(LOG_TAG, "Expecting no more additional frames.");
        zwave_tx_resume_from_backoff_step();
    }
}

void zwave_tx_process_log_state()
{
    bool timer_running = (static_cast<int>(timer_expired(&backoff_timer)) == 0);
    sl_log_debug(LOG_TAG,
                 "Current Z-Wave Tx State: %s - Current(/last) Tx Session "
                 "(id=%p) - Backoff timer running: %d\n",
                 zwave_tx_state_name(state),
                 current_tx_session_id,
                 timer_running);
}

////////////////////////////////////////////////////////////////////////////////
// C++ Threading Implementation
////////////////////////////////////////////////////////////////////////////////
namespace zwave_component
{
    zwave_tx_process::zwave_tx_process() : threading::threading("Z-Wave TX Process"), initialized(false) {}

    zwave_tx_process::~zwave_tx_process()
    {
        cleanup();
    }

    void zwave_tx_process::initialize_internal()
    {
        state = ZWAVE_TX_STATE_IDLE;
        tx_queue.clear();
        expected_incoming_frames.clear();
        zwave_tx_init();
        zwave_tx_route_cache_init();
        zwave_tx_process_open_tx_queue();
        initialized = true;
        sl_log_info(LOG_TAG, "Z-Wave Tx process initialized.\n");
    }

    sl_status_t zwave_tx_process::initialize()
    {
        // Set the global singleton instance for C API callbacks
        // This matches what zwave_tx_process_init_and_start() does
        zwave_tx_process_instance = this;

        initialize_internal();
        return SL_STATUS_OK;
    }

    int zwave_tx_process::shutdown()
    {
        stop();
        cleanup();
        // Clear the global singleton instance
        // This matches what zwave_tx_process_stop_and_cleanup() does
        if (zwave_tx_process_instance == this) {
            zwave_tx_process_instance = nullptr;
        }
        return 0;
    }

    std::string zwave_tx_process::name() const
    {
        return "Z-Wave TX";
    }

    void zwave_tx_process::cleanup()
    {
        if (initialized) {
            timer_stop(&backoff_timer);
            state = ZWAVE_TX_STATE_IDLE;
            tx_queue.clear();
            expected_incoming_frames.clear();
            preempted_backoff_stack.clear();
            initialized = false;
            sl_log_info(LOG_TAG, "Z-Wave Tx process exited.\n");
        }

        zwave_tx_event event;
        while (event_queue.try_pop(event)) {
            if (event.event_type == ZWAVE_TX_FRAME_RECEIVED) {
                free_frame_received_event_data(event.data);
            }
        }
    }

    void zwave_tx_process::post_event(zwave_tx_events_t event_type, void *data)
    {
        zwave_tx_event event;
        event.event_type = event_type;
        event.data       = data;
        event_queue.push(event);
    }

    void zwave_tx_process::handle_event(zwave_tx_event event)
    {
        if (event.event_type == ZWAVE_TX_SEND_NEXT_MESSAGE) {
            if ((state == ZWAVE_TX_STATE_IDLE) && (!zwave_tx_can_ignore_incoming_frames()) && (!expected_incoming_frames.empty())) {
                sl_log_debug(LOG_TAG,
                             "Not sending the next message as we should "
                             "receive more frames first.");
                zwave_tx_process_log_state();
                zwave_tx_process_check_queue();
            } else if (state == ZWAVE_TX_STATE_BACKOFF) {
                // A new frame was enqueued while we are backing off.
                // check_queue() handles the back-off-bypass pre-emption logic
                // (ignore_incoming_frames_back_off flag) so the backoff can be
                // broken for urgent frames (e.g. S2 bootstrapping frames).
                zwave_tx_process_check_queue();
            } else {
                zwave_tx_process_send_next_message_step();
            }

        } else if (event.event_type == ZWAVE_TX_SEND_OPERATION_COMPLETE) {
            if (state != ZWAVE_TX_STATE_BACKOFF) {
                zwave_tx_message_transmission_completed_step((zwave_tx_session_id_t)event.data);
            } else {
                // A completion arrived while we are in BACKOFF.  This happens for
                // bypass frames pre-empting BACKOFF_CURRENT_SESSION_ID and for S2
                // child frames whose chip callback fires asynchronously.
                //
                // Run the completion step so the transport_completion_step_pending
                // flag is consumed immediately (guards against double-run), but
                // preserve the active backoff context if the completion step belongs
                // to a different session and doesn't re-arm a backoff itself.
                // Without the save/restore, zwave_tx_finalize_element() sets state=IDLE
                // for the completing session and the backed-off session's timer fires
                // into a non-BACKOFF state, skipping its own finalization entirely.
                auto *const completing_session               = (zwave_tx_session_id_t)event.data;
                const bool is_foreign_session                = (completing_session != current_tx_session_id);
                const zwave_tx_backoff_reason_t saved_reason = backoff_reason;
                const zwave_tx_session_id_t saved_session_id = current_tx_session_id;

                sl_log_debug(LOG_TAG, "TX completion dropped in backoff: id=%p backoff_reason=%s tid=%lu", event.data, zwave_back_off_reason_name(backoff_reason), sl_log_thread_id());
                zwave_tx_message_transmission_completed_step(completing_session);

                if (is_foreign_session && saved_reason == BACKOFF_CURRENT_SESSION_ID) {
                    if (state != ZWAVE_TX_STATE_BACKOFF) {
                        // The completion step changed state without re-arming a backoff
                        // (e.g. a child frame finalized to IDLE).  Restore the original
                        // session's backoff so the timer fires into the correct state.
                        state                 = ZWAVE_TX_STATE_BACKOFF;
                        backoff_reason        = saved_reason;
                        current_tx_session_id = saved_session_id;
                    } else if (current_tx_session_id != saved_session_id) {
                        // The completion step re-armed BACKOFF for the *completing*
                        // session, stealing the single backoff slot (timer +
                        // current_tx_session_id) from the session we were backing off.
                        // Defer the displaced session onto the restore stack so it is
                        // re-armed when the completing session's backoff resolves,
                        // instead of being stranded at the queue head as an "IDLE
                        // waiting for replies" wedge.
                        preempted_backoff_stack.push_back(saved_session_id);
                    }
                }
            }
        } else if (event.event_type == ZWAVE_TX_BACKOFF_TIMER_EXPIRED) {
            // Handle backoff timer expiration in the process thread to avoid
            // data races. The timer callback runs in the timer manager's worker thread.
            zwave_tx_resume_from_backoff_step();
        } else if (event.event_type == ZWAVE_TX_NODE_DELETED) {
            const zwave_node_id_t node_id = static_cast<zwave_node_id_t>(reinterpret_cast<uintptr_t>(event.data));
            zwave_tx_process_abort_transmissions_for_node(node_id);
        } else if (event.event_type == ZWAVE_TX_FRAME_RECEIVED) {
            auto *payload = static_cast<zwave_tx_frame_received_data_t *>(event.data);
            if (payload != nullptr) {
                zwave_tx_process_inspect_received_frame_step(payload->node_id, payload->frame_data, payload->frame_length);
                free_frame_received_event_data(payload);
            }
        } else {
            // If an event/state combination does not make sense, the
            // state machine may get stuck, so we print out a warning when
            // such an exception occurs.
            sl_log_warning(LOG_TAG,
                           "Unhandled Z-Wave Tx Event: %s while in State: %s. "
                           "data=%p\n",
                           zwave_tx_event_name(event.event_type),
                           zwave_tx_state_name(state),
                           event.data);
        }
    }

    void zwave_tx_process::run()
    {
        // Check if we should stop before doing any work
        if (should_stop() || threading::threading::is_kill_switch_activated()) {
            return;
        }

        if (!initialized) {
            initialize_internal();
        }

        // Use 100ms timeout to allow periodic checks for should_stop()
        auto event = event_queue.pop(100);
        if (event.has_value()) {
            handle_event(event.value());
        }

        static auto last_stall_diag_log = std::chrono::steady_clock::now();
        const auto now                  = std::chrono::steady_clock::now();
        if (now - last_stall_diag_log >= std::chrono::seconds(30)) {
            last_stall_diag_log = now;
            sl_log_debug(LOG_TAG, "TX state periodic: state=%s id=%p backoff_reason=%s queue_size=%d tid=%lu", zwave_tx_state_name(state), current_tx_session_id, zwave_back_off_reason_name(backoff_reason), tx_queue.size(), sl_log_thread_id());
            tx_queue.log_per_node_distribution();
            zwave_tx_process_service_in_flight_watchdog();
        }
    }
}  // namespace zwave_component

// C API wrapper functions
extern "C" {
sl_status_t zwave_tx_process_init_and_start(void)
{
    try {
        zwave_tx_process_instance = new zwave_component::zwave_tx_process();
        sl_status_t status        = zwave_tx_process_instance->initialize();
        if (status != SL_STATUS_OK) {
            delete zwave_tx_process_instance;
            zwave_tx_process_instance = nullptr;
            return status;
        }
        return SL_STATUS_OK;
    } catch (const std::exception &e) {
        sl_log_error(LOG_TAG, "Failed to initialize Z-Wave TX process: %s", e.what());
        if (zwave_tx_process_instance != nullptr) {
            delete zwave_tx_process_instance;
            zwave_tx_process_instance = nullptr;
        }
        return SL_STATUS_FAIL;
    }
}

sl_status_t zwave_tx_process_stop_and_cleanup(void)
{
    try {
        if (zwave_tx_process_instance != nullptr) {
            zwave_tx_process_instance->shutdown();
            delete zwave_tx_process_instance;
            zwave_tx_process_instance = nullptr;
        }
        return SL_STATUS_OK;
    } catch (const std::exception &e) {
        sl_log_error(LOG_TAG, "Error during cleanup: %s", e.what());
        return SL_STATUS_FAIL;
    }
}

void zwave_tx_process_post_event(zwave_tx_events_t event_type, void *data)
{
    if (zwave_tx_process_instance != nullptr) {
        zwave_tx_process_instance->post_event(event_type, data);
    }
}
}
