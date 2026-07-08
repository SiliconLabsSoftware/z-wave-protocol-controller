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
#include "zpc_attribute_resolver.h"
#include "zpc_attribute_resolver_callbacks.h"

// ZPC components
#include "log.h"
#include "attribute_store_helper.h"
#include "attribute_timeouts.h"

// ZPC Components includes
#include "zwave_command_class_supervision.h"
#include "ZW_classcmd.h"
#include "zwave_utils.h"
#include "zwave_helper_macros.h"
#include "attribute_store_defined_attribute_types.h"

// Generic includes
#include <map>
#include <mutex>

constexpr char LOG_TAG[] = "zpc_attribute_resolver";

typedef struct node_data {
        resolver_rule_type_t rule_type;
        zwave_tx_session_id_t tx_session;
        bool valid_tx_session;
} node_data_t;

// Local map where we remember which nodes are being resolved
static std::map<attribute_store_node_t, node_data_t> nodes_under_resolution;

// Local list of functions to call when resolution events happened.
static std::map<attribute_store_type_t, zpc_resolver_event_notification_function_t> send_event_handlers;

// Protects nodes_under_resolution and send_event_handlers from concurrent access
// across the TX/supervision callback thread and the resolver/registration paths.
// on_resolver_send_data_complete and on_resolver_zwave_supervision_complete snapshot
// the required data under this lock and call on_resolver_send_data_complete outside it.
static std::mutex zpc_cb_mutex;

void add_node_in_resolution_list(attribute_store_node_t new_node, resolver_rule_type_t rule_type)
{
    node_data_t node_data      = {};
    node_data.rule_type        = rule_type;
    node_data.valid_tx_session = false;
    std::lock_guard<std::mutex> lk(zpc_cb_mutex);
    nodes_under_resolution.insert(std::pair<attribute_store_node_t, node_data_t>(new_node, node_data));
    sl_log_debug(LOG_TAG, "Resolution list add: node=%d rule_type=%d tid=%lu", new_node, rule_type, sl_log_thread_id());
}

bool is_node_in_resolution_list(attribute_store_node_t node)
{
    std::lock_guard<std::mutex> lk(zpc_cb_mutex);
    return nodes_under_resolution.contains(node);
}

void remove_node_from_resolution_list(attribute_store_node_t node)
{
    std::lock_guard<std::mutex> lk(zpc_cb_mutex);
    nodes_under_resolution.erase(node);
}

static void deferred_restart_set_resolution(attribute_store_node_t node)
{
    attribute_resolver_restart_set_resolution(node);
}

static void deferred_restart_get_resolution(attribute_store_node_t node)
{
    attribute_resolver_restart_get_resolution(node);
}

