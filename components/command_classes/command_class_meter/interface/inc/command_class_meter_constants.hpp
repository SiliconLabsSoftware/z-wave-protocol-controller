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

#ifndef COMMAND_CLASS_METER_CONSTANTS_H
#define COMMAND_CLASS_METER_CONSTANTS_H

#include "command_class_meter_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_meter_constants
    {
        constexpr uint8_t RATE_TYPE_DEFAULT = 0x00;
        constexpr uint8_t RATE_TYPE_IMPORT  = 0x01;
        constexpr uint8_t RATE_TYPE_EXPORT  = 0x02;

        constexpr uint8_t SUPPORTED_RATE_TYPE_IMPORT_ONLY       = 0x01;
        constexpr uint8_t SUPPORTED_RATE_TYPE_EXPORT_ONLY       = 0x02;
        constexpr uint8_t SUPPORTED_RATE_TYPE_IMPORT_AND_EXPORT = 0x03;

        constexpr uint8_t SCALE_BITS_EXTENDED = 0x07;
        constexpr uint8_t MAX_SCALE_V2        = 0x03;
        constexpr uint8_t MAX_SCALE_BYTE1     = 0x06;

        constexpr uint8_t METER_RESET_FLAG_MASK     = 0x80;
        constexpr uint8_t M_S_T_FLAG_MASK           = 0x80;
        constexpr uint8_t SCALE_SUPPORTED_0_MASK    = 0x7F;
        constexpr uint8_t RATE_TYPE_VALUE_MASK      = 0x03;
        constexpr uint8_t RATE_TYPE_SHIFT           = 0x05;
        constexpr uint8_t PRECISION_VALUE_MASK      = 0x07;
        constexpr uint8_t PRECISION_SHIFT           = 0x05;
        constexpr uint8_t SIZE_VALUE_MASK           = 0x07;
        constexpr uint8_t SCALE_BITS_10_VALUE_MASK  = 0x03;
        constexpr uint8_t SCALE_BITS_10_SHIFT       = 0x03;
        constexpr uint8_t SCALE_BIT_2_VALUE_MASK    = 0x01;
        constexpr uint8_t SCALE_BIT_2_SHIFT         = 0x07;
        constexpr uint8_t SCALE_HIGH_TO_SCALE_SHIFT = 0x02;

        inline uint8_t rate_type_from_masked(uint8_t masked_rate_type)
        {
            return static_cast<uint8_t>((masked_rate_type >> RATE_TYPE_SHIFT) & RATE_TYPE_VALUE_MASK);
        }

        inline uint8_t rate_type_to_masked(uint8_t logical_rate_type)
        {
            return static_cast<uint8_t>((logical_rate_type & RATE_TYPE_VALUE_MASK) << RATE_TYPE_SHIFT);
        }

        inline bool is_meter_reset_supported(uint8_t masked_meter_reset)
        {
            return (masked_meter_reset & METER_RESET_FLAG_MASK) != 0;
        }

        inline bool has_more_scale_types(uint8_t masked_m_s_t)
        {
            return (masked_m_s_t & M_S_T_FLAG_MASK) != 0;
        }

        inline uint8_t precision_from_masked(uint8_t masked_precision)
        {
            return static_cast<uint8_t>((masked_precision >> PRECISION_SHIFT) & PRECISION_VALUE_MASK);
        }

        inline uint8_t size_from_masked(uint8_t masked_size)
        {
            return static_cast<uint8_t>(masked_size & SIZE_VALUE_MASK);
        }

        // Report/Get scale fields are stored as masked values from the frame parser.
        inline uint8_t meter_scale_from_report_fields(uint8_t scale_bit_2, uint8_t scale_bits_10, uint8_t scale_2)
        {
            const uint8_t scale_low  = static_cast<uint8_t>((scale_bits_10 >> SCALE_BITS_10_SHIFT) & SCALE_BITS_10_VALUE_MASK);
            const uint8_t scale_high = static_cast<uint8_t>((scale_bit_2 >> SCALE_BIT_2_SHIFT) & SCALE_BIT_2_VALUE_MASK);
            const uint8_t scale_0_7  = static_cast<uint8_t>((scale_high << SCALE_HIGH_TO_SCALE_SHIFT) | scale_low);
            if (scale_0_7 < SCALE_BITS_EXTENDED) {
                return scale_0_7;
            }
            return static_cast<uint8_t>(SCALE_BITS_EXTENDED + scale_2);
        }

        inline void encode_scale_key_fields(uint8_t logical_scale, uint8_t &scale_bit_2, uint8_t &scale_bits_10, uint8_t &scale_2)
        {
            if (logical_scale < SCALE_BITS_EXTENDED) {
                scale_bits_10 = static_cast<uint8_t>((logical_scale & SCALE_BITS_10_VALUE_MASK) << SCALE_BITS_10_SHIFT);
                scale_bit_2   = static_cast<uint8_t>(((logical_scale >> SCALE_HIGH_TO_SCALE_SHIFT) & SCALE_BIT_2_VALUE_MASK) << SCALE_BIT_2_SHIFT);
                scale_2       = 0x00;
                return;
            }
            scale_bits_10 = static_cast<uint8_t>(SCALE_BITS_10_VALUE_MASK << SCALE_BITS_10_SHIFT);
            scale_bit_2   = static_cast<uint8_t>(SCALE_BIT_2_VALUE_MASK << SCALE_BIT_2_SHIFT);
            scale_2       = static_cast<uint8_t>(logical_scale - SCALE_BITS_EXTENDED);
        }

        inline void write_meter_get_scale(uint8_t logical_scale, uint8_t version, uint8_t &scale_bits, uint8_t &scale_2, bool &include_scale_2)
        {
            if (logical_scale < SCALE_BITS_EXTENDED) {
                scale_bits      = logical_scale;
                scale_2         = 0x00;
                include_scale_2 = (version >= 0x04);
                return;
            }
            scale_bits      = SCALE_BITS_EXTENDED;
            scale_2         = static_cast<uint8_t>(logical_scale - SCALE_BITS_EXTENDED);
            include_scale_2 = true;
        }
    }  // namespace command_class_meter_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_METER_CONSTANTS_H
