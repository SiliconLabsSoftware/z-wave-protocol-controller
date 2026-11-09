
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
#include "zwave_command_class_supervision.h"
#include "zwave_command_class_supervision_internals.h"
#include "zwave_command_class_supervision_process.h"

// Includes from other components
#include "ZW_classcmd.h"
#include "log.h"

// ZPC includes
#include "zwave_tx_groups.h"
#include "zwapi_protocol_transport.h"

#include "timer.hpp"

// Generic includes
#include <atomic>
#include <map>
#include <mutex>
#include <cstdlib>
#include <string.h>
#include <thread>
#include <vector>

constexpr char LOG_TAG[] = "supervision_process";

static void supervision_process_on_timer_expired_event(void *ptr);

////////////////////////////////////////////////////////////////////////////////
// Private variables
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Supervision Timer used to periodically check for expired supervision
 * sessions
 */
static struct timer_handle_t supervision_timer {nullptr};

// List of Supervision sessions that are monitored.
std::map<supervision_id_t, supervised_session_t> sessions;
typedef std::map<supervision_id_t, supervised_session_t>::iterator session_iterator_t;
static std::recursive_mutex sessions_mutex;
static std::atomic<bool> expiry_thread_running {false};

class supervision_sessions_lock
{
    public:
        supervision_sessions_lock()
        {
            sessions_mutex.lock();
        }
        ~supervision_sessions_lock()
        {
            sessions_mutex.unlock();
        }
        supervision_sessions_lock(const supervision_sessions_lock &)            = delete;
        supervision_sessions_lock &operator=(const supervision_sessions_lock &) = delete;
};

// Variable for the next session-id to assign for a transmission
static supervision_id_t next_supervision_id = 1;
static std::atomic<uint8_t> next_session_id {0};

static clock_time_t compute_next_expiry_locked()
{
    clock_time_t next_expiry = 0;
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        if (it->second.expiry_time == 0) {
            continue;
        }
        if (next_expiry == 0) {
            next_expiry = it->second.expiry_time;
        } else if (it->second.expiry_time < next_expiry) {
            next_expiry = it->second.expiry_time;
        }
    }
    return next_expiry;
}

static void arm_supervision_timer(clock_time_t next_expiry)
{
    timer_stop(&supervision_timer);
    if (next_expiry == 0) {
        return;
    }
    const clock_time_t now = clock_time();
    if (next_expiry < now) {
        timer_set(&supervision_timer, 1, supervision_process_on_timer_expired_event, 0);
    } else {
        timer_set(&supervision_timer, next_expiry - now, supervision_process_on_timer_expired_event, 0);
    }
}

static void process_supervision_timer_expiry()
{
    struct running_flag_clear {
            ~running_flag_clear()
            {
                expiry_thread_running.store(false);
            }
    } running_flag_clear;

    clock_time_t now = clock_time();
    std::vector<supervision_id_t> expired_sessions;
    struct expired_callback {
            on_zwave_tx_send_data_complete_t callback;
            bool tx_info_valid;
            zwapi_tx_report_t tx_info;
            void *user;
    };
    std::vector<expired_callback> callbacks;
    {
        supervision_sessions_lock lock;
        for (session_iterator_t it = sessions.begin(); it != sessions.end(); ++it) {
            if (it->second.expiry_time <= now) {
                sl_log_debug(LOG_TAG,
                             "Timed out waiting for a Supervision Report for NodeID "
                             "%d:%d Session ID %d. "
                             "Considering Supervision ID %d Session failed",
                             it->second.session.node_id,
                             it->second.session.endpoint_id,
                             it->second.session.session_id,
                             it->first);
                expired_sessions.push_back(it->first);
                if (it->second.callback != nullptr) {
                    callbacks.push_back({it->second.callback, it->second.tx_info_valid, it->second.tx_info, it->second.user});
                }
            }
        }
        for (auto &session: expired_sessions) {
            sessions.erase(session);
        }
    }
    for (auto &cb: callbacks) {
        cb.callback(SUPERVISION_REPORT_FAIL, cb.tx_info_valid ? &cb.tx_info : nullptr, cb.user);
    }
    zwave_command_class_supervision_restart_timer();
}

