

/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
// Base class
#include "zpc_mqtt_attribute_registration.hpp"

// C++ lib
#include <fmt/base.h>
#include <fmt/format.h>

// ZPC
#include "zwave_node_id_definitions.h"                // zwave_node_id_t, zwave_home_id_t, zwave_endpoint_id_t
#include "attribute_store_defined_attribute_types.h"  // ATTRIBUTE_ENDPOINT_ID

// ZPC
#include "log.h"

// Internal component
#include "zpc_mqtt_definitions.hpp"
#include "utils.hpp"
#include "zpc_mqtt_utils.hpp"  // zpc_mqtt::utils::get_base_topic_from_attribute

// Json
#include "nlohmann/json.hpp"

namespace zpc_mqtt
{
    constexpr char LOG_TAG[] = "zpc_mqtt_attribute_registration";

    void zpc_mqtt_attribute_registration::register_attribute_type(attribute_store_type_t attribute_type, const std::string &command_class_name, const std::string &attribute_name)
    {
        if (mqtt_registration_map.contains(attribute_type)) {
            sl_log_error(LOG_TAG, "Attribute type %d already registered to MQTT. Not registering again.", attribute_type);
            return;
        }

        mqtt_registration_map[attribute_type] = {command_class_name, attribute_name};
    }

