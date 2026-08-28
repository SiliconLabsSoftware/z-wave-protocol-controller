
/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>
#include <random>

// Base class
#include "command_class_supervision.hpp"

#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

#include "zwave_controller_utils.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_command_class_supervision_process.h"
#include "zwave_command_class_supervision.h"
#include "attribute_resolver.h"
#include "zwave_command_class_manager.h"
#include "attribute.hpp"
#include "zpc_attribute_store_network_helper.h"
#include "zwave_network_management.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_supervision";

    // Keep track of Supervision Sessions, when we are controlled.
    // We support only 1 session at a time, would abort an ongoing operation
    // for a new one if needed.
    static supervision_session_t supported_session = {0};
    static std::vector<uint8_t> supported_session_command;

    // Keep a list of nodes that must be "awaken" on demand
    static zwave_nodemask_t wake_on_demand_list = {0};

    command_class_supervision::command_class_supervision()
    {
        // Start up the first Session ID with a random number.
        std::random_device rd;
        std::mt19937 gen(rd());

        uint8_t next_session_id = gen() % UINT8_MAX;

        zwave_command_class_supervision_set_next_session_id(next_session_id);
        // Clean up our "Wake on demand" list
        memset(wake_on_demand_list, 0, sizeof(zwave_nodemask_t));

        component_connector connector;
        connector.connect_typed<component_connector_common_events_t, component_connector_node_deleted_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_DELETED, [](const component_connector_node_deleted_payload_t &payload) {
            zwave_command_class_supervision_on_node_deleted(payload.node_id);
            return SL_STATUS_OK;
        });
    }

    sl_status_t command_class_supervision::on_supervision_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_supervision_attribute_map_t payload)
    {
        zwave_command_class_supervision_restart_timer();

        uint8_t session_id = 0;
        session_id         = get_value_or_default(payload, "session_id", session_id);

        uint8_t more_status_updates = 0;
        more_status_updates         = get_value_or_default(payload, "more_status_updates", more_status_updates);

        uint8_t status = 0;
        status         = get_value_or_default(payload, "status", status);

        uint8_t duration = 0;
        duration         = get_value_or_default(payload, "duration", duration);

        // Verify that the Supervision Report corresponds to the session that we are currently controlling.
        supervision_id_t supervision_id = zwave_command_class_supervision_find_session(session_id, connection_info->remote.node_id, connection_info->remote.endpoint_id);

        supervised_session_t *current_session = zwave_command_class_supervision_find_session_by_unique_id(supervision_id);

        // If we have no idea about this session, we will just ignore it.
        if (current_session == NULL) {
            // Maybe a duplicate transmission of a finished report.
            // Just ignore it happily.
            sl_log_debug(LOG_TAG.data(), "NodeID %d:%d sent us an unknown Supervision Session ID (%d)", connection_info->remote.node_id, connection_info->remote.endpoint_id, session_id);
            zwave_command_class_supervision_process_log();
            return SL_STATUS_OK;
        }

        current_session->status = status;

        // Print out supervision session data
        sl_log_debug(LOG_TAG.data(),
                     "Incoming Supervision Report for NodeID %d:%d, group %d, "
                     "Session ID (%d). Status: %d ",
                     current_session->session.node_id,
                     current_session->session.endpoint_id,
                     current_session->session.group_id,
                     current_session->session.session_id,
                     current_session->status);

        if (duration < 0xFE) {
            current_session->expiry_time = clock_time() + command_class_utils::zwave_duration_to_time(duration) + SUPERVISION_REPORT_TIMEOUT;
        } else {  // Duration is unknown. Allocate some default waiting time here.
            current_session->expiry_time = clock_time() + (SUPERVISION_DEFAULT_SESSION_DURATION) + SUPERVISION_REPORT_TIMEOUT;
        }

        if (current_session->status == SUPERVISION_REPORT_WORKING) {
            if (!static_cast<bool>(more_status_updates)) {
                sl_log_debug(LOG_TAG.data(),
                             "NodeID %d:%d sent a WORKING status with the more_update "
                             "flag set to 0. Supervision Session ID (%d) will probably "
                             "time out automatically after its expiry duration",
                             connection_info->remote.node_id,
                             connection_info->remote.endpoint_id,
                             current_session->session.session_id);
            }
            zwave_command_class_supervision_restart_timer();

            // Give the callback to the user, so that it knows that the node is working
            if (current_session->callback != NULL) {
                current_session->callback(current_session->status, current_session->tx_info_valid ? &current_session->tx_info : NULL, current_session->user);
            }

        } else {
            // Close this session if it is not WORKING
            if (current_session->callback != NULL) {
                current_session->callback(current_session->status, current_session->tx_info_valid ? &current_session->tx_info : NULL, current_session->user);
            }
            zwave_command_class_supervision_close_session(supervision_id);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_supervision::on_supervision_get_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_supervision_attribute_map_t payload)
    {
        uint8_t session_id                  = 0;
        session_id                          = get_value_or_default(payload, "session_id", session_id);
        uint8_t encapsulated_command_length = 0;
        encapsulated_command_length         = get_value_or_default(payload, "encapsulated_command_length", encapsulated_command_length);
        std::vector<uint8_t> encapsulated_command;
        encapsulated_command = get_value_or_default(payload, "encapsulated_command", encapsulated_command);

        // CC:006C.01.01.11.00D ignore duplicate singlecast with the same Session ID and same encapsulated command.
        if ((session_id == supported_session.session_id) && (connection_info->remote.node_id == supported_session.node_id) && (connection_info->remote.endpoint_id == supported_session.endpoint_id) && (encapsulated_command == supported_session_command) && !connection_info->local.is_multicast) {
            sl_log_debug(LOG_TAG.data(), "Ignoring Supervision Get duplicate");
            return SL_STATUS_OK;
        }

        // If we could not get a status from a command handler, it is
        // recommended to default to NO_SUPPORT. FAIL encourages the
        // controlling node to try again.
        sl_status_t command_handler_status = SL_STATUS_NOT_SUPPORTED;

        // Unwrapping SUPERVISION encapsulation and insert to the Command Handler framework
        if (encapsulated_command.size() != encapsulated_command_length) {
            command_handler_status = SL_STATUS_NOT_SUPPORTED;
        } else if (!encapsulated_command.empty()) {
            command_handler_status = zwave_command_class_manager::dispatch(connection_info, encapsulated_command.data(), static_cast<uint16_t>(encapsulated_command.size()));
        }

        // Return a response if it was the first singlecast.
        if (!connection_info->local.is_multicast) {
            // Accept the SessionID and copy the data.
            supported_session.session_id  = session_id;
            supported_session.node_id     = connection_info->remote.node_id;
            supported_session.endpoint_id = connection_info->remote.endpoint_id;
            supported_session_command     = encapsulated_command;
            // Start preparing a report.
            ZW_SUPERVISION_REPORT_FRAME report;
            report.cmdClass    = COMMAND_CLASS_SUPERVISION;
            report.cmd         = SUPERVISION_REPORT;
            report.duration    = 0;
            report.properties1 = supported_session.session_id;

            // More Status Update is not supported as we never return WAITING.
            // Do not add the WAITING return code, unless you implement support for More Status Update.
            switch (command_handler_status) {
                case SL_STATUS_OK:
                    report.status = SUPERVISION_REPORT_SUCCESS;
                    break;
                case SL_STATUS_FAIL:
                case SL_STATUS_BUSY:
                    report.status = SUPERVISION_REPORT_FAIL;
                    break;
                case SL_STATUS_NOT_SUPPORTED:
                default:
                    report.status = SUPERVISION_REPORT_NO_SUPPORT;
                    break;
            }

            // Do we need to Wake Up on Demand?
            // (either via ZPC Stin command or via pending resolution)
            // attribute_store_node_t node = zwave_command_class_get_node_id_node(connection_info);
            // if (zwave_command_class_wake_up_supports_wake_up_on_demand(node) == true) {
            //     if (true == ZW_IS_NODE_IN_MASK(connection_info->remote.node_id, wake_on_demand_list)) {
            //         report.properties1 |= SUPERVISION_REPORT_PROPERTIES1_WAKE_UP_BIT_MASK;
            //     } else if (is_node_or_parent_paused(node) == true && attribute_resolver_node_or_child_needs_resolution(node) == true) {
            //         report.properties1 |= SUPERVISION_REPORT_PROPERTIES1_WAKE_UP_BIT_MASK;
            //     }
            // }

            // Send our Supervision Report.
            sl_status_t send_data_status = command_class_utils::send_report(connection_info, sizeof(report), reinterpret_cast<const uint8_t *>(&report));
            if (send_data_status == SL_STATUS_OK) {
                // Remove the node from the Wake On Demand list in any case
                zwave_command_class_supervision_stop_wake_on_demand(connection_info->remote.node_id);
            }
            return send_data_status;
        }

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class

extern "C" bool zwave_command_class_supervision_want_supervision_frame(zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id)
{
    sl_log_debug(zwave_command_class::LOG_TAG.data(), "Want supervision frame enter: node_id=%u endpoint=%u", node_id, endpoint_id);

    zwave_home_id_t home_id              = zwave_network_management_get_home_id();
    attribute_store_node_t endpoint_node = attribute_store_network_helper_get_endpoint_node(home_id, node_id, endpoint_id);
    auto endpoint_attribute              = attribute_store::attribute(endpoint_node);
    if (!endpoint_attribute.is_valid()) {
        sl_log_debug(zwave_command_class::LOG_TAG.data(), "Want supervision frame no endpoint: node_id=%u endpoint=%u", node_id, endpoint_id);
        return false;
    }

    auto version_node          = endpoint_attribute.child_by_type(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_SUPERVISION));
    const bool version_exists  = version_node.reported_exists();
    const uint8_t version      = version_exists ? version_node.reported<uint8_t>() : 0;
    const bool use_supervision = version_exists && (version > 0);
    sl_log_debug(zwave_command_class::LOG_TAG.data(), "Want supervision frame result: node_id=%u endpoint=%u endpoint_node=%u version_exists=%d version=%u use_supervision=%d", node_id, endpoint_id, endpoint_node, version_exists ? 1 : 0, version, use_supervision ? 1 : 0);
    return use_supervision;
}