static void increment_unique_supervision_id()
{
    next_supervision_id += 1;
    if (next_supervision_id == INVALID_SUPERVISION_ID) {
        next_supervision_id += 1;
    }
}

/**
 * @brief Restarts or stop the supervision timer towards the next supervision
 * session to expire.
 */
void zwave_command_class_supervision_restart_timer()
{
    clock_time_t next_expiry = 0;
    {
        supervision_sessions_lock lock;
        next_expiry = compute_next_expiry_locked();
    }
    arm_supervision_timer(next_expiry);
}

////////////////////////////////////////////////////////////////////////////////
// Private event functions
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Goes around and terminate the sessions that expired.
 */
static void supervision_process_on_timer_expired_event(void *ptr)
{
    (void)ptr;
    // Timer worker already holds / recently acquired the timer mutex. Do not
    // take sessions_mutex here (lock-order inversion vs API threads that
    // lock sessions then arm the timer).
    bool expected = false;
    if (expiry_thread_running.compare_exchange_strong(expected, true)) {
        std::thread(process_supervision_timer_expiry).detach();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Process functions (shared among the component)
////////////////////////////////////////////////////////////////////////////////
supervised_session_t *zwave_command_class_supervision_find_session_by_unique_id(supervision_id_t supervision_id)
{
    supervision_sessions_lock lock;
    if (sessions.contains(supervision_id)) {
        return (supervised_session_t *)(&sessions[supervision_id]);
    }
    return nullptr;
}

sl_status_t zwave_command_class_supervision_copy_session_id(supervision_id_t supervision_id, uint8_t *session_id)
{
    if (session_id == nullptr) {
        return SL_STATUS_FAIL;
    }
    supervision_sessions_lock lock;
    const auto it = sessions.find(supervision_id);
    if (it == sessions.end()) {
        return SL_STATUS_NOT_FOUND;
    }
    *session_id = it->second.session.session_id;
    return SL_STATUS_OK;
}

void zwave_command_class_supervision_handle_send_data_complete(supervision_id_t supervision_id, uint8_t status, const zwapi_tx_report_t *tx_info)
{
    on_zwave_tx_send_data_complete_t callback = nullptr;
    void *user                                = nullptr;
    bool invoke_fail                          = false;

    {
        supervision_sessions_lock lock;
        const auto it = sessions.find(supervision_id);
        if (it == sessions.end()) {
            // RX already received the report and closed the session, or multicast.
            return;
        }
        supervised_session_t &session = it->second;

        if ((status != TRANSMIT_COMPLETE_OK) && (status != TRANSMIT_COMPLETE_VERIFIED)) {
            if (session.callback != nullptr) {
                invoke_fail = true;
                callback    = session.callback;
                user        = session.user;
            }
            sessions.erase(it);
        } else {
            // Report can arrive before this TX callback; keep WORKING expiry.
            if (session.status != SUPERVISION_REPORT_WORKING) {
                session.expiry_time = clock_time() + SUPERVISION_REPORT_TIMEOUT;
            }
            session.tx_info_valid = true;
            if (tx_info != nullptr) {
                session.tx_info = *tx_info;
            }
        }
    }

    if (invoke_fail) {
        callback(SUPERVISION_REPORT_FAIL, tx_info, user);
        zwave_command_class_supervision_restart_timer();
    }
}

supervision_id_t zwave_command_class_supervision_find_session(uint8_t session_id, zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id)
{
    supervision_sessions_lock lock;
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        if (it->second.session.node_id == node_id && it->second.session.endpoint_id == endpoint_id && it->second.session.session_id == session_id) {
            return it->first;
        }
    }
    return INVALID_SUPERVISION_ID;
}

static supervision_id_t zwave_command_class_supervision_create_singlecast_session(zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id, uint32_t discard_timeout_ms, on_zwave_tx_send_data_complete_t callback, void *user)
{
    supervision_sessions_lock lock;
    supervised_session_t new_session = {};

    new_session.session.session_id  = next_session_id.load() & SUPERVISION_GET_PROPERTIES1_SESSION_ID_MASK;
    new_session.session.node_id     = node_id;
    new_session.session.endpoint_id = endpoint_id;
    new_session.session.group_id    = ZWAVE_TX_INVALID_GROUP;
    new_session.tx_info_valid       = false;
    new_session.callback            = callback;
    new_session.user                = user;
    new_session.tx_session_valid    = false;
    new_session.status              = SUPERVISION_REPORT_NO_SUPPORT;
    new_session.expiry_time         = clock_time() + discard_timeout_ms + SUPERVISION_SEND_DATA_EMERGENCY_TIMER;

    sessions.insert(std::pair<supervision_id_t, supervised_session_t>(next_supervision_id, new_session));
    supervision_id_t return_value = next_supervision_id;

    // Increment the Session IDs for the next call
    increment_unique_supervision_id();
    next_session_id.fetch_add(1);
    return return_value;
}

static supervision_id_t zwave_command_class_supervision_update_singlecast_follow_up_session(zwave_node_id_t node_id, zwave_multicast_group_id_t group_id, zwave_endpoint_id_t endpoint_id, uint32_t discard_timeout_ms, on_zwave_tx_send_data_complete_t callback, void *user)
{
    supervision_sessions_lock lock;
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        if (it->second.session.node_id == node_id && it->second.session.endpoint_id == endpoint_id && it->second.session.group_id == group_id) {
            it->second.callback    = callback;
            it->second.user        = user;
            it->second.expiry_time = clock_time() + discard_timeout_ms + SUPERVISION_SEND_DATA_EMERGENCY_TIMER;
            return it->first;
        }
    }
    sl_log_warning(LOG_TAG,
                   "Could not update Singlecast follow-up Supervision Session "
                   "for NodeID %d:%d, Group ID %d. Ignoring.",
                   node_id,
                   endpoint_id,
                   group_id);
    zwave_command_class_supervision_process_log();
    return INVALID_SUPERVISION_ID;
}

