
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

#ifndef COMMAND_CLASS_NODE_NAMING_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_NODE_NAMING_ATTRIBUTE_STORE_H

#include <vector>

#include "command_class_node_naming_core.hpp"
#include "command_class_node_naming_types.hpp"  // command_class_node_naming_types

namespace zwave_command_class
{

    class command_class_node_naming_attribute_store : public virtual command_class_node_naming_core
    {

        public:
            command_class_node_naming_attribute_store();
            ~command_class_node_naming_attribute_store() = default;

            static sl_status_t store_node_name(attribute_store::attribute endpoint_node, uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static sl_status_t store_node_location(attribute_store::attribute endpoint_node, uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static void load_node_name(attribute_store::attribute endpoint_node, uint8_t &char_presentation, std::vector<uint8_t> &characters);
            static void load_node_location(attribute_store::attribute endpoint_node, uint8_t &char_presentation, std::vector<uint8_t> &characters);

        private:
            static bool is_supported_char_presentation(uint8_t char_presentation);
            static sl_status_t validate_naming_characters(uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static sl_status_t store_naming_value(attribute_store::attribute endpoint_node, attribute_store_type_t report_group_type, attribute_store_type_t char_presentation_type, attribute_store_type_t characters_type, uint8_t char_presentation, const std::vector<uint8_t> &characters);
            static void load_naming_value(attribute_store::attribute endpoint_node, attribute_store_type_t report_group_type, attribute_store_type_t char_presentation_type, attribute_store_type_t characters_type, uint8_t &char_presentation, std::vector<uint8_t> &characters);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_NODE_NAMING_ATTRIBUTE_STORE_H
