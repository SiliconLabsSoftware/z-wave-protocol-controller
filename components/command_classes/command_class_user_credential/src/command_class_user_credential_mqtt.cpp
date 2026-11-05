
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

#include "command_class_user_credential_constants.hpp"
#include "command_class_user_credential_mqtt.hpp"
#include "command_class_user_credential_rules.hpp"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    using namespace command_class_user_credential_types;
    using namespace command_class_user_credential_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_credential_mqtt";

    namespace
    {
        template<typename T> void set_desired(attribute_store::attribute group, attribute_store_type_t type, T value)
        {
            group.emplace_node(type).set_desired(value);
        }
    }  // namespace

    command_class_user_credential_mqtt::command_class_user_credential_mqtt()
    {

        mqtt_callback_map.insert({"UserCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_user_capabilities_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CredentialCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_credential_capabilities_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_user_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_user_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CredentialSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_credential_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CredentialGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_credential_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CredentialLearnStart", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_credential_learn_start_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CredentialLearnCancel", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_credential_learn_cancel_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserCredentialAssociationSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_user_credential_association_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AllUsersChecksumGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_all_users_checksum_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"UserChecksumGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_user_checksum_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CredentialChecksumGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_credential_checksum_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AdminPinCodeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_admin_pin_code_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AdminPinCodeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_admin_pin_code_get_command(endpoint_node, payload);
                                  }});
        mqtt_register_command_handler();
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_user_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_capabilities_get_group_attributes_t::USER_CAPABILITIES_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_credential_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_capabilities_get_group_attributes_t::CREDENTIAL_CAPABILITIES_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_user_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t operation   = 0;
        uint16_t identifier = 0;
        uint8_t user_type   = 0;
        uint8_t active      = 0;
        uint8_t rule        = 0;
        uint16_t timeout    = 0;
        uint8_t encoding    = 0;
        uint8_t name_length = 0;
        std::vector<uint8_t> name;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("operation_type", operation)
          .parse("user_unique_identifier", identifier)
          .parse("user_type", user_type)
          .parse("user_active_state", active)
          .parse("credential_rule", rule)
          .parse("expiring_timeout_minutes", timeout)
          .parse("user_name_encoding", encoding)
          .parse("user_name_length", name_length)
          .parse("user_name", name);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::user_set_is_valid(endpoint_node, operation, identifier, user_type, active, rule, timeout, encoding, name_length, name)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_set_group_attributes_t::USER_SET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::operation_type), operation);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_unique_identifier), identifier);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_type), user_type);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_active_state), active);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::credential_rule), rule);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::expiring_timeout_minutes), timeout);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_name_encoding), encoding);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_name_length), name_length);
        set_desired(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_name), name);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_user_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t identifier = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_unique_identifier", identifier);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::user_identifier_is_supported(endpoint_node, identifier, true)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_get_group_attributes_t::USER_GET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(user_get_group_attributes_t::user_unique_identifier), identifier);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_credential_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t identifier = 0;
        uint8_t type        = 0;
        uint16_t slot       = 0;
        uint8_t operation   = 0;
        uint8_t length      = 0;
        std::vector<uint8_t> data;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_unique_identifier", identifier).parse("credential_type", type).parse("credential_slot", slot).parse("operation_type", operation).parse("credential_length", length).parse("credential_data", data);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::credential_set_is_valid(endpoint_node, identifier, type, slot, operation, length, data)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_set_group_attributes_t::CREDENTIAL_SET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::user_unique_identifier), identifier);
        set_desired(group, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_type), type);
        set_desired(group, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_slot), slot);
        set_desired(group, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::operation_type), operation);
        set_desired(group, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_length), length);
        set_desired(group, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_data), data);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_credential_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t identifier = 0;
        uint8_t type        = 0;
        uint16_t slot       = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_unique_identifier", identifier).parse("credential_type", type).parse("credential_slot", slot);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::credential_get_is_valid(endpoint_node, identifier, type, slot)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_get_group_attributes_t::CREDENTIAL_GET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::user_unique_identifier), identifier);
        set_desired(group, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_type), type);
        set_desired(group, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_slot), slot);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_credential_learn_start_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t identifier = 0;
        uint8_t type        = 0;
        uint16_t slot       = 0;
        uint8_t operation   = 0;
        uint8_t timeout     = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_unique_identifier", identifier).parse("credential_type", type).parse("credential_slot", slot).parse("operation_type", operation).parse("credential_learn_timeout", timeout);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::credential_learn_start_is_valid(endpoint_node, identifier, type, slot, operation)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::CREDENTIAL_LEARN_START_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::user_unique_identifier), identifier);
        set_desired(group, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::credential_type), type);
        set_desired(group, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::credential_slot), slot);
        set_desired(group, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::operation_type), operation);
        set_desired(group, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::credential_learn_timeout), timeout);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_credential_learn_cancel_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_learn_cancel_group_attributes_t::CREDENTIAL_LEARN_CANCEL_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_user_credential_association_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t type         = 0;
        uint16_t slot        = 0;
        uint16_t destination = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("credential_type", type).parse("credential_slot", slot).parse("destination_user_unique_identifier", destination);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::user_credential_association_set_is_valid(endpoint_node, type, slot, destination)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_credential_association_set_group_attributes_t::USER_CREDENTIAL_ASSOCIATION_SET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(user_credential_association_set_group_attributes_t::credential_type), type);
        set_desired(group, static_cast<attribute_store_type_t>(user_credential_association_set_group_attributes_t::credential_slot), slot);
        set_desired(group, static_cast<attribute_store_type_t>(user_credential_association_set_group_attributes_t::destination_user_unique_identifier), destination);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_all_users_checksum_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        if (!command_class_user_credential_rules::all_users_checksum_get_is_supported(endpoint_node)) {
            return SL_STATUS_FAIL;
        }
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(all_users_checksum_get_group_attributes_t::ALL_USERS_CHECKSUM_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_user_checksum_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t identifier = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("user_unique_identifier", identifier);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::user_checksum_get_is_supported(endpoint_node, identifier)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_checksum_get_group_attributes_t::USER_CHECKSUM_GET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(user_checksum_get_group_attributes_t::user_unique_identifier), identifier);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_credential_checksum_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t type = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("credential_type", type);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::credential_checksum_get_is_supported(endpoint_node, type)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_checksum_get_group_attributes_t::CREDENTIAL_CHECKSUM_GET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(credential_checksum_get_group_attributes_t::credential_type), type);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_admin_pin_code_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t length = 0;
        std::vector<uint8_t> code;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("admin_pin_code_length", length).parse_optional("admin_code", code);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!command_class_user_credential_rules::admin_pin_code_set_is_valid(endpoint_node, length, code)) {
            return SL_STATUS_FAIL;
        }
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_pin_code_set_group_attributes_t::ADMIN_PIN_CODE_SET_GROUP));
        set_desired(group, static_cast<attribute_store_type_t>(admin_pin_code_set_group_attributes_t::admin_pin_code_length), length);
        // The resolver group can be reused after a previous Admin PIN Set.
        // Always update the code, including an empty vector for deactivation,
        // so an old PIN is not retained in the desired attribute.
        set_desired(group, static_cast<attribute_store_type_t>(admin_pin_code_set_group_attributes_t::admin_code), code);
        start_group_resolution(group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_mqtt::mqtt_on_admin_pin_code_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        if (!command_class_user_credential_rules::admin_pin_code_get_is_supported(endpoint_node)) {
            return SL_STATUS_FAIL;
        }
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_pin_code_get_group_attributes_t::ADMIN_PIN_CODE_GET_GROUP)));
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
