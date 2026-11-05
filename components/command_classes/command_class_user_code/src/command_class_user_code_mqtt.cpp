
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

#include <string_view>
#include <vector>

#include "command_class_user_code_constants.hpp"
#include "command_class_user_code_mqtt.hpp"
#include "command_class_user_code_rules.hpp"
#include "zwave_command_class_mqtt_utils.hpp"
#include "zpc_mqtt.hpp"

namespace zwave_command_class
{
    using namespace command_class_user_code_types;
    using namespace command_class_user_code_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_code_mqtt";

    command_class_user_code_mqtt::command_class_user_code_mqtt()
    {

        mqtt_callback_map.insert({"UserCodeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_user_code_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_user_code_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UsersNumberGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_users_number_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ExtendedUserCodeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_extended_user_code_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ExtendedUserCodeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_extended_user_code_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_user_code_capabilities_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeKeypadModeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeKeypadModeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AdminCodeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_admin_code_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AdminCodeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_admin_code_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeChecksumGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_user_code_mqtt::mqtt_on_user_code_checksum_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t identifier = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_identifier", identifier);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (identifier == 0 || !command_class_user_code_rules::user_identifier_is_supported(endpoint_node, identifier)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::USER_CODE_GET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::user_identifier)).set_desired(identifier);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t identifier = 0;
        uint8_t status     = 0;
        std::vector<uint8_t> code;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_identifier", identifier).parse("user_id_status", status).parse("user_code", code);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        // CL:0063.01.31.02.1: a version 2 peer is set with Extended User Code Set.
        if (command_class_user_code_rules::peer_version(endpoint_node) >= 2) {
            const auto packed = command_class_user_code_rules::pack_set_block(identifier, status, code);
            if (!command_class_user_code_rules::extended_user_code_blocks_are_valid(endpoint_node, 1, packed)) {
                return SL_STATUS_FAIL;
            }
            auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::EXTENDED_USER_CODE_SET_GROUP));
            group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes)).set_desired(static_cast<uint8_t>(1));
            group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).set_desired(packed);
            start_group_resolution(group);
            return SL_STATUS_OK;
        }

        if (identifier == 0 && status != STATUS_AVAILABLE) {
            return SL_STATUS_FAIL;
        }
        if (!command_class_user_code_rules::user_code_v1_set_is_valid(status, code) || !command_class_user_code_rules::user_identifier_is_supported(endpoint_node, identifier)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::USER_CODE_SET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_identifier)).set_desired(identifier);
        group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_id_status)).set_desired(status);
        group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_code)).set_desired(code);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_users_number_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(users_number_get_group_attributes_t::USERS_NUMBER_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_extended_user_code_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        struct block_t {
                uint16_t user_identifier = 0;
                uint8_t user_id_status   = 0;
                std::vector<uint8_t> user_code;
        };
        std::vector<block_t> blocks;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        for (auto [scope, item]: parser.parse_array<block_t>("vg1", blocks)) {
            scope.parse("user_identifier", item.user_identifier).parse("user_id_status", item.user_id_status).parse("user_code", item.user_code);
        }
        if (parser.status() != SL_STATUS_OK || blocks.empty() || blocks.size() > 255) {
            return SL_STATUS_FAIL;
        }
        std::vector<uint8_t> packed;
        for (const auto &block: blocks) {
            const auto one = command_class_user_code_rules::pack_set_block(block.user_identifier, block.user_id_status, block.user_code);
            packed.insert(packed.end(), one.begin(), one.end());
        }
        const auto count = static_cast<uint8_t>(blocks.size());
        if (!command_class_user_code_rules::extended_user_code_blocks_are_valid(endpoint_node, count, packed)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::EXTENDED_USER_CODE_SET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes)).set_desired(count);
        group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).set_desired(packed);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_extended_user_code_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t identifier = 0;
        uint8_t report_more = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_identifier", identifier).parse_optional("report_more", report_more);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (identifier == 0 || !command_class_user_code_rules::user_identifier_is_supported(endpoint_node, identifier)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::EXTENDED_USER_CODE_GET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::user_identifier)).set_desired(identifier);
        group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::report_more)).set_desired(report_more);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_get_group_attributes_t::USER_CODE_CAPABILITIES_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t mode = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("keypad_mode", mode);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_code_rules::keypad_mode_set_is_supported(endpoint_node, mode)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::USER_CODE_KEYPAD_MODE_SET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode)).set_desired(mode);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_get_group_attributes_t::USER_CODE_KEYPAD_MODE_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_admin_code_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t length = 0;
        std::vector<uint8_t> code;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("admin_code_length", length);
        parser.parse_optional("admin_code", code);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_code_rules::admin_code_is_valid(endpoint_node, length, code)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::ADMIN_CODE_SET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(length);
        if (!code.empty()) {
            group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(code);
        }
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_admin_code_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_get_group_attributes_t::ADMIN_CODE_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_checksum_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        if (!command_class_user_code_rules::user_code_checksum_get_is_supported(endpoint_node)) {
            return SL_STATUS_FAIL;
        }
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_checksum_get_group_attributes_t::USER_CODE_CHECKSUM_GET_GROUP)));
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class