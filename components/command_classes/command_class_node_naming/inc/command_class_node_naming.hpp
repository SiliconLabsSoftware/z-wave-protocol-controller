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

#ifndef COMMAND_CLASS_NODE_NAMING_H
#define COMMAND_CLASS_NODE_NAMING_H

#include <vector>

#include "command_class_node_naming_mqtt.hpp"
#include "command_class_node_naming_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_node_naming final : public command_class_node_naming_attribute_store, public command_class_node_naming_mqtt
    {

        public:
            command_class_node_naming();
            ~command_class_node_naming() = default;

            static sl_status_t store_node_name(attribute_store::attribute endpoint_node, uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static sl_status_t store_node_location(attribute_store::attribute endpoint_node, uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static void load_node_name(attribute_store::attribute endpoint_node, uint8_t &char_presentation, std::vector<uint8_t> &characters);
            static void load_node_location(attribute_store::attribute endpoint_node, uint8_t &char_presentation, std::vector<uint8_t> &characters);
            static sl_status_t validate_naming_characters(uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static command_class_node_naming_attribute_map_t make_report_attribute_map(uint8_t char_presentation, const std::vector<uint8_t> &characters, const char *characters_key);

        private:
            static bool is_supported_char_presentation(uint8_t char_presentation);
            static sl_status_t store_naming_value(attribute_store::attribute endpoint_node, attribute_store_type_t report_group_type, attribute_store_type_t char_presentation_type, attribute_store_type_t characters_type, uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static void load_naming_value(attribute_store::attribute endpoint_node, attribute_store_type_t report_group_type, attribute_store_type_t char_presentation_type, attribute_store_type_t characters_type, uint8_t &char_presentation, std::vector<uint8_t> &characters);
            static void assemble_naming_report(zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame, uint8_t char_presentation, const std::vector<uint8_t> &characters);

            sl_status_t on_node_naming_node_name_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_node_naming_node_location_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_node_naming_node_name_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t attribute_map) override;
            sl_status_t on_node_naming_node_location_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_node_naming_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_NODE_NAMING_H
