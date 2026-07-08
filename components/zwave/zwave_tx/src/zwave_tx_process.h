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

#ifndef ZWAVE_TX_PROCESS_H
#define ZWAVE_TX_PROCESS_H

#include "zwave_tx.h"
/**
 * @defgroup zwave_tx_process Z-Wave TX Process
 * @ingroup zwave_tx
 * @brief Z-Wave TX process taking care of the TX Queue and interfaces with the @ref zwave_api
 *
 * The Z-Wave TX Process is responsible for looking into the
 * \ref zwave_tx_queue and perform transmission requests, monitor callbacks
 * with the @ref zwave_api. This process requires the @ref zwave_api to be initialized
 * before it will work properly. The TX Process is a small state machine
 * consisting of 3 states, depicted in the following diagram:
 *
@startuml
' Style for the diagram
!theme plain
skinparam ActivityBackgroundColor #DEDEDE
skinparam ActivityBorderColor #480509
skinparam ActivityBorderThickness 2
skinparam ActivityFontColor #000000
skinparam ActivityStartColor #797777

partition "Z-Wave TX Process" {
  (*) -->[init] ZWAVE_TX_STATE_IDLE
  if "Queue empty?" then
    -->[Yes] ZWAVE_TX_STATE_IDLE
  else
    -->[No] ZWAVE_TX_STATE_TRANSMISSION_ONGOING
  endif
}

partition "Z-Wave API" {
  --> zwapi_send_data()
}

partition "Z-Wave TX Process" {
  -->[Yes] ZWAVE_TX_STATE_BACKOFF
  if "Backoff timer expired/answer received" then
    if "Child frame?" then
      -->[Yes] ZWAVE_TX_STATE_TRANSMISSION_ONGOING
    else
      -->[No] ZWAVE_TX_STATE_IDLE
    endif
  else
    -->[wait] ZWAVE_TX_STATE_BACKOFF
endif
}
@enduml
 *
 * The \ref zwave_tx_process must be started after the \ref zwave_rx_process
 * @{
 */

/**
 * @brief Event definitions for the Z-Wave TX Process.
 */
typedef enum {
    /// Send the next message in the TX Queue.
    ZWAVE_TX_SEND_NEXT_MESSAGE,
    /// The ongoing transmission is now completed.
    ZWAVE_TX_SEND_OPERATION_COMPLETE,
    /// The backoff timer has expired and we should resume from backoff.
    ZWAVE_TX_BACKOFF_TIMER_EXPIRED,
    /// A NodeID was removed from the network
    ZWAVE_TX_NODE_DELETED,
    /// An incoming Z-Wave frame was received.
    ZWAVE_TX_FRAME_RECEIVED,
} zwave_tx_events_t;

/**
 * @brief List of reasons for going into a Tx Back-off-
 */
typedef enum {
    /// Back-off has been initiated due to the current_tx_session_id expecting
    /// some responses
    BACKOFF_CURRENT_SESSION_ID,
    /// Back-off has been initiated due to the application telling us of expected
    /// incoming frames.
    BACKOFF_EXPECTED_ADDITIONAL_FRAMES,
    /// Back-off has been initiated because Z-Wave API module is sending frames
    /// on its own, and we do not want to interfere with this.
    BACKOFF_PROTOCOL_SENDING_FRAMES,
    /// Back-off has been initiated becasue we received a routed frame and we
    /// want to avoid using the route before we are sure that the sender has
    /// received the routed ack.
    BACKOFF_INCOMING_UNSOLICITED_ROUTED_FRAME,
} zwave_tx_backoff_reason_t;

/**
 * @brief The Z-Wave TX Process states.
 */
