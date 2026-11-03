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

#ifndef COMMAND_CLASS_USER_CODE_CONSTANTS_H
#define COMMAND_CLASS_USER_CODE_CONSTANTS_H

#include <cstdint>

#include "command_class_user_code_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_user_code_constants
    {
        constexpr uint8_t USER_CODE_LENGTH_MIN  = 4;
        constexpr uint8_t USER_CODE_LENGTH_MAX  = 10;
        constexpr uint8_t ASCII_DIGIT_MIN       = 0x30;
        constexpr uint8_t ASCII_DIGIT_MAX       = 0x39;
        constexpr uint8_t ASCII_PRINTABLE_MIN   = 0x20;
        constexpr uint8_t ASCII_PRINTABLE_MAX   = 0x7E;
        constexpr uint8_t STATUS_AVAILABLE      = 0x00;
        constexpr uint8_t STATUS_OCCUPIED       = 0x01;
        constexpr uint8_t STATUS_RESERVED       = 0x02;
        constexpr uint8_t STATUS_PASSAGE        = 0x04;
        constexpr uint8_t EXTENDED_BLOCK_HEADER = 4;
        constexpr uint8_t LENGTH_NIBBLE_MASK    = 0x0F;
        constexpr uint8_t FIRST_USER_IDENTIFIER = 1;
        constexpr uint8_t ERASED_CODE_LENGTH    = 4;
    }  // namespace command_class_user_code_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CODE_CONSTANTS_H
