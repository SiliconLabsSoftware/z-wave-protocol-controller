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

#ifndef COMMAND_CLASS_SWITCH_MULTILEVEL_CONSTANTS_H
#define COMMAND_CLASS_SWITCH_MULTILEVEL_CONSTANTS_H

#include "command_class_switch_multilevel_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_switch_multilevel_constants
    {
        constexpr uint8_t SWITCH_TYPE_UNDEFINED       = 0x00;
        constexpr uint8_t SWITCH_TYPE_ON_OFF          = 0x01;
        constexpr uint8_t SWITCH_TYPE_UP_DOWN         = 0x02;
        constexpr uint8_t SWITCH_TYPE_OPEN_CLOSE      = 0x03;
        constexpr uint8_t SWITCH_TYPE_CCW_CW          = 0x04;
        constexpr uint8_t SWITCH_TYPE_LEFT_RIGHT      = 0x05;
        constexpr uint8_t SWITCH_TYPE_REVERSE_FORWARD = 0x06;
        constexpr uint8_t SWITCH_TYPE_PULL_PUSH       = 0x07;

        constexpr uint8_t MULTILEVEL_SWITCH_VALUE_OFF     = 0x00;
        constexpr uint8_t MULTILEVEL_SWITCH_VALUE_MAX     = 0x63;
        constexpr uint8_t MULTILEVEL_SWITCH_VALUE_UNKNOWN = 0xFE;
        constexpr uint8_t MULTILEVEL_SWITCH_VALUE_ON      = 0xFF;

        constexpr uint8_t UP_DOWN_UP        = 0x00;
        constexpr uint8_t UP_DOWN_DOWN      = 0x01;
        constexpr uint8_t UP_DOWN_RESERVED  = 0x02;
        constexpr uint8_t UP_DOWN_NO_MOTION = 0x03;

        constexpr uint8_t INC_DEC_INCREMENT  = 0x00;
        constexpr uint8_t INC_DEC_DECREMENT  = 0x01;
        constexpr uint8_t INC_DEC_RESERVED   = 0x02;
        constexpr uint8_t INC_DEC_NO_INC_DEC = 0x03;

        constexpr uint8_t DURATION_INSTANTLY       = 0x00;
        constexpr uint8_t DURATION_FACTORY_DEFAULT = 0xFF;
    }  // namespace command_class_switch_multilevel_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SWITCH_MULTILEVEL_CONSTANTS_H
