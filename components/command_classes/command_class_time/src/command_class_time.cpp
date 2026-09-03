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
#include <string_view>

// Base class
#include "command_class_time.hpp"
#include "command_class_time_constants.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "zwave_command_class_utils.hpp"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_time";

    namespace
    {
        using command_class_time_types::time_report_properties1_attribute_masks_t;

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

    sl_status_t command_class_time::on_time_get_support_requested_assemble_frame(const zwave_controller_connection_info_t * /*connection_info*/, command_class_time_attribute_map_t /*attribute_map*/, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
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

}  // namespace zwave_command_class
