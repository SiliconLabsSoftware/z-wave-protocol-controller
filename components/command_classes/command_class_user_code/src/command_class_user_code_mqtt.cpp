
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
#include <vector>

#include "command_class_user_code.hpp"

#include "zpc_mqtt.hpp"
#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_code_mqtt";

    command_class_user_code_mqtt::command_class_user_code_mqtt()
    {
        mqtt_callback_map.insert({"UserCodeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_user_code_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_user_code_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UsersNumberGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_users_number_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ExtendedUserCodeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_extended_user_code_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ExtendedUserCodeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_extended_user_code_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_user_code_capabilities_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeKeypadModeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeKeypadModeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AdminCodeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_admin_code_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AdminCodeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_admin_code_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCodeChecksumGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_user_code_mqtt::mqtt_on_user_code_checksum_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t user_identifier = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_identifier", user_identifier);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::USER_CODE_GET_GROUP));
        group_node.clear_reported();
        auto user_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::user_identifier));
        user_identifier_node.set_desired(user_identifier);

        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t user_identifier = 0;
        uint8_t user_id_status  = 0;
        std::vector<uint8_t> user_code;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_identifier", user_identifier).parse("user_id_status", user_id_status).parse("user_code", user_code);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::USER_CODE_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_identifier)).set_desired(user_identifier);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_id_status)).set_desired(user_id_status);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_code)).set_desired(user_code);

        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_users_number_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(users_number_get_group_attributes_t::USERS_NUMBER_GET_GROUP));
        group_node.clear_reported();
        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_extended_user_code_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        std::vector<extended_user_code_set_vg1_t_item_t> vg1_items;
        std::vector<uint8_t> packed_vg1;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        for (auto &&[elem, vg1_item]: parser.parse_array("vg1", vg1_items)) {
            uint8_t user_code_length = 0;
            elem.parse("user_identifier", vg1_item.user_identifier).parse("user_id_status", vg1_item.user_id_status);
            elem.parse_nested("properties1").parse("user_code_length", user_code_length);
            elem.parse_optional("user_code", vg1_item.user_code);
            user_code_length = static_cast<uint8_t>(user_code_length & 0x0F);
            if (vg1_item.user_code.size() > user_code_length) {
                vg1_item.user_code.resize(user_code_length);
            } else {
                // The frame only contains the bytes supplied by the MQTT payload.
                // Keep the length field in sync so later variant groups stay aligned.
                user_code_length = static_cast<uint8_t>(vg1_item.user_code.size());
            }

            packed_vg1.push_back(static_cast<uint8_t>((vg1_item.user_identifier >> 8) & 0xFF));
            packed_vg1.push_back(static_cast<uint8_t>(vg1_item.user_identifier & 0xFF));
            packed_vg1.push_back(vg1_item.user_id_status);
            packed_vg1.push_back(user_code_length);
            packed_vg1.insert(packed_vg1.end(), vg1_item.user_code.begin(), vg1_item.user_code.end());
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::EXTENDED_USER_CODE_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes)).set_desired(static_cast<uint8_t>(vg1_items.size()));
        group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).set_desired(packed_vg1);

        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_extended_user_code_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t user_identifier = 0;
        uint8_t report_more      = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_identifier", user_identifier);
        parser.parse_nested("properties1").parse("report_more", report_more);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::EXTENDED_USER_CODE_GET_GROUP));
        group_node.clear_reported();
        group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::user_identifier)).set_desired(user_identifier);
        group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::report_more)).set_desired(static_cast<uint8_t>(report_more & 0x01));

        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_get_group_attributes_t::USER_CODE_CAPABILITIES_GET_GROUP));
        group_node.clear_reported();
        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t keypad_mode = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("keypad_mode", keypad_mode);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::USER_CODE_KEYPAD_MODE_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode)).set_desired(keypad_mode);

        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_keypad_mode_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_get_group_attributes_t::USER_CODE_KEYPAD_MODE_GET_GROUP));
        group_node.clear_reported();
        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_admin_code_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t admin_code_length = 0;
        std::vector<uint8_t> admin_code;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("admin_code_length", admin_code_length);
        parser.parse_optional("admin_code", admin_code);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        admin_code_length = static_cast<uint8_t>(admin_code_length & 0x0F);
        if (admin_code.size() > admin_code_length) {
            admin_code.resize(admin_code_length);
        } else {
            // The frame generator does not pad short or omitted arrays.
            admin_code_length = static_cast<uint8_t>(admin_code.size());
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::ADMIN_CODE_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(admin_code_length);
        group_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(admin_code);

        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_admin_code_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_get_group_attributes_t::ADMIN_CODE_GET_GROUP));
        group_node.clear_reported();
        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_mqtt::mqtt_on_user_code_checksum_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_checksum_get_group_attributes_t::USER_CODE_CHECKSUM_GET_GROUP));
        group_node.clear_reported();
        command_class_user_code_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
