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

#ifndef COMMAND_CLASS_MULTI_CMD_CONSTANTS_H
#define COMMAND_CLASS_MULTI_CMD_CONSTANTS_H

#include "command_class_multi_cmd_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_multi_cmd_constants
    {
        // CC + MULTI_CMD_ENCAP + Number of Commands
        constexpr uint16_t encap_header_size = 3;
        // Command Class + Command
        constexpr uint8_t inner_min_length = 2;
    }  // namespace command_class_multi_cmd_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_MULTI_CMD_CONSTANTS_H
