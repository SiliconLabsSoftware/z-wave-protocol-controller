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

#ifndef COMMAND_CLASS_NODE_NAMING_CONSTANTS_H
#define COMMAND_CLASS_NODE_NAMING_CONSTANTS_H

#include <cstdint>

#include "command_class_node_naming_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_node_naming_constants
    {
        constexpr uint8_t character_field_max_length = 16;

        constexpr uint8_t char_presentation_standard_ascii = 0;
        constexpr uint8_t char_presentation_extended_ascii = 1;
        constexpr uint8_t char_presentation_utf16          = 2;

        constexpr const char *char_presentation_key  = "char__presentation";
        constexpr const char *node_name_char_key     = "node_name_char";
        constexpr const char *node_location_char_key = "node_location_char";
    }  // namespace command_class_node_naming_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_NODE_NAMING_CONSTANTS_H
