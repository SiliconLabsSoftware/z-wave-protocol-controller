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

#ifndef COMMAND_CLASS_TIME_CONSTANTS_H
#define COMMAND_CLASS_TIME_CONSTANTS_H

#include <cstdint>

namespace zwave_command_class
{
    namespace command_class_time_constants
    {
        constexpr uint8_t time_source_zwave = 0;  // 00b
        constexpr uint8_t time_source_gps   = 1;  // 01b
        constexpr uint8_t time_source_wifi  = 2;  // 10b
        constexpr uint8_t time_source_shift = 5;
    }  // namespace command_class_time_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_TIME_CONSTANTS_H
