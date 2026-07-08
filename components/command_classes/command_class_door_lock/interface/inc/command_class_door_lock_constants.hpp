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

#ifndef COMMAND_CLASS_DOOR_LOCK_CONSTANTS_H
#define COMMAND_CLASS_DOOR_LOCK_CONSTANTS_H

#include "command_class_door_lock_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_door_lock_constants
    {
        constexpr uint8_t DOOR_LOCK_MODE_UNSECURED                                       = 0x00;
        constexpr uint8_t DOOR_LOCK_MODE_UNSECURED_WITH_TIMEOUT                          = 0x01;
        constexpr uint8_t DOOR_LOCK_MODE_UNSECURED_FOR_INSIDE_DOOR_HANDLES               = 0x10;
        constexpr uint8_t DOOR_LOCK_MODE_UNSECURED_FOR_INSIDE_DOOR_HANDLES_WITH_TIMEOUT  = 0x11;
        constexpr uint8_t DOOR_LOCK_MODE_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES              = 0x20;
        constexpr uint8_t DOOR_LOCK_MODE_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_WITH_TIMEOUT = 0x21;
        constexpr uint8_t DOOR_LOCK_MODE_UNKNOWN                                         = 0xFE;
        constexpr uint8_t DOOR_LOCK_MODE_SECURED                                         = 0xFF;

        constexpr uint8_t DOOR_LOCK_OPERATION_TYPE_CONSTANT = 0x01;
        constexpr uint8_t DOOR_LOCK_OPERATION_TYPE_TIMED    = 0x02;

        constexpr uint8_t DOOR_CONDITION_DOOR_OPEN     = 0x00;
        constexpr uint8_t DOOR_CONDITION_DOOR_CLOSED   = 0x01;
        constexpr uint8_t DOOR_CONDITION_BOLT_UNLOCKED = 0x00;
        constexpr uint8_t DOOR_CONDITION_BOLT_LOCKED   = 0x02;
        constexpr uint8_t DOOR_CONDITION_LATCH_OPEN    = 0x00;
        constexpr uint8_t DOOR_CONDITION_LATCH_CLOSED  = 0x04;
    }  // namespace command_class_door_lock_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_DOOR_LOCK_CONSTANTS_H
