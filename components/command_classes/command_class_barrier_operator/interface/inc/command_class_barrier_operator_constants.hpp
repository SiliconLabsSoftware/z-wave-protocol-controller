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

#ifndef COMMAND_CLASS_BARRIER_OPERATOR_CONSTANTS_H
#define COMMAND_CLASS_BARRIER_OPERATOR_CONSTANTS_H

#include "command_class_barrier_operator_generated_types.hpp"

#include <cstddef>
#include <cstdint>

namespace zwave_command_class
{
    namespace command_class_barrier_operator_constants
    {
        constexpr uint8_t BARRIER_OPERATOR_CC_ID                    = 0x66;
        constexpr uint8_t BARRIER_OPERATOR_VERSION_1                = 1;
        constexpr uint8_t BARRIER_OPERATOR_TARGET_CLOSE             = 0x00;
        constexpr uint8_t BARRIER_OPERATOR_TARGET_OPEN              = 0xFF;
        constexpr uint8_t BARRIER_OPERATOR_TARGET_RESERVED          = 0x01;
        constexpr uint8_t BARRIER_OPERATOR_SUBSYSTEM_AUDIBLE        = 0x01;
        constexpr uint8_t BARRIER_OPERATOR_SUBSYSTEM_VISUAL         = 0x02;
        constexpr uint8_t BARRIER_OPERATOR_SUBSYSTEM_RESERVED       = 0x03;
        constexpr uint8_t BARRIER_OPERATOR_SUBSYSTEM_OFF            = 0x00;
        constexpr uint8_t BARRIER_OPERATOR_SUBSYSTEM_ON             = 0xFF;
        constexpr uint8_t BARRIER_OPERATOR_SUBSYSTEM_STATE_RESERVED = 0x01;
        constexpr uint8_t BARRIER_OPERATOR_STATE_MAXIMUM            = 0x63;
        constexpr uint8_t BARRIER_OPERATOR_STATE_MINIMUM_UNKNOWN    = 0xFC;
        constexpr uint8_t BARRIER_OPERATOR_STATE_RESERVED           = 0x64;
        constexpr size_t BARRIER_OPERATOR_CAPABILITIES_MAXIMUM_SIZE = 32;
    }  // namespace command_class_barrier_operator_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_BARRIER_OPERATOR_CONSTANTS_H