static supervision_id_t zwave_command_class_supervision_create_group_session(zwave_multicast_group_id_t group_id, zwave_endpoint_id_t endpoint_id, uint32_t discard_timeout_ms, on_zwave_tx_send_data_complete_t callback, void *user)
{
    supervision_sessions_lock lock;
    supervision_id_t return_value    = INVALID_SUPERVISION_ID;
    supervised_session_t new_session = {};
    new_session.session.session_id   = next_session_id.load() & SUPERVISION_GET_PROPERTIES1_SESSION_ID_MASK;
    new_session.session.endpoint_id  = endpoint_id;
    new_session.session.group_id     = group_id;
    new_session.callback             = callback;
    new_session.user                 = user;
    new_session.tx_info_valid        = false;
    new_session.tx_session_valid     = false;
    new_session.status               = SUPERVISION_REPORT_NO_SUPPORT;
    new_session.expiry_time          = clock_time() + discard_timeout_ms + SUPERVISION_SEND_DATA_EMERGENCY_TIMER;

    zwave_nodemask_t node_list = {};
    zwave_tx_get_nodes(node_list, group_id);
    for (zwave_node_id_t n = ZW_MIN_NODE_ID; n <= ZW_LR_MAX_NODE_ID; n++) {
        if (ZW_IS_NODE_IN_MASK(n, node_list)) {
            new_session.session.node_id = n;
            sessions.insert(std::pair<supervision_id_t, supervised_session_t>(next_supervision_id, new_session));
            return_value = next_supervision_id;
            increment_unique_supervision_id();
        }
    }

    // Increment the Session IDs for the next call, all nodes in this group have
    // the same Session ID
    next_session_id.fetch_add(1);
    return return_value;
}