///////////////////////////////////////////////////////////////////////////////
// Function shared among the component. Used for send_data callbacks
///////////////////////////////////////////////////////////////////////////////
void on_resolver_zwave_send_data_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
{
    attribute_store_node_t current_node = (intptr_t)user;

    // Snapshot the node's rule type and remove it from the resolution list atomically.
    // The erase MUST happen before any callbacks are invoked: on_resolver_send_data_complete
    // posts NEXT_EVENT which causes the resolver thread to call attribute_resolver_send(),
    // whose is_node_in_resolution_list() guard must NOT find the stale entry.
    resolver_rule_type_t current_rule_type;
    zpc_resolver_event_notification_function_t custom_handler = nullptr;
    {
        std::lock_guard<std::mutex> lk(zpc_cb_mutex);
        auto it = nodes_under_resolution.find(current_node);
        if (it == nodes_under_resolution.end()) {
            sl_log_debug(LOG_TAG, "Send Data Callback for node %d not under resolution", current_node);
            return;
        }
        current_rule_type = it->second.rule_type;
        nodes_under_resolution.erase(it);
        if (IS_TRANSMISSION_SUCCESSFUL(status)) {
            sl_log_debug(LOG_TAG, "Resolution list erase: node=%d tx_status=%u tid=%lu", current_node, status, sl_log_thread_id());
        } else {
            sl_log_debug(LOG_TAG, "Resolution list erase: node=%d tx_status=%u tid=%lu", current_node, status, sl_log_thread_id());
        }

        auto handler_it = send_event_handlers.find(attribute_store_get_node_type(current_node));
        if (handler_it != send_event_handlers.end()) {
            custom_handler = handler_it->second;
        }
    }

    zpc_resolver_event_t event = FRAME_SENT_EVENT_OK_NO_SUPERVISION;
    resolver_send_status_t rs;
    switch (status) {
        case TRANSMIT_COMPLETE_OK:
            rs = RESOLVER_SEND_STATUS_OK;
            break;
        case TRANSMIT_COMPLETE_VERIFIED:
            rs = RESOLVER_SEND_STATUS_OK_EXECUTION_VERIFIED;
            break;
        default:
            rs    = RESOLVER_SEND_STATUS_FAIL;
            event = FRAME_SENT_EVENT_FAIL;
            break;
    }

    clock_time_t transmission_time = (tx_info != nullptr) ? tx_info->transmit_ticks * 10 : 0;

    // For NL (sleeping) nodes, a TX failure means the node went to sleep — not that
    // the command is invalid. Defer the resolution restart so it runs after the network
    // monitor has paused the node. Without the delay, the restart would call scan_node
    // before the pause, causing an immediate second failed attempt.
    if (rs == RESOLVER_SEND_STATUS_FAIL) {
        attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(current_node, ATTRIBUTE_NODE_ID);
        zwave_node_id_t node_id             = 0;
        attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));
        if (OPERATING_MODE_NL == zwave_get_operating_mode(node_id)) {
            rs = RESOLVER_SEND_STATUS_ALREADY_HANDLED;
            if (current_rule_type == RESOLVER_SET_RULE) {
                attribute_timeout_set_callback(current_node, 200, deferred_restart_set_resolution);
            } else {
                attribute_timeout_set_callback(current_node, 200, deferred_restart_get_resolution);
            }
        }
    }

    if (custom_handler == nullptr) {
        on_resolver_send_data_complete(rs, transmission_time, current_node, current_rule_type);
    } else {
        custom_handler(current_node, current_rule_type, event);
        on_resolver_send_data_complete(RESOLVER_SEND_STATUS_ALREADY_HANDLED, transmission_time, current_node, current_rule_type);
    }
}

void on_resolver_zwave_supervision_complete(uint8_t supervision_status, const zwapi_tx_report_t *tx_info, void *user)
{
    attribute_store_node_t current_node = (intptr_t)user;

    // Snapshot rule type, custom handler, and (if the session is terminal) erase
    // the entry — all under one lock, before any callbacks are invoked.
    resolver_rule_type_t current_rule_type;
    zpc_resolver_event_notification_function_t custom_handler = nullptr;
    {
        std::lock_guard<std::mutex> lk(zpc_cb_mutex);
        auto it = nodes_under_resolution.find(current_node);
        if (it == nodes_under_resolution.end()) {
            sl_log_debug(LOG_TAG, "Supervision Callback for node %d not under resolution", current_node);
            return;
        }
        current_rule_type = it->second.rule_type;
        if (supervision_status != SUPERVISION_REPORT_WORKING) {
            nodes_under_resolution.erase(it);
            sl_log_debug(LOG_TAG, "Resolution list erase: node=%d supervision_status=%u tid=%lu", current_node, supervision_status, sl_log_thread_id());
        } else {
            sl_log_debug(LOG_TAG, "Supervision working keep: node=%d tid=%lu", current_node, sl_log_thread_id());
        }

        auto handler_it = send_event_handlers.find(attribute_store_get_node_type(current_node));
        if (handler_it != send_event_handlers.end()) {
            custom_handler = handler_it->second;
        }
    }

    zpc_resolver_event_t event;
    resolver_send_status_t rs;
    switch (supervision_status) {
        case SUPERVISION_REPORT_SUCCESS:
            event = FRAME_SENT_EVENT_OK_SUPERVISION_SUCCESS;
            rs    = RESOLVER_SEND_STATUS_OK_EXECUTION_VERIFIED;
            break;
        case SUPERVISION_REPORT_NO_SUPPORT:
            event = FRAME_SENT_EVENT_OK_SUPERVISION_NO_SUPPORT;
            rs    = RESOLVER_SEND_STATUS_OK_EXECUTION_FAILED;
            break;
        case SUPERVISION_REPORT_FAIL:
            event = FRAME_SENT_EVENT_OK_SUPERVISION_FAIL;
            rs    = RESOLVER_SEND_STATUS_OK_EXECUTION_FAILED;
            break;
        case SUPERVISION_REPORT_WORKING:
            event = FRAME_SENT_EVENT_OK_SUPERVISION_WORKING;
            rs    = RESOLVER_SEND_STATUS_OK_EXECUTION_PENDING;
            break;
        default:
            event = FRAME_SENT_EVENT_OK_SUPERVISION_FAIL;
            rs    = RESOLVER_SEND_STATUS_FAIL;
            break;
    }

    clock_time_t transmission_time = (tx_info != nullptr) ? tx_info->transmit_ticks * 10 : 0;

    if (custom_handler == nullptr) {
        on_resolver_send_data_complete(rs, transmission_time, current_node, current_rule_type);
    } else {
        custom_handler(current_node, current_rule_type, event);
        on_resolver_send_data_complete(RESOLVER_SEND_STATUS_ALREADY_HANDLED, transmission_time, current_node, current_rule_type);
    }
}