typedef enum zwave_tx_state {
    /// Z-Wave TX is idle and new transmissions to Z-Wave nodes
    /// can be initiated.
    ZWAVE_TX_STATE_IDLE,
    /// Z-Wave TX has passed on the message to the Z-Wave module
    /// and waits for a callback.
    ZWAVE_TX_STATE_TRANSMISSION_ONGOING,
    /// Z-Wave TX waits after a transmission that requires a response
    /// to minimize the risk of radio transmit collisions.
    ZWAVE_TX_STATE_BACKOFF,
} zwave_tx_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief React to any incoming frame from a NodeID.
 *
 * Does three things:
 *   1. Decrements the BACKOFF_EXPECTED_ADDITIONAL_FRAMES counter.
 *   2. Applies the routing back-off (avoid colliding with a repeater
 *      about to forward).
 *   3. Counts the reply for the current TX session. S2 nonce frames
 *      (NONCE_GET / NONCE_REPORT) are skipped here since they are part
 *      of the S2 handshake, not application replies.
 *
 * Must be called before any security transport sees the frame; otherwise
 * reply counting races the transport's send-done callback.
 *
 * Posts a ZWAVE_TX_FRAME_RECEIVED event to the TX worker thread so shared
 * state is not mutated from the RX thread.
 *
 * @param node_id      Z-Wave NodeID that sent us the frame.
 * @param frame_data   Raw Z-Wave payload, or NULL if not available.
 * @param frame_length Length of `frame_data`, or 0 if `frame_data` is NULL.
 */
void zwave_tx_process_inspect_received_frame(zwave_node_id_t node_id, const uint8_t *frame_data, uint16_t frame_length);

/**
 * @brief Aborts a transmission that has been queued but not delivered yet.
 *
 * This function is used to attempt to abort a queued or ongoing transmission.
 *
 * @param session_id the session_id of the element to abort.
 * @returns
 * - SL_STATUS_IN_PROGRESS if the element was sent to the @ref zwave_api and
 *                         cancellation was requested but pending
 *                         @ref zwave_api to return a callback.
 * - SL_STATUS_OK          if the element was removed from the queue.
 * - SL_STATUS_NOT_FOUND   if the element identified by session_id does not
 *                         exist in the queue.
 */
sl_status_t zwave_tx_process_abort_transmission(zwave_tx_session_id_t session_id);

/**
 * @brief Notifies the Z-Wave TX process that a NodeID has been removed from
 * the network so any queued transmissions targeting it can be aborted.
 *
 * @param node_id  NodeID that was removed.
 */
void zwave_tx_process_on_node_deleted(zwave_node_id_t node_id);

/**
 * @brief Verifies if we are trying to flush the queue or keep it empty.
 *
 * @returns boolean value.
 */
bool zwave_tx_process_queue_flush_is_ongoing();

/**
 * @brief Triggers the processing of the next frame in the queue if we are idle
 *
 * Use this function instead of posting directly a ZWAVE_TX_SEND_NEXT_MESSAGE
 * event to the Z-Wave TX Process
 */
void zwave_tx_process_check_queue();

/**
 * @brief Tells the Z-Wave TX process that more frames are to be expected f
 *        from some NodeIDs.
 *
 * @param remote_node_id              The remote NodeID that will send us >1 frame(s)
 * @param number_of_incoming_frames   The number of frames to add.
 */
void zwave_tx_process_set_expected_frames(zwave_node_id_t remote_node_id, uint8_t number_of_incoming_frames);

/**
 * @brief Logs the state of the Z-Wave TX Process using sl_log
 */
void zwave_tx_process_log_state();

/**
 * @brief Initiates a flush of the Tx Queue for a reset operation.
 *
 * Tx Queue will be closed for new elements until calling
 * zwave_tx_process_open_tx_queue();
 */
sl_status_t zwave_tx_process_flush_queue_reset_step();

/**
 * @brief (Re-)opens the Z-Wave Tx Queue to accept frames.
 */
void zwave_tx_process_open_tx_queue();

/**
 * @brief Initialize and start the Z-Wave TX process thread
 * @returns SL_STATUS_OK on success, SL_STATUS_FAIL on error
 */
sl_status_t zwave_tx_process_init_and_start(void);

/**
 * @brief Stop and cleanup the Z-Wave TX process thread
 * @returns SL_STATUS_OK on success, SL_STATUS_FAIL on error
 */
sl_status_t zwave_tx_process_stop_and_cleanup(void);

/**
 * @brief Post an event to the Z-Wave TX process
 * @param event_type The event type to post
 * @param data Optional data pointer associated with the event
 */
void zwave_tx_process_post_event(zwave_tx_events_t event_type, void *data);

#ifdef __cplusplus
}
#endif

/** @} end of zwave_tx_process */

#endif  // ZWAVE_TX_PROCESS_H