supervision_id_t zwave_command_class_supervision_create_session(const zwave_controller_connection_info_t *connection, const zwave_tx_options_t *tx_options, on_zwave_tx_send_data_complete_t callback, void *user)
{
    supervision_sessions_lock lock;
    // What kind of message do we have here ?
    // Singlecast, Multicast, or singlecast follow-up ?
    if (connection->remote.is_multicast) {
        return zwave_command_class_supervision_create_group_session(connection->remote.multicast_group, connection->remote.endpoint_id, tx_options->discard_timeout_ms, callback, user);
    }
    if (tx_options->transport.group_id != ZWAVE_TX_INVALID_GROUP) {
        return zwave_command_class_supervision_update_singlecast_follow_up_session(connection->remote.node_id, tx_options->transport.group_id, connection->remote.endpoint_id, tx_options->discard_timeout_ms, callback, user);
    }
    return zwave_command_class_supervision_create_singlecast_session(connection->remote.node_id, connection->remote.endpoint_id, tx_options->discard_timeout_ms, callback, user);
}

sl_status_t zwave_command_class_supervision_assign_session_tx_id(supervision_id_t supervision_id, zwave_tx_session_id_t tx_session_id)
{
    supervision_sessions_lock lock;
    supervised_session_t *session = zwave_command_class_supervision_find_session_by_unique_id(supervision_id);
    if (session == nullptr) {
        return SL_STATUS_NOT_FOUND;
    }

    sl_log_debug(LOG_TAG,
                 "Assigning Z-Wave Tx Session %p to Supervision Session %d "
                 "(local SessionID %d with NodeID %d:%d, group %d)",
                 tx_session_id,
                 supervision_id,
                 session->session.session_id,
                 session->session.node_id,
                 session->session.endpoint_id,
                 session->session.group_id);

    session->tx_session_valid = true;
    session->tx_session_id    = tx_session_id;
    return SL_STATUS_OK;
}

sl_status_t zwave_command_class_supervision_close_session(supervision_id_t supervision_id)
{
    {
        supervision_sessions_lock lock;
        if (supervision_id == INVALID_SUPERVISION_ID) {
            return SL_STATUS_NOT_FOUND;
        }
        // Find the session
        session_iterator_t it = sessions.find(supervision_id);
        if (it == sessions.end()) {
            return SL_STATUS_NOT_FOUND;
        }

        sl_log_debug(LOG_TAG,
                     "Closing Supervision Session %d "
                     "(local SessionID %d with NodeID %d:%d, group %d)",
                     supervision_id,
                     it->second.session.session_id,
                     it->second.session.node_id,
                     it->second.session.endpoint_id,
                     it->second.session.group_id);
        sessions.erase(it);
    }
    zwave_command_class_supervision_restart_timer();

    return SL_STATUS_OK;
}

sl_status_t zwave_command_class_supervision_close_session_by_tx_session(zwave_tx_session_id_t tx_session_id)
{
    supervision_id_t supervision_id = INVALID_SUPERVISION_ID;
    {
        supervision_sessions_lock lock;
        for (auto it = sessions.begin(); it != sessions.end(); ++it) {
            if (it->second.tx_session_valid && it->second.tx_session_id == tx_session_id) {
                supervision_id = it->first;
                break;
            }
        }
    }
    if (supervision_id == INVALID_SUPERVISION_ID) {
        sl_log_debug(LOG_TAG, "Could not find Tx Session %p among supervised sessions", tx_session_id);
        return SL_STATUS_NOT_FOUND;
    }

    return zwave_command_class_supervision_close_session(supervision_id);
}

void zwave_command_class_supervision_process_log()
{
    {
        supervision_sessions_lock lock;
        sl_log_debug(LOG_TAG, "Number of Supervision Sessions: %d", sessions.size());
        for (auto it = sessions.begin(); it != sessions.end(); ++it) {
            sl_log_debug(LOG_TAG,
                         "\t - Supervision ID %d / NodeID %d:%d - Group %d, "
                         "Session ID %d - Callback (%p[%p]) - Expiry %lu",
                         it->first,
                         it->second.session.node_id,
                         it->second.session.endpoint_id,
                         it->second.session.group_id,
                         it->second.session.session_id,
                         it->second.callback,
                         it->second.user,
                         it->second.expiry_time);
        }
    }
}

