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

#ifndef COMMAND_CLASS_PROTOCOL_LR_TYPES_H
#define COMMAND_CLASS_PROTOCOL_LR_TYPES_H

#include <stdint.h>

#include <variant>
#include <vector>
#include <string>
#include <map>

namespace zwave_command_class
{
    namespace command_class_protocol_lr_types
    {

        // Flexible value type that can hold int, vector, or string
        using command_class_protocol_lr_flexible_map_value_t = std::variant<int, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string>;

        // Map type that can hold different value types
        using command_class_protocol_lr_attribute_map_t = std::map<std::string, command_class_protocol_lr_flexible_map_value_t>;

    }  // namespace command_class_protocol_lr_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_PROTOCOL_LR_TYPES_H
