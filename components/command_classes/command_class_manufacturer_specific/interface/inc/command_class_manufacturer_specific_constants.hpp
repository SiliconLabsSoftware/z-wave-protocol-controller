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

#ifndef COMMAND_CLASS_MANUFACTURER_SPECIFIC_CONSTANTS_H
#define COMMAND_CLASS_MANUFACTURER_SPECIFIC_CONSTANTS_H

#include <cstddef>

#include "command_class_manufacturer_specific_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_manufacturer_specific_constants
    {
        constexpr uint8_t DEVICE_ID_TYPE_PSEUDO_RANDOM  = 0x02;
        constexpr std::size_t DEVICE_ID_DATA_MAX_LENGTH = 0x1F;
    }  // namespace command_class_manufacturer_specific_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_MANUFACTURER_SPECIFIC_CONSTANTS_H
