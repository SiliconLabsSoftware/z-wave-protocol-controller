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

#ifndef COMMAND_CLASS_THERMOSTAT_SETPOINT_CONSTANTS_H
#define COMMAND_CLASS_THERMOSTAT_SETPOINT_CONSTANTS_H

#include "command_class_thermostat_setpoint_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_thermostat_setpoint_constants
    {
        /** Z-Wave Thermostat Setpoint Value field length in bytes (Size field in Set/Report). Spec allows 1, 2 or 4. */
        enum class SetpointValueSize : uint8_t {
            OneByte   = 1,
            TwoBytes  = 2,
            FourBytes = 4,
        };

        constexpr uint8_t VALID_SIZE_BYTES[] = {1, 2, 4};
        constexpr size_t VALID_SIZE_COUNT    = sizeof(VALID_SIZE_BYTES) / sizeof(VALID_SIZE_BYTES[0]);

        constexpr uint8_t SCALE_CELSIUS    = 0;
        constexpr uint8_t SCALE_FAHRENHEIT = 1;

        inline bool is_valid_size(uint8_t size)
        {
            for (size_t i = 0; i < VALID_SIZE_COUNT; ++i) {
                if (VALID_SIZE_BYTES[i] == size) {
                    return true;
                }
            }
            return false;
        }

        inline bool is_valid_scale(uint8_t scale)
        {
            return scale == SCALE_CELSIUS || scale == SCALE_FAHRENHEIT;
        }
    }  // namespace command_class_thermostat_setpoint_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_THERMOSTAT_SETPOINT_CONSTANTS_H
