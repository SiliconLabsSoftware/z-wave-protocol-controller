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

#ifndef COMMAND_CLASS_SENSOR_BINARY_CONSTANTS_H
#define COMMAND_CLASS_SENSOR_BINARY_CONSTANTS_H

#include "command_class_sensor_binary_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_sensor_binary_constants
    {
        // Sensor Type uses the bit position directly; bit 0 is reserved by CC:0030.02.01.11.001.
        constexpr uint8_t SENSOR_TYPE_BIT_MASK_OFFSET = 0;
    }  // namespace command_class_sensor_binary_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SENSOR_BINARY_CONSTANTS_H
