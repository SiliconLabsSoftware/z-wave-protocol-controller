
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

#include <fmt/base.h>
#include <fmt/format.h>
#include <optional>
#include <string_view>

#include "command_class_sensor_binary.hpp"
#include "command_class_sensor_binary_constants.hpp"
#include "log.h"

namespace zwave_command_class
{

    using namespace command_class_sensor_binary_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_sensor_binary";

    static std::optional<uint8_t> next_supported_sensor_type(const sensor_binary_supported_sensor_report_bit_mask_t &bit_mask, uint8_t from)
    {
        if (from == 0) {
            return std::nullopt;
        }
        for (size_t byte_idx = 0; byte_idx < bit_mask.size(); ++byte_idx) {
            for (uint8_t bit_idx = 0; bit_idx < 8; ++bit_idx) {
                uint8_t sensor_type = static_cast<uint8_t>((byte_idx * 8) + bit_idx + SENSOR_TYPE_BIT_MASK_OFFSET);
                if (sensor_type < from) {
                    continue;
                }
                if (((bit_mask[byte_idx] >> bit_idx) & 0x01U) != 0U) {
                    return sensor_type;
                }
            }
        }
        return std::nullopt;
    }

    static bool matches_outstanding_request(attribute_store::attribute endpoint_node, attribute_store_type_t group_type, attribute_store_type_t sensor_type_attr, uint8_t sensor_type)
    {
        auto group_node = endpoint_node.child_by_type(group_type);
        if (!group_node.is_valid()) {
            return false;
        }
        auto type_node = group_node.child_by_type(sensor_type_attr);
        if (!type_node.is_valid() || !type_node.desired_exists()) {
            return true;
        }
        return type_node.desired<uint8_t>() == sensor_type;
    }

    static void stop_get_group(attribute_store::attribute endpoint_node, attribute_store_type_t group_type)
    {
        auto group_node = endpoint_node.child_by_type(group_type);
        if (group_node.is_valid()) {
            command_class_sensor_binary_core::stop_group_resolution(group_node);
        }
    }

    static void request_sensor_reading(attribute_store::attribute endpoint_node, uint8_t sensor_type, const group_resolution_options &options)
    {
        auto get_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
        auto type_node = get_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
        type_node.set_desired<uint8_t>(sensor_type);
        command_class_sensor_binary_core::start_group_resolution(get_group, options);
    }

    void command_class_sensor_binary::require_sensor_value(attribute_store::attribute endpoint_node, uint8_t sensor_type)
    {
        if (has_report_for_sensor_type(endpoint_node, sensor_type)) {
            return;
        }
        auto report_group = find_or_create_report_group_by_sensor_type(endpoint_node, sensor_type);
        cc_interview_require_attribute(report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)));
    }

    bool command_class_sensor_binary::sensor_type_completes_interview_step(attribute_store::attribute endpoint_node, uint8_t sensor_type, const sensor_binary_supported_sensor_report_bit_mask_t &bit_mask)
    {
        if (!has_report_for_sensor_type(endpoint_node, sensor_type)) {
            return false;
        }
        for (auto prior = next_supported_sensor_type(bit_mask, 1); prior.has_value() && prior.value() < sensor_type; prior = next_supported_sensor_type(bit_mask, static_cast<uint8_t>(prior.value() + 1))) {
            if (!has_report_for_sensor_type(endpoint_node, prior.value())) {
                return false;
            }
        }
        return true;
    }

    void command_class_sensor_binary::advance_interview(attribute_store::attribute endpoint_node, uint8_t after_type)
    {
        const auto bit_mask = get_supported_sensor_bit_mask(endpoint_node);
        for (uint8_t after = after_type;;) {
            auto next = next_supported_sensor_type(bit_mask, after == 0 ? 1 : static_cast<uint8_t>(after + 1));
            if (!next.has_value()) {
                return;
            }
            if (!has_report_for_sensor_type(endpoint_node, next.value())) {
                require_sensor_value(endpoint_node, next.value());
                request_sensor_reading(endpoint_node, next.value(), interview_resolution_options());
                return;
            }
            after = next.value();
        }
    }

    command_class_sensor_binary::command_class_sensor_binary() {}

    void command_class_sensor_binary::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        if (supported_version >= 2) {
            auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP));
            cc_interview_require_attribute(report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_get_sensor_group_attributes_t::SENSOR_BINARY_SUPPORTED_GET_SENSOR_GROUP)), interview_resolution_options());
        } else {
            auto report_group = find_or_create_report_group_by_sensor_type(endpoint_node, 0x00);
            cc_interview_require_attribute(report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP)), interview_resolution_options());
        }
    }

    sl_status_t command_class_sensor_binary::on_sensor_binary_supported_sensor_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_sensor_binary_attribute_map_t payload)
    {
        (void)connection_info;
        (void)payload;
        if (cc_interview_is_open(endpoint)) {
            advance_interview(endpoint, 0);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_binary::on_sensor_binary_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_sensor_binary_attribute_map_t payload)
    {
        (void)connection_info;

        sensor_binary_report_sensor_type_t sensor_type = 0;
        sensor_type                                    = get_value_or_default(payload, "sensor_type", sensor_type);

        const auto get_group_type   = static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP);
        const auto sensor_type_attr = static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type);

        // Version 1: Get carries no Sensor Type, so any report completes it.
        if (endpoint_supported_version(endpoint) < 2) {
            stop_get_group(endpoint, get_group_type);
            return SL_STATUS_OK;
        }

        if (!matches_outstanding_request(endpoint, get_group_type, sensor_type_attr, sensor_type)) {
            return SL_STATUS_OK;
        }

        stop_get_group(endpoint, get_group_type);

        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }

        const auto bit_mask = get_supported_sensor_bit_mask(endpoint);
        if (sensor_type_completes_interview_step(endpoint, sensor_type, bit_mask)) {
            advance_interview(endpoint, sensor_type);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_binary::on_sensor_binary_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto type_node = group_node.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
        if (endpoint_supported_version(group_node.parent()) < 2 || !type_node.is_valid() || !type_node.desired_exists()) {
            return frame_generator->generate_no_args_frame();
        }

        frame_generator->add_raw_byte(type_node.desired<uint8_t>());
        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