    std::string zpc_mqtt_attribute_registration::get_attribute_topic(const attribute_store::attribute &attribute) const
    {
        std::string attribute_topic;
        try {
            // Get back the attribute name and command class name from the map
            auto node_type                   = attribute.type();
            const auto &value                = mqtt_registration_map.at(node_type);
            const std::string command_class  = value.command_class_name;
            const std::string attribute_name = value.attribute_name;

            // Default path if attribute is under the endpoint
            std::string attribute_path;

            // Check if attribute is nested
            auto endpoint_node = attribute.first_parent(ATTRIBUTE_ENDPOINT_ID);
            // If we are not at the endpoint level, we do not need to go back
            if (endpoint_node.is_valid()) {
                auto current_node = attribute.parent();
                // Go back until we meet our endpoint
                while (endpoint_node != current_node) {
                    auto current_attribute_topic = attribute_value_to_topic(current_node);
                    // In case of error we don't go any further
                    if (current_attribute_topic.empty()) {
                        attribute_path.clear();
                        break;
                    }
                    // Emplace the current attribute topic at the beginning of the path
                    attribute_path.insert(0, current_attribute_topic);

                    current_node = current_node.parent();
                }
            }

            // Add a slash at the beginning of the path
            attribute_path.insert(0, "/");

            return fmt::format(MQTT_ATTRIBUTE_TOPIC, fmt::arg("command_class", command_class), fmt::arg("attribute_path", attribute_path), fmt::arg("attribute_name", attribute_name));

        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG, "Error while formatting attribute topic: %s", e.what());
            return "";
        }
    }

    std::string zpc_mqtt_attribute_registration::get_complete_attribute_topic(const attribute_store::attribute &attribute, attribute_store_node_value_state_t state) const
    {
        constexpr std::string_view topic = "{base_topic}{attribute_topic}{state}";

        auto base_topic      = zpc_mqtt::utils::get_base_topic_from_attribute(attribute);
        auto attribute_topic = get_attribute_topic(attribute);

        // Check if the base topic and attribute topic are valid
        // Error handling are done in their respective functions
        if (base_topic.empty() || attribute_topic.empty()) {
            return "";
        }

        auto state_str = (state == DESIRED_ATTRIBUTE) ? MQTT_DESIRED_KEYWORD : MQTT_REPORTED_KEYWORD;
        return fmt::format(topic, fmt::arg("base_topic", base_topic), fmt::arg("attribute_topic", attribute_topic), fmt::arg("state", state_str));
    }

    std::string zpc_mqtt_attribute_registration::attribute_value_to_topic(const attribute_store::attribute &attribute) const
    {
        try {
            auto current_attribute_name = mqtt_registration_map.at(attribute.type()).attribute_name;

            std::string current_attribute_value;

            if (!attribute.reported_exists()) {
                sl_log_warning(LOG_TAG,
                               "Parent attribute %s doesn't have a reported value. "
                               "Published MQTT topic will not contain any parent value.",
                               attribute.name_and_id().c_str());
                return current_attribute_value;
            }

            auto attribute_type = attribute.type();
            // Check if we have a custom encoder
            // In that case we let the function handles the conversion
            if (is_encoder_available(attribute_type)) {
                current_attribute_value = encoders.at(attribute_type)(attribute, REPORTED_ATTRIBUTE);
            } else {
                switch (attribute_store_get_storage_type(attribute_type)) {
                    case U8_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<uint8_t>());
                        break;
                    case U16_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<uint16_t>());
                        break;
                    case U32_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<uint32_t>());
                        break;
                    case U64_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<uint64_t>());
                        break;
                    case I8_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<int8_t>());
                        break;
                    case I16_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<int16_t>());
                        break;
                    case I32_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<int32_t>());
                        break;
                    case I64_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<int64_t>());
                        break;
                    case DOUBLE_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<double>());
                        break;
                    case FLOAT_STORAGE_TYPE:
                        current_attribute_value = fmt::format("{}", attribute.reported<float>());
                        break;
                    case C_STRING_STORAGE_TYPE:
                        current_attribute_value = attribute.reported<std::string>();
                        break;
                    case BYTE_ARRAY_STORAGE_TYPE:
                        current_attribute_value = Utils::byte_array_to_string(attribute.reported<std::vector<uint8_t>>());
                        break;
                    default:
                        sl_log_critical(LOG_TAG, "Unsupported value type for %s. Sending empty payload.", attribute.name_and_id().c_str());
                        return "";
                }
            }
            return fmt::format("{}/{}/", current_attribute_name, current_attribute_value);
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG,
                         "Error in attribute_value_to_topic: %s. Maybe you forget to "
                         "register this attribute to mqtt ?",
                         e.what());
        }
        return "";
    }

    std::string zpc_mqtt_attribute_registration::get_attribute_payload(const attribute_store::attribute &attribute, attribute_store_node_value_state_t state)
    {
        auto storage_type = attribute_store_get_storage_type(attribute.type());
        try {
            // Check if we have a custom encoder
            // In that case we let the function handles the conversion and return it
            if (is_encoder_available(attribute.type())) {
                nlohmann::json payload;
                payload[MQTT_PAYLOAD_VALUE_KEYWORD] = encoders.at(attribute.type())(attribute, state);
                return payload.dump();
            }

            // We need a special case for the float type to avoid it being casted to double
            // See https://github.com/nlohmann/json/issues/1109
            if (storage_type == FLOAT_STORAGE_TYPE) {
                using json = nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, float>;
                json payload;
                payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<float>(state);
                return payload.dump();
            }

            nlohmann::json payload;
            switch (storage_type) {
                case U8_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<uint8_t>(state);
                    break;
                case U16_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<uint16_t>(state);
                    break;
                case U32_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<uint32_t>(state);
                    break;
                case U64_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<uint64_t>(state);
                    break;
                case I8_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<int8_t>(state);
                    break;
                case I16_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<int16_t>(state);
                    break;
                case I32_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<int32_t>(state);
                    break;
                case I64_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<int64_t>(state);
                    break;
                case DOUBLE_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<double>(state);
                    break;
                case C_STRING_STORAGE_TYPE:
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = attribute.get<std::string>(state);
                    break;
                case BYTE_ARRAY_STORAGE_TYPE: {
                    auto byte_array                     = attribute.get<std::vector<uint8_t>>(state);
                    payload[MQTT_PAYLOAD_VALUE_KEYWORD] = Utils::byte_array_to_string(byte_array);
                    break;
                }
                default:
                    sl_log_critical(LOG_TAG, "Unsupported value type for %s. Sending empty payload", attribute.name_and_id().c_str());
            }
            return payload.dump();
        } catch (const std::exception &e) {
            sl_log_critical(LOG_TAG, "Failed to get value of %s (%s). Sending empty payload", attribute.name_and_id().c_str(), e.what());
            return "";
        }
    }

    void zpc_mqtt_attribute_registration::register_encoder(attribute_store_type_t attribute_type, const mqtt_value_encoder &encoder)
    {
        encoders[attribute_type] = encoder;
    }

    bool zpc_mqtt_attribute_registration::is_encoder_available(attribute_store_type_t attribute_type) const
    {
        return encoders.contains(attribute_type);
    }

}  // namespace zpc_mqtt