void zwave_command_class_supervision_set_next_session_id(uint8_t session_id)
{
    next_session_id.store(session_id);
}

void zwave_command_class_supervision_on_node_deleted(zwave_node_id_t node_id)
{
    struct pending_callback {
            on_zwave_tx_send_data_complete_t callback;
            bool tx_info_valid;
            zwapi_tx_report_t tx_info;
            void *user;
    };
    std::vector<pending_callback> callbacks;
    {
        supervision_sessions_lock lock;
        std::vector<supervision_id_t> to_close;
        for (auto it = sessions.begin(); it != sessions.end(); ++it) {
            if (it->second.session.node_id == node_id) {
                to_close.push_back(it->first);
                if (it->second.callback != nullptr) {
                    callbacks.push_back({it->second.callback, it->second.tx_info_valid, it->second.tx_info, it->second.user});
                }
            }
        }
        for (supervision_id_t id: to_close) {
            sessions.erase(id);
        }
    }
    for (auto &cb: callbacks) {
        cb.callback(SUPERVISION_REPORT_FAIL, cb.tx_info_valid ? &cb.tx_info : nullptr, cb.user);
    }
    zwave_command_class_supervision_restart_timer();
}

sl_status_t zwave_command_class_supervision_handle_incoming_report(zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id, uint8_t session_id, uint8_t status, uint8_t more_status_updates, clock_time_t expiry_time)
{
    on_zwave_tx_send_data_complete_t callback = nullptr;
    bool tx_info_valid                        = false;
    zwapi_tx_report_t tx_info                 = {};
    void *user                                = nullptr;
    uint8_t callback_status                   = status;
    bool invoke_callback                      = false;

    {
        supervision_sessions_lock lock;
        const supervision_id_t supervision_id = zwave_command_class_supervision_find_session(session_id, node_id, endpoint_id);
        supervised_session_t *current_session = zwave_command_class_supervision_find_session_by_unique_id(supervision_id);
        if (current_session == nullptr) {
            sl_log_debug(LOG_TAG, "NodeID %d:%d sent us an unknown Supervision Session ID (%d)", node_id, endpoint_id, session_id);
            zwave_command_class_supervision_process_log();
            return SL_STATUS_OK;
        }

        current_session->status      = status;
        current_session->expiry_time = expiry_time;
        sl_log_debug(LOG_TAG,
                     "Incoming Supervision Report for NodeID %d:%d, group %d, "
                     "Session ID (%d). Status: %d ",
                     current_session->session.node_id,
                     current_session->session.endpoint_id,
                     current_session->session.group_id,
                     current_session->session.session_id,
                     current_session->status);

        if (current_session->status == SUPERVISION_REPORT_WORKING) {
            if (!static_cast<bool>(more_status_updates)) {
                sl_log_debug(LOG_TAG,
                             "NodeID %d:%d sent a WORKING status with the more_update "
                             "flag set to 0. Supervision Session ID (%d) will probably "
                             "time out automatically after its expiry duration",
                             node_id,
                             endpoint_id,
                             current_session->session.session_id);
            }
        }

        if (current_session->callback != nullptr) {
            invoke_callback = true;
            callback        = current_session->callback;
            tx_info_valid   = current_session->tx_info_valid;
            tx_info         = current_session->tx_info;
            user            = current_session->user;
            callback_status = current_session->status;
        }

        if (current_session->status != SUPERVISION_REPORT_WORKING) {
            sessions.erase(supervision_id);
        }
    }

    if (invoke_callback) {
        callback(callback_status, tx_info_valid ? &tx_info : nullptr, user);
    }
    zwave_command_class_supervision_restart_timer();
    return SL_STATUS_OK;
}
