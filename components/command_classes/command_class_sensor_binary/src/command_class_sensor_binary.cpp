
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
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_sensor_binary";

    static std::optional<uint8_t> next_supported_sensor_type(const sensor_binary_supported_sensor_report_bit_mask_t &bit_mask, uint8_t from)
    {
        for (size_t byte_idx = 0; byte_idx < bit_mask.size(); ++byte_idx) {
            for (uint8_t bit_idx = 0; bit_idx < 8; ++bit_idx) {
                uint8_t sensor_type = static_cast<uint8_t>((byte_idx * 8) + bit_idx);
                if (sensor_type == 0 || sensor_type < from) {
                    continue;
                }
                if (((bit_mask[byte_idx] >> bit_idx) & 0x01U) != 0U) {
                    return sensor_type;
                }
            }
        }
        return std::nullopt;
    }

    command_class_sensor_binary::command_class_sensor_binary() {}

    void command_class_sensor_binary::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        if (supported_version >= 2) {
            auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP));
            cc_interview_require_attribute(report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_get_sensor_group_attributes_t::SENSOR_BINARY_SUPPORTED_GET_SENSOR_GROUP)));
        } else {
            auto report_group = find_or_create_report_group_by_sensor_type(endpoint_node, 0x00);
            cc_interview_require_attribute(report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP)));
        }
    }

    sl_status_t command_class_sensor_binary::on_sensor_binary_supported_sensor_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_sensor_binary_attribute_map_t payload)
    {
        (void)connection_info;

        sensor_binary_supported_sensor_report_bit_mask_t bit_mask = {};
        bit_mask                                                  = get_value_or_default(payload, "bit_mask", bit_mask);

        for (size_t byte_idx = 0; byte_idx < bit_mask.size(); ++byte_idx) {
            for (uint8_t bit_idx = 0; bit_idx < 8; ++bit_idx) {
                uint8_t sensor_type = static_cast<uint8_t>((byte_idx * 8) + bit_idx);
                if (sensor_type == 0) {
                    continue;
                }
                if (((bit_mask[byte_idx] >> bit_idx) & 0x01U) != 0U) {
                    auto report_group = find_or_create_report_group_by_sensor_type(endpoint, sensor_type);
                    cc_interview_require_attribute(report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)));
                }
            }
        }

        auto first = next_supported_sensor_type(bit_mask, 1);
        if (!first.has_value()) {
            sl_log_debug(LOG_TAG.data(), "No supported sensor types found in bit mask");
            return SL_STATUS_OK;
        }

        auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
        auto type_node = get_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
        type_node.set_desired<uint8_t>(first.value());
        start_group_resolution(get_group);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_binary::on_sensor_binary_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_sensor_binary_attribute_map_t payload)
    {
        (void)connection_info;
        (void)payload;

        auto supported_report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP));
        if (!supported_report_group.is_valid()) {
            return SL_STATUS_OK;
        }

        auto mask_node = supported_report_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask));
        if (!mask_node.is_valid() || !mask_node.reported_exists()) {
            return SL_STATUS_OK;
        }

        const auto bit_mask = mask_node.reported<sensor_binary_supported_sensor_report_bit_mask_t>();

        auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
        if (!get_group.is_valid()) {
            return SL_STATUS_OK;
        }

        auto type_node = get_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
        if (!type_node.is_valid() || !type_node.desired_exists()) {
            return SL_STATUS_OK;
        }

        const uint8_t current_type = type_node.desired<uint8_t>();
        auto next                  = next_supported_sensor_type(bit_mask, current_type + 1);
        if (!next.has_value()) {
            return SL_STATUS_OK;
        }

        type_node.set_desired<uint8_t>(next.value());
        start_group_resolution(get_group);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_binary::on_sensor_binary_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
        if (type_node.desired_exists()) {
            frame_generator->add_value(type_node, DESIRED_ATTRIBUTE);
        }

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class