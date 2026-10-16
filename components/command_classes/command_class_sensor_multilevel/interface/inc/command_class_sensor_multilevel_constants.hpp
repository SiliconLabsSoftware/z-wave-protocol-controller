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

#ifndef COMMAND_CLASS_SENSOR_MULTILEVEL_CONSTANTS_H
#define COMMAND_CLASS_SENSOR_MULTILEVEL_CONSTANTS_H

#include "command_class_sensor_multilevel_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_sensor_multilevel_constants
    {
        /** First version that defines Supported Sensor/Scale Get and the Sensor Type / Scale fields in Get. */
        constexpr uint8_t SUPPORTED_GET_MIN_VERSION = 5;

        /** Supported Sensor Report: bit 0 of Bit Mask 1 is Sensor Type 0x01 (CC:0031.05.02.11.001). */
        constexpr uint8_t SENSOR_TYPE_BIT_MASK_OFFSET = 1;

        /** Supported Scale Report: Scale Bit Mask is 4 bits wide (scales 0..3). */
        constexpr uint8_t SCALE_BIT_MASK_WIDTH = 4;

        /** Scale used in Get when the node has not advertised any supported scale. */
        constexpr uint8_t DEFAULT_SCALE = 0;
    }  // namespace command_class_sensor_multilevel_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SENSOR_MULTILEVEL_CONSTANTS_H
