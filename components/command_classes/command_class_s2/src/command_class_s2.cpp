
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

#include <algorithm>
#include <any>
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_s2.hpp"

#include "command_class_s2_core.hpp"

#include "component_connector.hpp"

#include "granted_keys_resolver_types.hpp"
#include "granted_keys_resolver_events.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_controller_utils.h"

#include "command_class_s2_events.hpp"

#include "zwave_network_management.h"
#include "attribute_store_helper.h"
#include "zpc_attribute_store_network_helper.h"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_s2";

    zwave_node_id_t command_class_s2::s_last_node_id = 0;

    command_class_s2::command_class_s2()
    {
        component_connector connector;
        connector.connect_typed<command_class_s2_events_t, command_class_s2_types::s2_supported_get_payload_t>(command_class_s2_events_t::COMMAND_CLASS_S2_COMMANDS_SUPPORTED_GET,
                                                                                                               [](const command_class_s2_types::s2_supported_get_payload_t &p) { return zwave_command_class::command_class_s2::s2_supported_get(p); });

        connector.connect_typed<command_class_s2_events_t, command_class_s2_types::s2_get_supported_command_class_list_payload_t, std::vector<uint8_t>>(
          command_class_s2_events_t::COMMAND_CLASS_S2_GET_SUPPORTED_COMMAND_CLASSES,
          [](const command_class_s2_types::s2_get_supported_command_class_list_payload_t &p, std::vector<uint8_t> &r) { return zwave_command_class::command_class_s2::s2_get_supported_command_class_list(p, r); });

        attribute_store_register_callback_by_type_and_state(&on_nls_state_desired_change, ATTRIBUTE_ZWAVE_NLS_STATE, DESIRED_ATTRIBUTE);
    }

    sl_status_t command_class_s2::s2_get_supported_command_class_list(const command_class_s2_types::s2_get_supported_command_class_list_payload_t &payload_struct, std::vector<uint8_t> &result)
    {

        attribute_store::attribute endpoint_node(payload_struct.endpoint_node);
        auto group_node         = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(s2_commands_supported_report_group_attributes_t::S2_COMMANDS_SUPPORTED_REPORT_GROUP));
        auto command_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(s2_commands_supported_report_group_attributes_t::command_class));

        result = command_class_node.reported<std::vector<uint8_t>>();

        return SL_STATUS_OK;
    }

    sl_status_t command_class_s2::s2_supported_get(command_class_s2_types::s2_supported_get_payload_t payload_struct)
    {
        // Just queue the frame directly ourselves.
        zwave_controller_connection_info_t connection = {};
        connection.remote.node_id                     = payload_struct.zwave_node_id;
        connection.remote.endpoint_id                 = payload_struct.endpoint_id;
        connection.local.node_id                      = zwave_network_management_get_node_id();
        connection.local.endpoint_id                  = 0;
        connection.encapsulation                      = zwave_controller_get_highest_encapsulation(payload_struct.granted_keys);

        // Prepare the Z-Wave TX options.
        zwave_tx_options_t tx_options = {0};
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_RECOMMENDED_GET_ANSWER_PRIORITY - ZWAVE_TX_RECOMMENDED_QOS_GAP,
                                            1,  // We expect 1 answer
                                            0,
                                            &tx_options);

        uint8_t frame[2];
        uint16_t frame_len = 0;
        command_class_s2::commands_supported_get(frame, &frame_len);

        // Pack node_id/endpoint into the user pointer so the TX callback can
        // notify the interviewer when air TX fails (status=2 hang otherwise).
        const uintptr_t user_token = (static_cast<uintptr_t>(payload_struct.zwave_node_id) << 8) | payload_struct.endpoint_id;

        sl_status_t transmit_status = zwave_tx_send_data(&connection, frame_len, frame, &tx_options, &command_class_s2::on_s2_supported_get_send_complete, reinterpret_cast<void *>(user_token), nullptr);
        if (transmit_status != SL_STATUS_OK) {
            fire_s2_supported_get_tx_failed(payload_struct.zwave_node_id, payload_struct.endpoint_id, static_cast<uint8_t>(transmit_status & 0xFF));
            return SL_STATUS_FAIL;
        }

        return SL_STATUS_OK;
    }

    void command_class_s2::fire_s2_supported_get_tx_failed(zwave_node_id_t node_id, uint8_t endpoint_id, uint8_t status)
    {
        sl_log_warning(LOG_TAG.data(), "S2 Commands Supported Get TX failed for NodeID %d endpoint %d (status=%u)", node_id, endpoint_id, status);

        command_class_s2_types::s2_supported_get_tx_failed_payload_t payload {};
        payload.zwave_node_id = node_id;
        payload.endpoint_id   = endpoint_id;
        payload.status        = status;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_s2_events_t::COMMAND_CLASS_S2_COMMANDS_SUPPORTED_GET_TX_FAILED), payload);
    }

    void command_class_s2::on_s2_supported_get_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
    {
        (void)tx_info;
        const uintptr_t user_token    = reinterpret_cast<uintptr_t>(user);
        const zwave_node_id_t node_id = static_cast<zwave_node_id_t>(user_token >> 8);
        const uint8_t endpoint_id     = static_cast<uint8_t>(user_token & 0xFF);

        if (status == TRANSMIT_COMPLETE_OK || status == TRANSMIT_COMPLETE_VERIFIED) {
            return;
        }

        fire_s2_supported_get_tx_failed(node_id, endpoint_id, status);
    }

    sl_status_t command_class_s2::commands_supported_get(uint8_t *frame, uint16_t *frame_len)
    {
        ZW_SECURITY_2_COMMANDS_SUPPORTED_GET_FRAME *security_2_get_frame = (ZW_SECURITY_2_COMMANDS_SUPPORTED_GET_FRAME *)frame;
        security_2_get_frame->cmdClass                                   = COMMAND_CLASS_SECURITY_2;
        security_2_get_frame->cmd                                        = SECURITY_2_COMMANDS_SUPPORTED_GET;
        *frame_len                                                       = sizeof(ZW_SECURITY_2_COMMANDS_SUPPORTED_GET_FRAME);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_s2::on_s2_commands_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint_node, command_class_s2_attribute_map_t attribute_map)
    {
        // zwave_command_class_mark_key_protocol_as_supported
        granted_keys_resolver_types::granted_keys_resolver_payload_t payload_map;
        payload_map.endpoint_node = attribute_store_get_first_parent_with_type(endpoint_node, ATTRIBUTE_NODE_ID);
        payload_map.encapsulation = connection_info->encapsulation;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(granted_keys_resolver_events_t::GRANTED_KEYS_RESOLVER_MARK_KEY_PROTOCOL_AS_SUPPORTED), payload_map);

        // Get the S2 capabilities atribute node under the endpoint
        attribute_store_node_t secure_nif_node = attribute_store_get_first_child_by_type(endpoint_node, ATTRIBUTE_ZWAVE_SECURE_NIF);

        std::vector<uint8_t> supported_cc_list;
        supported_cc_list = get_value_or_default(attribute_map, "supported_cc_list", supported_cc_list);

        // We have to drop values after the mark value in the list
        constexpr uint8_t command_class_mark = 0xEF;
        auto mark_iterator                   = std::find(supported_cc_list.begin(), supported_cc_list.end(), command_class_mark);
        supported_cc_list.erase(mark_iterator, supported_cc_list.end());

        if (supported_cc_list.empty()) {
            if (command_class_utils::is_using_zpc_highest_security_class(connection_info)) {
                attribute_store_delete_node(secure_nif_node);
                sl_log_debug(LOG_TAG.data(),
                             "Received empty S2 Commands Supported Report with an equal "
                             "or higher security scheme than the ZPC, deleting the "
                             "S2 capabilities from the Attribute Store");
            } else {
                sl_log_debug(LOG_TAG.data(),
                             "Received empty S2 Commands Supported Report with "
                             "security scheme lower than highest ZPC security scheme");
            }

        } else {
            // Note that Securely Supported CC list will not be larger than 255
            // Accept the capabilities only if it is received at the highest granted key
            if (connection_info->encapsulation != zwave_tx_scheme_get_node_highest_security_class(connection_info->remote.node_id)) {
                // Here it could be a downgrade attack, where we receive a non-secure
                // S2 Command Supported Report. Do not accept the contents!
                sl_log_warning(LOG_TAG.data(),
                               "Received S2 Commands Supported Report with "
                               "content on a 'non-secure' level. Discarding.");
            }

            attribute_store_set_child_reported(endpoint_node, ATTRIBUTE_ZWAVE_SECURE_NIF, supported_cc_list.data(), supported_cc_list.size());
        }

        command_class_s2_types::s2_supported_report_payload_t report_payload;
        memcpy(&report_payload.connection_info, connection_info, sizeof(zwave_controller_connection_info_t));
        report_payload.supported_cc_list = supported_cc_list;
        connector.fire_event(static_cast<uint32_t>(command_class_s2_events_t::COMMAND_CLASS_S2_COMMANDS_SUPPORTED_REPORT), report_payload);

        return SL_STATUS_OK;
    }

    void command_class_s2::on_nls_state_desired_change(attribute_store_node_t node, attribute_store_change_t change)
    {
        if (change != ATTRIBUTE_UPDATED) {
            return;
        }

        zwave_node_id_t node_id = 0;
        attribute_store_network_helper_get_node_id_from_node(node, &node_id);

        nls_state_set(node_id);
    }

    sl_status_t command_class_s2::nls_state_set(zwave_node_id_t node_id)
    {
        ZW_NLS_STATE_SET_V2_FRAME frame = {};
        frame.cmdClass                  = COMMAND_CLASS_SECURITY_2;
        frame.cmd                       = NLS_STATE_SET_V2;
        frame.nlsState                  = 1U;

        zwave_controller_connection_info_t connection_info = {};
        zwave_tx_options_t tx_options                      = {};
        constexpr uint8_t number_of_expected_responses     = 0;
        constexpr uint32_t discard_timeout_ms              = 5000;

        zwave_tx_scheme_get_node_connection_info(node_id, 0, &connection_info);
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_RECOMMENDED_GET_ANSWER_PRIORITY, number_of_expected_responses, discard_timeout_ms, &tx_options);

        s_last_node_id          = node_id;
        sl_status_t send_status = zwave_tx_send_data(&connection_info, sizeof(ZW_NLS_STATE_SET_V2_FRAME), reinterpret_cast<const uint8_t *>(&frame), &tx_options, &on_nls_state_set_send_complete, static_cast<void *>(&s_last_node_id), nullptr);

        if (send_status == SL_STATUS_OK) {
            sl_log_debug(LOG_TAG.data(), "Sending NLS State Set to NodeID %d", node_id);
        } else {
            sl_log_error(LOG_TAG.data(), "Failed to send NLS State Set to NodeID %d, status: 0x%04X", node_id, send_status);
        }

        return send_status;
    }

    void command_class_s2::on_nls_state_set_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
    {
        (void)tx_info;
        zwave_node_id_t node_id = *static_cast<zwave_node_id_t *>(user);

        sl_log_debug(LOG_TAG.data(), "NLS State Set send complete, status: %d, NodeID: %d", status, node_id);

        if (status != TRANSMIT_COMPLETE_OK && status != TRANSMIT_COMPLETE_VERIFIED) {
            sl_log_error(LOG_TAG.data(), "NLS State Set failed for NodeID %d", node_id);
            return;
        }

        ZW_NLS_STATE_GET_V2_FRAME frame = {};
        frame.cmdClass                  = COMMAND_CLASS_SECURITY_2;
        frame.cmd                       = NLS_STATE_GET_V2;

        zwave_controller_connection_info_t connection_info = {};
        zwave_tx_options_t tx_options                      = {};
        constexpr uint8_t number_of_expected_responses     = 1;
        constexpr uint32_t discard_timeout_ms              = 5000;

        zwave_tx_scheme_get_node_connection_info(node_id, 0, &connection_info);
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_MAX_PRIORITY, number_of_expected_responses, discard_timeout_ms, &tx_options);

        s_last_node_id          = node_id;
        sl_status_t send_status = zwave_tx_send_data(&connection_info, sizeof(ZW_NLS_STATE_GET_V2_FRAME), reinterpret_cast<const uint8_t *>(&frame), &tx_options, &on_nls_state_get_send_complete, static_cast<void *>(&s_last_node_id), nullptr);

        if (send_status == SL_STATUS_OK) {
            sl_log_debug(LOG_TAG.data(), "Sending NLS State Get to NodeID %d", node_id);
        } else {
            sl_log_error(LOG_TAG.data(), "Failed to send NLS State Get to NodeID %d, status: 0x%04X", node_id, send_status);
        }
    }

    void command_class_s2::on_nls_state_get_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
    {
        (void)tx_info;
        zwave_node_id_t node_id = *static_cast<zwave_node_id_t *>(user);

        sl_log_debug(LOG_TAG.data(), "NLS State Get send complete, status: %d, NodeID: %d", status, node_id);
    }

}  // namespace zwave_command_class