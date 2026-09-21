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
#include <cstring>

#include "command_class_security.hpp"
#include "command_class_security_core.hpp"
#include "command_class_security_types.hpp"
#include "component_connector.hpp"

#include "granted_keys_resolver_types.hpp"
#include "granted_keys_resolver_events.hpp"

#include "ZW_classcmd.h"
#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_controller_utils.h"
#include "zwave_network_management.h"

#include "command_class_security_events.hpp"
#include "attribute_store_helper.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_security";

    command_class_security::command_class_security()
    {
        component_connector connector;
        connector.connect_typed<command_class_security_events_t, command_class_security_types::s0_supported_get_payload_t>(command_class_security_events_t::COMMAND_CLASS_SECURITY_COMMANDS_SUPPORTED_GET,
                                                                                                                           [](const command_class_security_types::s0_supported_get_payload_t &p) { return zwave_command_class::command_class_security::s0_supported_get(p); });
    }

    sl_status_t command_class_security::control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        if (connection_info->encapsulation != ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_0) {
            return SL_STATUS_FAIL;
        }
        return command_class_security_core::control_handler(connection_info, frame_data, frame_length);
    }

    sl_status_t command_class_security::s0_supported_get(command_class_security_types::s0_supported_get_payload_t payload_struct)
    {
        zwave_controller_connection_info_t connection = {};
        connection.remote.node_id                     = payload_struct.zwave_node_id;
        connection.remote.endpoint_id                 = payload_struct.endpoint_id;
        connection.local.node_id                      = zwave_network_management_get_node_id();
        connection.local.endpoint_id                  = 0;
        connection.encapsulation                      = ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_0;

        zwave_tx_options_t tx_options = {0};
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_RECOMMENDED_GET_ANSWER_PRIORITY - ZWAVE_TX_RECOMMENDED_QOS_GAP, 1, 0, &tx_options);

        uint8_t frame[2];
        uint16_t frame_len = 0;
        command_class_security::commands_supported_get(frame, &frame_len);

        sl_status_t transmit_status = zwave_tx_send_data(&connection, frame_len, frame, &tx_options, NULL, NULL, NULL);
        if (transmit_status != SL_STATUS_OK) {
            return SL_STATUS_FAIL;
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_security::commands_supported_get(uint8_t *frame, uint16_t *frame_length)
    {
        ZW_SECURITY_COMMANDS_SUPPORTED_GET_FRAME *security_0_get_frame = (ZW_SECURITY_COMMANDS_SUPPORTED_GET_FRAME *)frame;
        security_0_get_frame->cmdClass                                 = COMMAND_CLASS_SECURITY;
        security_0_get_frame->cmd                                      = SECURITY_COMMANDS_SUPPORTED_GET;
        *frame_length                                                  = sizeof(ZW_SECURITY_COMMANDS_SUPPORTED_GET_FRAME);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_security::on_security_commands_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint_node, command_class_security_attribute_map_t attribute_map)
    {
        reports_to_follow_t reports_to_follow = 0;
        reports_to_follow                     = get_value_or_default(attribute_map, "reports_to_follow", reports_to_follow);

        granted_keys_resolver_types::granted_keys_resolver_payload_t payload_map;
        payload_map.endpoint_node = attribute_store_get_first_parent_with_type(endpoint_node, ATTRIBUTE_NODE_ID);
        payload_map.encapsulation = connection_info->encapsulation;

        attribute_store_node_t report_node = attribute_store_create_child_if_missing(endpoint_node, ATTRIBUTE_ZWAVE_SECURE_NIF);

        attribute_store::attribute reports_to_follow_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(command_class_security_types::security_reports_to_follow_attributes_t::SECURITY_REPORTS_TO_FOLLOW_ATTRIBUTE));
        reports_to_follow_t previous_reports_to_follow    = reports_to_follow_node.reported_exists() ? reports_to_follow_node.reported<reports_to_follow_t>() : reports_to_follow_t(0);

        reports_to_follow_node.set_reported(reports_to_follow);
        if (reports_to_follow > 0) {
            zwave_tx_set_expected_frames(connection_info->remote.node_id, reports_to_follow);
        }

        std::vector<uint8_t> supported_cc_list;
        supported_cc_list = get_value_or_default(attribute_map, "command_class_support", supported_cc_list);

        const bool has_supported_cc_list = !supported_cc_list.empty() || attribute_map.contains("command_class_support");
        const bool continuing_sequence   = previous_reports_to_follow > 0 && reports_to_follow < previous_reports_to_follow;
        const bool starting_new_sequence = !continuing_sequence;

        if (continuing_sequence && has_supported_cc_list) {
            attribute_store_append_to_reported(report_node, supported_cc_list.data(), supported_cc_list.size());
        } else if (has_supported_cc_list && starting_new_sequence) {
            attribute_store_set_reported(report_node, supported_cc_list.data(), supported_cc_list.size());
        } else if (starting_new_sequence && previous_reports_to_follow == 0) {
            attribute_store_set_reported(report_node, supported_cc_list.data(), supported_cc_list.size());
        }

        attribute_store::attribute report_attr(report_node);
        if (report_attr.reported_exists()) {
            supported_cc_list = report_attr.reported<std::vector<uint8_t>>();
        }

        // Strip only when the segmented report is complete so the mark is not dropped mid-sequence.
        if (reports_to_follow == 0) {
            command_class_utils::strip_controlled_command_classes(supported_cc_list);
            attribute_store_set_reported(report_node, supported_cc_list.data(), supported_cc_list.size());
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(security_commands_supported_report_group_attributes_t::SECURITY_COMMANDS_SUPPORTED_REPORT_GROUP));
        auto cc_node    = group_node.emplace_node(static_cast<attribute_store_type_t>(security_commands_supported_report_group_attributes_t::command_class_support));
        cc_node.set_reported(supported_cc_list);

        if (reports_to_follow == 0) {
            component_connector connector;
            command_class_security_types::s0_supported_report_payload_t report_payload;
            memcpy(&report_payload.connection_info, connection_info, sizeof(zwave_controller_connection_info_t));
            report_payload.supported_cc_list = supported_cc_list;
            connector.fire_event(static_cast<uint32_t>(command_class_security_events_t::COMMAND_CLASS_SECURITY_COMMANDS_SUPPORTED_REPORT), report_payload);
            connector.fire_event(static_cast<uint32_t>(granted_keys_resolver_events_t::GRANTED_KEYS_RESOLVER_MARK_KEY_PROTOCOL_AS_SUPPORTED), payload_map);
        }

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
