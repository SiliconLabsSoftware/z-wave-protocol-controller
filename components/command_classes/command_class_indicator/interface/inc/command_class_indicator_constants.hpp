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

#ifndef COMMAND_CLASS_INDICATOR_CONSTANTS_H
#define COMMAND_CLASS_INDICATOR_CONSTANTS_H

#include <array>

#include "command_class_indicator_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_indicator_constants
    {
        enum class indicator_id : uint8_t {
            NA            = 0x00,
            NODE_IDENTIFY = 0x50,
        };

        enum class property_id : uint8_t {
            MULTILEVEL             = 0x01,
            BINARY                 = 0x02,
            ON_OFF_PERIOD          = 0x03,
            ON_OFF_CYCLES          = 0x04,
            ONE_TIME_ON_OFF_PERIOD = 0x05,
        };

        // Indicator 0 Value in Indicator Report: for V2+ object-based reporting,
        // this legacy V1 field is reported as 0x00.
        constexpr uint8_t INDICATOR_0_VALUE_V2_PLUS = 0x00;

        // Indicator Supported Report fallback for unsupported Indicator ID
        // (v2 spec): all fields are set to 0x00.
        constexpr uint8_t PROPERTY_SUPPORTED_BITMASK_LENGTH_NONE = 0x00;

        // Indicator Report fallback for unsupported Indicator ID (v2 spec):
        // one object with Property ID = 0x00 and Value = 0x00.
        constexpr uint8_t UNSUPPORTED_INDICATOR_OBJECT_COUNT = 0x01;
        constexpr uint8_t UNSUPPORTED_INDICATOR_PROPERTY_ID  = 0x00;
        constexpr uint8_t UNSUPPORTED_INDICATOR_VALUE        = 0x00;

        // Missing/non-specified property values default to 0x00 per spec.
        constexpr uint8_t PROPERTY_VALUE_DEFAULT = 0x00;

        // Indicator Description Report v4: Description Length = 0 means no description.
        constexpr uint8_t INDICATOR_DESCRIPTION_LENGTH_NONE = 0x00;

        // Mandatory properties for Node Identify per DT:00.12.0004.1:
        // Property IDs 0x03 (On/Off Period), 0x04 (On/Off Cycles),
        // 0x05 (One-time On/Off Period). Bit 0 MUST be zero per spec.
        constexpr uint8_t NODE_IDENTIFY_SUPPORTED_PROPERTIES_MASK = 0x38;
        constexpr uint8_t NODE_IDENTIFY_BITMASK_LENGTH            = 1;

        constexpr std::array<property_id, 3> NODE_IDENTIFY_PROPERTIES = {
          property_id::ON_OFF_PERIOD,
          property_id::ON_OFF_CYCLES,
          property_id::ONE_TIME_ON_OFF_PERIOD,
        };
    }  // namespace command_class_indicator_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_INDICATOR_CONSTANTS_H
