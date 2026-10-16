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

#include <chrono>
#include <ctime>
#include <algorithm>
#include <utility>
#include <string_view>

// Base class
#include "command_class_time.hpp"
#include "command_class_time_constants.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_tx_scheme_selector.h"
#include "zpc_attribute_store_network_helper.h"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_time";

    std::map<command_class_time::endpoint_key_t, command_class_time::monitored_endpoint_t> command_class_time::monitored_endpoints {};
    std::map<command_class_time::endpoint_key_t, std::time_t> command_class_time::last_time_get_requests {};
    std::mutex command_class_time::periodic_report_mutex {};
    struct timer_handle_t command_class_time::periodic_report_timer {nullptr};
    bool command_class_time::periodic_report_timer_started = false;
    bool command_class_time::daylight_saving_state_known   = false;
    int command_class_time::previous_daylight_saving_state = 0;

    namespace
    {
        using command_class_time_types::time_report_properties1_attribute_masks_t;

        constexpr std::time_t proactive_report_interval_seconds = 36 * 60 * 60;
        constexpr uint64_t periodic_report_check_interval_ms    = 60 * 60 * 1000;

        std::tm local_time_tm()
        {
            const auto now          = std::chrono::system_clock::now();
            const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm result {};
            localtime_r(&now_c, &result);
            return result;
        }

        sl_status_t validate_time_command(const zwave_controller_connection_info_t *connection_info)
        {
            // CC:008A.03.00.41.001: A supporting node MUST NOT accept Set/Report-Type commands of Time CC
            // unless received using its own highest (granted) security class.
            if (!command_class_utils::is_using_zpc_highest_security_class(connection_info)) {
                sl_log_debug(LOG_TAG.data(), "CC:008A.03.00.41.001 Rejecting command: not highest granted security class");
                return SL_STATUS_NOT_SUPPORTED;
            }

            return SL_STATUS_OK;
        }
    }  // namespace

    command_class_time::command_class_time() {}

    void command_class_time::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        zwave_home_id_t home_id         = 0;
        zwave_node_id_t node_id         = 0;
        zwave_endpoint_id_t endpoint_id = 0;
        if (attribute_store_network_helper_get_home_node_endpoint_from_node(endpoint_node, &home_id, &node_id, &endpoint_id) == SL_STATUS_OK) {
            const endpoint_key_t key {node_id, endpoint_id};
            std::lock_guard<std::mutex> lock(periodic_report_mutex);
            // A fresh process has no persisted send/request history. Queue an initial report on the next scan.
            monitored_endpoints[key] = {home_id, supported_version, 0};
        }
        schedule_periodic_report_timer();

        auto time_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_report_group_attributes_t::TIME_REPORT_GROUP));
        cc_interview_require_attribute(time_report.emplace_node(static_cast<attribute_store_type_t>(time_report_group_attributes_t::hour_local_time)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_get_group_attributes_t::TIME_GET_GROUP)), interview_resolution_options());

        auto date_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(date_report_group_attributes_t::DATE_REPORT_GROUP));
        cc_interview_require_attribute(date_report.emplace_node(static_cast<attribute_store_type_t>(date_report_group_attributes_t::year)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(date_get_group_attributes_t::DATE_GET_GROUP)), interview_resolution_options());

        // CC:008A.02.06.11.001 Time Offset Get exists from version 2.
        if (supported_version >= command_class_time_constants::time_offset_min_version) {
            auto offset_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_report_group_attributes_t::TIME_OFFSET_REPORT_GROUP));
            cc_interview_require_attribute(offset_report.emplace_node(static_cast<attribute_store_type_t>(time_offset_report_group_attributes_t::hour_tzo)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_get_group_attributes_t::TIME_OFFSET_GET_GROUP)), interview_resolution_options());
        }
    }

    void command_class_time::schedule_periodic_report_timer()
    {
        std::lock_guard<std::mutex> lock(periodic_report_mutex);
        if (!periodic_report_timer_started) {
            periodic_report_timer_started = true;
            timer_set(&periodic_report_timer, periodic_report_check_interval_ms, on_periodic_report_timer, nullptr);
        }
    }

    void command_class_time::on_periodic_report_timer(void * /*data*/)
    {
        const std::time_t now                   = std::time(nullptr);
        const std::tm local_now                 = local_time_tm();
        const int current_daylight_saving_state = local_now.tm_isdst > 0 ? 1 : 0;
        std::vector<std::pair<endpoint_key_t, monitored_endpoint_t>> due_endpoints;
        bool daylight_saving_changed = false;
        {
            std::lock_guard<std::mutex> lock(periodic_report_mutex);
            if (!daylight_saving_state_known) {
                previous_daylight_saving_state = current_daylight_saving_state;
                daylight_saving_state_known    = true;
            } else {
                daylight_saving_changed = previous_daylight_saving_state != current_daylight_saving_state;
            }

            for (const auto &[key, endpoint]: monitored_endpoints) {
                const auto request              = last_time_get_requests.find(key);
                const std::time_t last_request  = request == last_time_get_requests.end() ? 0 : request->second;
                const std::time_t last_activity = std::max(endpoint.last_report_time, last_request);
                if (daylight_saving_changed || last_activity == 0 || std::difftime(now, last_activity) >= proactive_report_interval_seconds) {
                    due_endpoints.emplace_back(key, endpoint);
                }
            }
        }

        bool all_sends_accepted = true;
        for (const auto &[key, endpoint]: due_endpoints) {
            if (attribute_store_network_helper_get_endpoint_node(endpoint.home_id, key.first, key.second) == ATTRIBUTE_STORE_INVALID_NODE) {
                std::lock_guard<std::mutex> lock(periodic_report_mutex);
                monitored_endpoints.erase(key);
                last_time_get_requests.erase(key);
                continue;
            }

            zwave_frame_generator_standalone report_frame;
            report_frame.add_header(COMMAND_CLASS_TIME, TIME_REPORT);

            const std::tm time_now = local_time_tm();
            const uint8_t hour     = static_cast<uint8_t>(time_now.tm_hour) & static_cast<uint8_t>(time_report_properties1_attribute_masks_t::hour_local_time_mask);
            uint8_t properties1    = hour;
            if (endpoint.version >= command_class_time_constants::time_source_min_version) {
                const uint8_t source = static_cast<uint8_t>((command_class_time_constants::time_source_wifi << command_class_time_constants::time_source_shift) & static_cast<uint8_t>(time_report_properties1_attribute_masks_t::time_source_mask));
                properties1          = static_cast<uint8_t>(properties1 | source);
            }
            report_frame.add_raw_byte(properties1);
            report_frame.add_raw_byte(static_cast<uint8_t>(time_now.tm_min));
            report_frame.add_raw_byte(static_cast<uint8_t>(time_now.tm_sec));
            const std::vector<uint8_t> frame = report_frame.generate_frame();

            zwave_controller_connection_info_t destination {};
            zwave_tx_scheme_get_node_connection_info(key.first, key.second, &destination);
            const sl_status_t send_status = command_class_utils::send_report(&destination, static_cast<uint16_t>(frame.size()), frame.data());
            if (send_status == SL_STATUS_OK) {
                std::lock_guard<std::mutex> lock(periodic_report_mutex);
                const auto registered_endpoint = monitored_endpoints.find(key);
                if (registered_endpoint != monitored_endpoints.end()) {
                    registered_endpoint->second.last_report_time = now;
                }
            } else {
                all_sends_accepted = false;
                sl_log_debug(LOG_TAG.data(), "Unable to queue proactive Time Report to node %u endpoint %u (status 0x%08X)", key.first, key.second, send_status);
            }
        }

        {
            std::lock_guard<std::mutex> lock(periodic_report_mutex);
            if (all_sends_accepted) {
                previous_daylight_saving_state = current_daylight_saving_state;
                daylight_saving_state_known    = true;
            }
            timer_set(&periodic_report_timer, periodic_report_check_interval_ms, on_periodic_report_timer, nullptr);
        }
    }

    sl_status_t command_class_time::on_time_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t /*attribute_map*/, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        if (connection_info != nullptr) {
            const endpoint_key_t requester {connection_info->remote.node_id, connection_info->remote.endpoint_id};
            std::lock_guard<std::mutex> lock(periodic_report_mutex);
            last_time_get_requests[requester] = std::time(nullptr);
        }
        const std::tm now = local_time_tm();

        const uint8_t hour = static_cast<uint8_t>(now.tm_hour) & static_cast<uint8_t>(time_report_properties1_attribute_masks_t::hour_local_time_mask);
        // CC:008A.03.02.11.004 host OS is the local/internet time source.
        const uint8_t time_source = static_cast<uint8_t>((command_class_time_constants::time_source_wifi << command_class_time_constants::time_source_shift) & static_cast<uint8_t>(time_report_properties1_attribute_masks_t::time_source_mask));
        // CC:008A.03.02.11.006 host time is available; RTC Failure stays 0.
        const uint8_t properties1 = static_cast<uint8_t>(hour | time_source);

        report_frame.add_raw_byte(properties1);
        report_frame.add_raw_byte(static_cast<uint8_t>(now.tm_min));
        report_frame.add_raw_byte(static_cast<uint8_t>(now.tm_sec));

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time::on_date_get_support_requested_assemble_frame(const zwave_controller_connection_info_t * /*connection_info*/, command_class_time_attribute_map_t /*attribute_map*/, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        const std::tm now   = local_time_tm();
        const uint16_t year = static_cast<uint16_t>(now.tm_year + 1900);

        report_frame.add_raw_byte(static_cast<uint8_t>((year >> 8) & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>(year & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>(now.tm_mon + 1));
        report_frame.add_raw_byte(static_cast<uint8_t>(now.tm_mday));

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time::on_time_offset_get_support_requested_assemble_frame(const zwave_controller_connection_info_t * /*connection_info*/, command_class_time_attribute_map_t /*attribute_map*/, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        for (int i = 0; i < 9; ++i) {
            report_frame.add_raw_byte(0);
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time::on_time_offset_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t /*attribute_map*/)
    {
        const sl_status_t status = validate_time_command(connection_info);
        if (status != SL_STATUS_OK) {
            return status;
        }

        return SL_STATUS_FAIL;
    }

    sl_status_t command_class_time::on_date_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t /*attribute_map*/)
    {
        const sl_status_t status = validate_time_command(connection_info);
        if (status != SL_STATUS_OK) {
            return status;
        }

        return SL_STATUS_FAIL;
    }

    sl_status_t command_class_time::on_time_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t /*attribute_map*/)
    {
        const sl_status_t status = validate_time_command(connection_info);
        if (status != SL_STATUS_OK) {
            return status;
        }

        return SL_STATUS_FAIL;
    }

    namespace
    {
        sl_status_t desired_u8(attribute_store::attribute group, attribute_store_type_t type, uint8_t &value)
        {
            auto node = group.emplace_node(type);
            if (!node.desired_exists()) {
                return SL_STATUS_NOT_READY;
            }
            value = node.desired<uint8_t>();
            return SL_STATUS_OK;
        }

        sl_status_t desired_u16(attribute_store::attribute group, attribute_store_type_t type, uint16_t &value)
        {
            auto node = group.emplace_node(type);
            if (!node.desired_exists()) {
                return SL_STATUS_NOT_READY;
            }
            value = node.desired<uint16_t>();
            return SL_STATUS_OK;
        }

        bool clock_component_in_range(uint8_t value, uint8_t max)
        {
            return value <= max;
        }

        bool sign_in_range(uint8_t sign)
        {
            return sign == command_class_time_constants::sign_plus || sign == command_class_time_constants::sign_minus;
        }
    }  // namespace

    sl_status_t command_class_time::on_time_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        uint8_t hour   = 0;
        uint8_t minute = 0;
        uint8_t second = 0;

        const sl_status_t hour_status   = desired_u8(group_node, static_cast<attribute_store_type_t>(time_set_group_attributes_t::hour), hour);
        const sl_status_t minute_status = desired_u8(group_node, static_cast<attribute_store_type_t>(time_set_group_attributes_t::minute), minute);
        const sl_status_t second_status = desired_u8(group_node, static_cast<attribute_store_type_t>(time_set_group_attributes_t::second), second);
        if (hour_status != SL_STATUS_OK) {
            return hour_status;
        }
        if (minute_status != SL_STATUS_OK) {
            return minute_status;
        }
        if (second_status != SL_STATUS_OK) {
            return second_status;
        }

        // CC:008A.03.09.11.004 / .005 / .006
        if (!clock_component_in_range(hour, command_class_time_constants::hour_local_time_max) || !clock_component_in_range(minute, command_class_time_constants::minute_max) || !clock_component_in_range(second, command_class_time_constants::second_max)) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.03.09.11.004 Rejecting Time Set: hour=%u minute=%u second=%u", hour, minute, second);
            return SL_STATUS_FAIL;
        }

        // CC:008A.03.09.11.002 Reserved bits MUST be 0.
        const uint8_t properties1 = static_cast<uint8_t>(hour & static_cast<uint8_t>(time_set_properties1_attribute_masks_t::hour_mask));
        frame_generator->add_raw_byte(properties1);
        frame_generator->add_raw_byte(minute);
        frame_generator->add_raw_byte(second);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_time::on_date_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        uint16_t year = 0;
        uint8_t month = 0;
        uint8_t day   = 0;

        const sl_status_t year_status  = desired_u16(group_node, static_cast<attribute_store_type_t>(date_set_group_attributes_t::year), year);
        const sl_status_t month_status = desired_u8(group_node, static_cast<attribute_store_type_t>(date_set_group_attributes_t::month), month);
        const sl_status_t day_status   = desired_u8(group_node, static_cast<attribute_store_type_t>(date_set_group_attributes_t::day), day);
        if (year_status != SL_STATUS_OK) {
            return year_status;
        }
        if (month_status != SL_STATUS_OK) {
            return month_status;
        }
        if (day_status != SL_STATUS_OK) {
            return day_status;
        }

        // CC:008A.03.08.11.003 / .005
        if (month < command_class_time_constants::month_min || month > command_class_time_constants::month_max || day < command_class_time_constants::day_min || day > command_class_time_constants::day_max) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.03.08.11.003 Rejecting Date Set: year=%u month=%u day=%u", year, month, day);
            return SL_STATUS_FAIL;
        }

        // CC:008A.03.08.11.001 Year MSB first.
        frame_generator->add_raw_byte(static_cast<uint8_t>((year >> 8) & 0xFF));
        frame_generator->add_raw_byte(static_cast<uint8_t>(year & 0xFF));
        frame_generator->add_raw_byte(month);
        frame_generator->add_raw_byte(day);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_time::on_time_offset_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

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

        const sl_status_t reads[] = {
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::hour_tzo), hour_tzo),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::sign_tzo), sign_tzo),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::minute_tzo), minute_tzo),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::minute_offset_dst), minute_offset_dst),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::sign_offset_dst), sign_offset_dst),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::month_start_dst), month_start_dst),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::day_start_dst), day_start_dst),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::hour_start_dst), hour_start_dst),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::month_end_dst), month_end_dst),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::day_end_dst), day_end_dst),
          desired_u8(group_node, static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::hour_end_dst), hour_end_dst),
        };
        for (const sl_status_t status: reads) {
            if (status != SL_STATUS_OK) {
                return status;
            }
        }

        // CC:008A.02.05.12.002 Hour TZO SHOULD be 0..14. CC:008A.02.05.11.003 Minute TZO MUST be 0..59.
        const bool fields_valid = hour_tzo <= command_class_time_constants::hour_tzo_max && clock_component_in_range(minute_tzo, command_class_time_constants::minute_max) && minute_offset_dst <= command_class_time_constants::minute_offset_dst_max && sign_in_range(sign_tzo)
                                  && sign_in_range(sign_offset_dst) && month_start_dst <= command_class_time_constants::dst_month_max && day_start_dst <= command_class_time_constants::dst_day_max && hour_start_dst <= command_class_time_constants::dst_hour_max
                                  && month_end_dst <= command_class_time_constants::dst_month_max && day_end_dst <= command_class_time_constants::dst_day_max && hour_end_dst <= command_class_time_constants::dst_hour_max;
        if (!fields_valid) {
            sl_log_debug(LOG_TAG.data(), "CC:008A.02.05.11 Rejecting Time Offset Set");
            return SL_STATUS_FAIL;
        }

        const uint8_t level  = static_cast<uint8_t>((hour_tzo & static_cast<uint8_t>(time_offset_set_level_attribute_masks_t::hour_tzo_mask)) | static_cast<uint8_t>(sign_tzo << command_class_time_constants::sign_bit_shift));
        const uint8_t level2 = static_cast<uint8_t>((minute_offset_dst & static_cast<uint8_t>(time_offset_set_level2_attribute_masks_t::minute_offset_dst_mask)) | static_cast<uint8_t>(sign_offset_dst << command_class_time_constants::sign_bit_shift));

        frame_generator->add_raw_byte(level);
        frame_generator->add_raw_byte(minute_tzo);
        frame_generator->add_raw_byte(level2);
        frame_generator->add_raw_byte(month_start_dst);
        frame_generator->add_raw_byte(day_start_dst);
        frame_generator->add_raw_byte(hour_start_dst);
        frame_generator->add_raw_byte(month_end_dst);
        frame_generator->add_raw_byte(day_end_dst);
        frame_generator->add_raw_byte(hour_end_dst);
        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
