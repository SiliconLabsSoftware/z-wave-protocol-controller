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

#include "ZW_classcmd.h"
#include "zwave_command_class_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_node_naming";

    using command_class_node_naming_constants::char_presentation_key;
    using command_class_node_naming_constants::char_presentation_standard_ascii;
    using command_class_node_naming_constants::character_field_max_length;
    using command_class_node_naming_constants::node_location_char_key;
    using command_class_node_naming_constants::node_name_char_key;

    command_class_node_naming::command_class_node_naming() {}

    void command_class_node_naming::assemble_naming_report(zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame, uint8_t char_presentation, const std::vector<uint8_t> &characters)
    {
        report_frame.add_raw_byte(static_cast<uint8_t>(char_presentation & 0x07));
        for (size_t i = 0; i < character_field_max_length; ++i) {
            report_frame.add_raw_byte(i < characters.size() ? characters[i] : 0);
        }
        frame = report_frame.generate_frame();
    }

    command_class_node_naming_attribute_map_t command_class_node_naming::make_report_attribute_map(uint8_t char_presentation, const std::vector<uint8_t> &characters, const char *characters_key)
    {
        command_class_node_naming_attribute_map_t attribute_map;
        attribute_map.insert({char_presentation_key, char_presentation});
        attribute_map.insert({characters_key, characters});
        return attribute_map;
    }

    sl_status_t
      command_class_node_naming::on_node_naming_node_name_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t /*attribute_map*/, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_zpc_endpoint_node(connection_info));
        uint8_t char_presentation = char_presentation_standard_ascii;
        std::vector<uint8_t> characters;
        command_class_node_naming_attribute_store::load_node_name(endpoint_node, char_presentation, characters);
        assemble_naming_report(report_frame, frame, char_presentation, characters);
        return SL_STATUS_OK;
    }

    sl_status_t
      command_class_node_naming::on_node_naming_node_location_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t /*attribute_map*/, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_zpc_endpoint_node(connection_info));
        uint8_t char_presentation = char_presentation_standard_ascii;
        std::vector<uint8_t> characters;
        command_class_node_naming_attribute_store::load_node_location(endpoint_node, char_presentation, characters);
        assemble_naming_report(report_frame, frame, char_presentation, characters);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_node_naming::on_node_naming_node_name_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t attribute_map)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_zpc_endpoint_node(connection_info));
        const uint8_t char_presentation = get_value_or_default<uint8_t>(attribute_map, char_presentation_key, char_presentation_standard_ascii);
        const auto characters           = get_value_or_default<std::vector<uint8_t>>(attribute_map, node_name_char_key, {});

        const sl_status_t status = command_class_node_naming_attribute_store::store_node_name(endpoint_node, char_presentation, characters);
        if (status != SL_STATUS_OK) {
            return status;
        }

        mqtt_publish_report(endpoint_node, command_class_node_naming_commands_t::COMMAND_CLASS_NODE_NAMING_NODE_NAMING_NODE_NAME_REPORT, make_report_attribute_map(char_presentation, characters, node_name_char_key));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_node_naming::on_node_naming_node_location_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t attribute_map)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_zpc_endpoint_node(connection_info));
        const uint8_t char_presentation = get_value_or_default<uint8_t>(attribute_map, char_presentation_key, char_presentation_standard_ascii);
        const auto characters           = get_value_or_default<std::vector<uint8_t>>(attribute_map, node_location_char_key, {});

        const sl_status_t status = command_class_node_naming_attribute_store::store_node_location(endpoint_node, char_presentation, characters);
        if (status != SL_STATUS_OK) {
            return status;
        }

        mqtt_publish_report(endpoint_node, command_class_node_naming_commands_t::COMMAND_CLASS_NODE_NAMING_NODE_NAMING_NODE_LOCATION_REPORT, make_report_attribute_map(char_presentation, characters, node_location_char_key));
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
