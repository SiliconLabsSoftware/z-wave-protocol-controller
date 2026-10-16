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

        // CC:008A.03.02.11.008 Hour is 5 bits. 255 means the time has not been set and cannot be encoded in those bits.
        constexpr uint8_t hour_field_all_ones = 0x1F;
        constexpr uint8_t local_time_unset    = 255;
        constexpr uint8_t hour_local_time_max = 23;
        constexpr uint8_t minute_max          = 59;
        constexpr uint8_t second_max          = 59;

        constexpr uint8_t month_min = 1;
        constexpr uint8_t month_max = 12;
        constexpr uint8_t day_min   = 1;
        constexpr uint8_t day_max   = 31;

        constexpr uint8_t time_offset_min_version = 2;
        constexpr uint8_t time_source_min_version = 3;
        constexpr uint8_t hour_tzo_max            = 14;
        constexpr uint8_t dst_month_max           = 12;
        constexpr uint8_t dst_day_max             = 31;
        constexpr uint8_t dst_hour_max            = 23;
        constexpr uint8_t minute_offset_dst_max   = 127;
        constexpr uint8_t sign_plus               = 0;
        constexpr uint8_t sign_minus              = 1;
        constexpr uint8_t sign_bit_shift          = 7;
    }  // namespace command_class_time_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_TIME_CONSTANTS_H