sl_status_t attribute_resolver_associate_node_with_tx_sessions_id(attribute_store_node_t node, zwave_tx_session_id_t tx_session_id)
{
    std::lock_guard<std::mutex> lk(zpc_cb_mutex);
    auto it = nodes_under_resolution.find(node);
    if (it == nodes_under_resolution.end()) {
        return SL_STATUS_NOT_FOUND;
    }
    it->second.valid_tx_session = true;
    it->second.tx_session       = tx_session_id;
    return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Init/reset function
///////////////////////////////////////////////////////////////////////////////
void attribute_resolver_callbacks_reset()
{
    std::lock_guard<std::mutex> lk(zpc_cb_mutex);
    nodes_under_resolution.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t register_send_event_handler(attribute_store_type_t type, zpc_resolver_event_notification_function_t function)
{
    std::lock_guard<std::mutex> lk(zpc_cb_mutex);
    if (!send_event_handlers.contains(type)) {
        send_event_handlers[type] = function;
        return SL_STATUS_OK;
    }
    return SL_STATUS_ALREADY_EXISTS;
}

sl_status_t unregister_send_event_handler(attribute_store_type_t type, zpc_resolver_event_notification_function_t function)
{
    std::lock_guard<std::mutex> lk(zpc_cb_mutex);
    send_event_handlers.erase(type);
    return SL_STATUS_OK;
}

sl_status_t attribute_resolver_abort_pending_resolution(attribute_store_node_t node)
{
    bool valid_tx_session        = false;
    zwave_tx_session_id_t tx_sid = {};
    resolver_rule_type_t rule    = RESOLVER_GET_RULE;

    {
        std::lock_guard<std::mutex> lk(zpc_cb_mutex);
        if (!nodes_under_resolution.contains(node)) {
            sl_log_debug(LOG_TAG, "Node %d is not under resolution. Ignoring abort request", node);
            return SL_STATUS_NOT_FOUND;
        }
        sl_log_debug(LOG_TAG, "Aborting resolution for node %d", node);
        valid_tx_session = nodes_under_resolution[node].valid_tx_session;
        tx_sid           = nodes_under_resolution[node].tx_session;
        rule             = nodes_under_resolution[node].rule_type;
        nodes_under_resolution.erase(node);
    }

    if (valid_tx_session && (rule == RESOLVER_SET_RULE)) {
        zwave_command_class_supervision_close_session_by_tx_session(tx_sid);
    }
    on_resolver_send_data_complete(RESOLVER_SEND_STATUS_ABORTED, 0, node, rule);
    return SL_STATUS_OK;
}
