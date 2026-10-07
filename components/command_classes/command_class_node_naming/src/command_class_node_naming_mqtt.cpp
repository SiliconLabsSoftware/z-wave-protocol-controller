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

#include "command_class_node_naming.hpp"
#include "command_class_node_naming_constants.hpp"

#include "zpc_mqtt.hpp"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_node_naming_mqtt";

    using command_class_node_naming_constants::char_presentation_key;
    using command_class_node_naming_constants::char_presentation_standard_ascii;
    using command_class_node_naming_constants::node_location_char_key;
    using command_class_node_naming_constants::node_name_char_key;

    command_class_node_naming_mqtt::command_class_node_naming_mqtt()
    {
        mqtt_callback_map.insert({"NodeNamingNodeLocationSet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                      this->mqtt_on_node_naming_node_location_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"NodeNamingNodeLocationGet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                      this->mqtt_on_node_naming_node_location_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"NodeNamingNodeNameGet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                      this->mqtt_on_node_naming_node_name_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"NodeNamingNodeNameSet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                      this->mqtt_on_node_naming_node_name_set_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_node_naming_mqtt::mqtt_on_node_naming_node_name_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t char_presentation = char_presentation_standard_ascii;
        std::vector<uint8_t> characters;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("level").parse(char_presentation_key, char_presentation);
        parser.parse(node_name_char_key, characters);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        const sl_status_t status = command_class_node_naming::store_node_name(endpoint_node, char_presentation, characters);
        if (status != SL_STATUS_OK) {
            return status;
        }

        mqtt_publish_report(endpoint_node, command_class_node_naming_commands_t::COMMAND_CLASS_NODE_NAMING_NODE_NAMING_NODE_NAME_REPORT, command_class_node_naming::make_report_attribute_map(char_presentation, characters, node_name_char_key));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_node_naming_mqtt::mqtt_on_node_naming_node_location_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t char_presentation = char_presentation_standard_ascii;
        std::vector<uint8_t> characters;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("level").parse(char_presentation_key, char_presentation);
        parser.parse(node_location_char_key, characters);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        const sl_status_t status = command_class_node_naming::store_node_location(endpoint_node, char_presentation, characters);
        if (status != SL_STATUS_OK) {
            return status;
        }

        mqtt_publish_report(endpoint_node, command_class_node_naming_commands_t::COMMAND_CLASS_NODE_NAMING_NODE_NAMING_NODE_LOCATION_REPORT, command_class_node_naming::make_report_attribute_map(char_presentation, characters, node_location_char_key));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_node_naming_mqtt::mqtt_on_node_naming_node_name_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        uint8_t char_presentation = char_presentation_standard_ascii;
        std::vector<uint8_t> characters;
        command_class_node_naming::load_node_name(endpoint_node, char_presentation, characters);
        mqtt_publish_report(endpoint_node, command_class_node_naming_commands_t::COMMAND_CLASS_NODE_NAMING_NODE_NAMING_NODE_NAME_REPORT, command_class_node_naming::make_report_attribute_map(char_presentation, characters, node_name_char_key));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_node_naming_mqtt::mqtt_on_node_naming_node_location_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        uint8_t char_presentation = char_presentation_standard_ascii;
        std::vector<uint8_t> characters;
        command_class_node_naming::load_node_location(endpoint_node, char_presentation, characters);
        mqtt_publish_report(endpoint_node, command_class_node_naming_commands_t::COMMAND_CLASS_NODE_NAMING_NODE_NAMING_NODE_LOCATION_REPORT, command_class_node_naming::make_report_attribute_map(char_presentation, characters, node_location_char_key));
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
