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

#ifndef COMMAND_CLASS_CONFIGURATION_CONSTANTS_H
#define COMMAND_CLASS_CONFIGURATION_CONSTANTS_H

#include <array>
#include <cstdint>

#include "command_class_configuration_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_configuration_constants
    {
        constexpr uint16_t PARAMETER_NUMBER_PROBE_START = 0x0000;
        constexpr uint16_t NEXT_PARAMETER_TERMINATOR    = 0x0000;

        constexpr uint8_t PARAMETER_NUMBER_V1_MIN = 1;
        constexpr uint8_t PARAMETER_NUMBER_V1_MAX = 255;

        constexpr uint8_t SIZE_1_BYTE = 1;
        constexpr uint8_t SIZE_2_BYTE = 2;
        constexpr uint8_t SIZE_4_BYTE = 4;

        constexpr std::array<uint8_t, 3> PARAMETER_SIZES = {SIZE_1_BYTE, SIZE_2_BYTE, SIZE_4_BYTE};

        constexpr int64_t SCAN_PROBE_VALUE = 0;

        enum class format : uint8_t {
            SIGNED_INTEGER   = 0,
            UNSIGNED_INTEGER = 1,
            ENUMERATED       = 2,
            BIT_FIELD        = 3,
        };

        inline bool is_valid_size(uint8_t size)
        {
            return size == SIZE_1_BYTE || size == SIZE_2_BYTE || size == SIZE_4_BYTE;
        }
    }  // namespace command_class_configuration_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_CONFIGURATION_CONSTANTS_H
