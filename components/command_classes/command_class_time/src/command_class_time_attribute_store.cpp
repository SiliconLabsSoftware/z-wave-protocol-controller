
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

#include <string_view>

// Base class
#include "command_class_time.hpp"

#include "command_class_time_attribute_store.hpp"
#include "command_class_time_constants.hpp"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_time_attribute_store";

    namespace
    {
        using command_class_time_constants::dst_day_max;
        using command_class_time_constants::dst_hour_max;
        using command_class_time_constants::dst_month_max;
        using command_class_time_constants::hour_field_all_ones;
        using command_class_time_constants::hour_local_time_max;
        using command_class_time_constants::hour_tzo_max;
        using command_class_time_constants::local_time_unset;
        using command_class_time_constants::minute_max;
        using command_class_time_constants::second_max;
        using command_class_time_constants::sign_bit_shift;
        using command_class_time_constants::time_source_min_version;
        using command_class_time_constants::time_source_shift;
        using command_class_time_constants::time_source_zwave;

    }  // namespace

    command_class_time_attribute_store::command_class_time_attribute_store() {}

    template<typename Enum, typename T> void command_class_time_attribute_store::set_reported_child(attribute_store::attribute parent, Enum type, T value)
    {
        parent.emplace_node(static_cast<attribute_store_type_t>(type)).set_reported<T>(value);
    }

    uint8_t command_class_time_attribute_store::logical_flag(uint8_t masked_bit)
    {
        return static_cast<uint8_t>(masked_bit >> sign_bit_shift);
    }

    sl_status_t command_class_time_attribute_store::on_time_report_received_store(attribute_store::attribute endpoint_node, command_class_time_attribute_map_t attribute_map)
    {
        uint8_t hour        = 0;
        uint8_t minute      = 0;
        uint8_t second      = 0;
        uint8_t time_source = 0;
        uint8_t rtc_failure = 0;

        hour        = get_value_or_default(attribute_map, "hour_local_time", hour);
        minute      = get_value_or_default(attribute_map, "minute_local_time", minute);
        second      = get_value_or_default(attribute_map, "second_local_time", second);
        time_source = get_value_or_default(attribute_map, "time_source", time_source);
        rtc_failure = get_value_or_default(attribute_map, "rtc_failure", rtc_failure);

        // CC:008A.03.02.11.008 The 5-bit hour field cannot hold 255. All-ones means the time has not been set.
        if (hour == hour_field_all_ones) {
            hour = local_time_unset;
        } else if (hour > hour_local_time_max) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.03.02.11.008 Rejecting Time Report: hour %u", hour);
            return SL_STATUS_FAIL;
        }

        // CC:008A.03.02.11.009 / .010 Minute and second are 0..59, or 255 when the time has not been set.
        if ((minute != local_time_unset && minute > minute_max) || (second != local_time_unset && second > second_max)) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.03.02.11.009 Rejecting Time Report: minute=%u second=%u", minute, second);
            return SL_STATUS_FAIL;
        }

        // CC:008A.03.02.11 note: version 1 and 2 reserve the Time Source bits. 00 is assumed to be Z-Wave.
        const uint8_t version = endpoint_supported_version(endpoint_node);
        if (version > 0 && version < time_source_min_version) {
            time_source = time_source_zwave;
        } else {
            // The generated parser masks Time Source but leaves it in wire bits 5..6.
            time_source = static_cast<uint8_t>(time_source >> time_source_shift);
        }
        // The generated parser likewise leaves RTC Failure in wire bit 7.
        rtc_failure = logical_flag(rtc_failure);

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_report_group_attributes_t::TIME_REPORT_GROUP));
        set_reported_child(group_node, time_report_group_attributes_t::hour_local_time, hour);
        set_reported_child(group_node, time_report_group_attributes_t::time_source, time_source);
        set_reported_child(group_node, time_report_group_attributes_t::rtc_failure, rtc_failure);
        set_reported_child(group_node, time_report_group_attributes_t::minute_local_time, minute);
        set_reported_child(group_node, time_report_group_attributes_t::second_local_time, second);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time_attribute_store::on_date_report_received_store(attribute_store::attribute endpoint_node, command_class_time_attribute_map_t attribute_map)
    {
        date_report_year_t year   = 0;
        date_report_month_t month = 0;
        date_report_day_t day     = 0;

        year  = get_value_or_default(attribute_map, "year", year);
        month = get_value_or_default(attribute_map, "month", month);
        day   = get_value_or_default(attribute_map, "day", day);

        // CC:008A.01.04.11.003 / .005
        if (month < command_class_time_constants::month_min || month > command_class_time_constants::month_max || day < command_class_time_constants::day_min || day > command_class_time_constants::day_max) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.01.04.11.003 Rejecting Date Report: year=%u month=%u day=%u", year, month, day);
            return SL_STATUS_FAIL;
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(date_report_group_attributes_t::DATE_REPORT_GROUP));
        set_reported_child(group_node, date_report_group_attributes_t::year, year);
        set_reported_child(group_node, date_report_group_attributes_t::month, month);
        set_reported_child(group_node, date_report_group_attributes_t::day, day);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time_attribute_store::on_time_offset_report_received_store(attribute_store::attribute endpoint_node, command_class_time_attribute_map_t attribute_map)
    {
        uint8_t hour_tzo          = 0;
        uint8_t sign_tzo          = 0;
        uint8_t minute_tzo        = 0;
        uint8_t minute_offset_dst = 0;
        uint8_t sign_offset_dst   = 0;
        uint8_t month_start_dst   = 0;
        uint8_t day_start_dst     = 0;
        uint8_t hour_start_dst    = 0;
        uint8_t month_end_dst     = 0;
        uint8_t day_end_dst       = 0;
        uint8_t hour_end_dst      = 0;

        hour_tzo          = get_value_or_default(attribute_map, "hour_tzo", hour_tzo);
        sign_tzo          = get_value_or_default(attribute_map, "sign_tzo", sign_tzo);
        minute_tzo        = get_value_or_default(attribute_map, "minute_tzo", minute_tzo);
        minute_offset_dst = get_value_or_default(attribute_map, "minute_offset_dst", minute_offset_dst);
        sign_offset_dst   = get_value_or_default(attribute_map, "sign_offset_dst", sign_offset_dst);
        month_start_dst   = get_value_or_default(attribute_map, "month_start_dst", month_start_dst);
        day_start_dst     = get_value_or_default(attribute_map, "day_start_dst", day_start_dst);
        hour_start_dst    = get_value_or_default(attribute_map, "hour_start_dst", hour_start_dst);
        month_end_dst     = get_value_or_default(attribute_map, "month_end_dst", month_end_dst);
        day_end_dst       = get_value_or_default(attribute_map, "day_end_dst", day_end_dst);
        hour_end_dst      = get_value_or_default(attribute_map, "hour_end_dst", hour_end_dst);

        // CC:008A.02.05.11.003 Minute TZO MUST be 0..59. DST calendar fields follow the Time Offset Set ranges.
        if (minute_tzo > minute_max || month_start_dst > dst_month_max || day_start_dst > dst_day_max || hour_start_dst > dst_hour_max || month_end_dst > dst_month_max || day_end_dst > dst_day_max || hour_end_dst > dst_hour_max) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.02.05.11 Rejecting Time Offset Report");
            return SL_STATUS_FAIL;
        }
        if (hour_tzo > hour_tzo_max) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.02.05.12.002 Time Offset Report Hour TZO %u is outside 0..14", hour_tzo);
        }

        // The generated parser masks sign fields but leaves them in wire bit 7.
        sign_tzo        = logical_flag(sign_tzo);
        sign_offset_dst = logical_flag(sign_offset_dst);

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_report_group_attributes_t::TIME_OFFSET_REPORT_GROUP));
        set_reported_child(group_node, time_offset_report_group_attributes_t::hour_tzo, hour_tzo);
        set_reported_child(group_node, time_offset_report_group_attributes_t::sign_tzo, sign_tzo);
        set_reported_child(group_node, time_offset_report_group_attributes_t::minute_tzo, minute_tzo);
        set_reported_child(group_node, time_offset_report_group_attributes_t::minute_offset_dst, minute_offset_dst);
        set_reported_child(group_node, time_offset_report_group_attributes_t::sign_offset_dst, sign_offset_dst);
        set_reported_child(group_node, time_offset_report_group_attributes_t::month_start_dst, month_start_dst);
        set_reported_child(group_node, time_offset_report_group_attributes_t::day_start_dst, day_start_dst);
        set_reported_child(group_node, time_offset_report_group_attributes_t::hour_start_dst, hour_start_dst);
        set_reported_child(group_node, time_offset_report_group_attributes_t::month_end_dst, month_end_dst);
        set_reported_child(group_node, time_offset_report_group_attributes_t::day_end_dst, day_end_dst);
        set_reported_child(group_node, time_offset_report_group_attributes_t::hour_end_dst, hour_end_dst);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
