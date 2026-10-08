
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
#include <string_view>

// Base class
#include "command_class_sensor_multilevel.hpp"
#include "command_class_sensor_multilevel_constants.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

#include "log.h"

namespace zwave_command_class
{
    using namespace command_class_sensor_multilevel_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_sensor_multilevel";

    command_class_sensor_multilevel::command_class_sensor_multilevel() : command_class_sensor_multilevel_attribute_store(), command_class_sensor_multilevel_mqtt() {}

    // Returns the first supported Sensor Type identifier > after_type, or 0 if none.
    // Bit 0 of Bit Mask 1 maps to Sensor Type 0x01 (CC:0031.05.02.11.001).
    static uint8_t next_supported_sensor_type(const std::vector<uint8_t> &bit_mask, uint8_t after_type)
    {
        for (size_t byte_idx = 0; byte_idx < bit_mask.size(); ++byte_idx) {
            for (uint8_t bit_idx = 0; bit_idx < 8; ++bit_idx) {
                const uint8_t type = static_cast<uint8_t>(byte_idx * 8 + bit_idx + SENSOR_TYPE_BIT_MASK_OFFSET);
                if (type > after_type && (bit_mask[byte_idx] & (1U << bit_idx)) != 0U) {
                    return type;
                }
            }
        }
        return 0;
    }

    // Lowest scale advertised in the Supported Scale Report bit mask, or DEFAULT_SCALE if none.
    static uint8_t first_supported_scale(uint8_t scale_bit_mask)
    {
        for (uint8_t scale = 0; scale < SCALE_BIT_MASK_WIDTH; ++scale) {
            if ((scale_bit_mask & (1U << scale)) != 0U) {
                return scale;
            }
        }
        return DEFAULT_SCALE;
    }

    // Returns true if the group has a desired sensor_type equal to sensor_type (i.e. the report answers our outstanding request).
    static bool matches_outstanding_request(attribute_store::attribute endpoint_node, attribute_store_type_t group_type, attribute_store_type_t sensor_type_attr, uint8_t sensor_type)
    {
        auto group_node = endpoint_node.child_by_type(group_type);
        if (!group_node.is_valid()) {
            return false;
        }
        auto type_node = group_node.child_by_type(sensor_type_attr);
        return type_node.is_valid() && type_node.desired_exists() && type_node.desired<uint8_t>() == sensor_type;
    }

    static void request_supported_scale(attribute_store::attribute endpoint_node, uint8_t sensor_type)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_get_scale_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_get_scale_group_attributes_t::sensor_type)).set_desired<uint8_t>(sensor_type);
        command_class_sensor_multilevel_core::start_group_resolution(group_node);
    }

    static void request_sensor_reading(attribute_store::attribute endpoint_node, uint8_t sensor_type, uint8_t scale)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::SENSOR_MULTILEVEL_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::sensor_type)).set_desired<uint8_t>(sensor_type);
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::scale)).set_desired<uint8_t>(scale);
        command_class_sensor_multilevel_core::start_group_resolution(group_node);
    }

    void command_class_sensor_multilevel::stop_get_group(attribute_store::attribute endpoint_node, attribute_store_type_t group_type)
    {
        auto group_node = endpoint_node.child_by_type(group_type);
        if (group_node.is_valid()) {
            command_class_sensor_multilevel_core::stop_group_resolution(group_node);
        }
    }

    void command_class_sensor_multilevel::require_scale_bit_mask(attribute_store::attribute endpoint_node, uint8_t sensor_type)
    {
        const auto group_type = static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_SCALE_REPORT_GROUP);
        const auto type_attr  = static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::sensor_type);
        auto group_node       = find_group_by_sensor_type(endpoint_node, group_type, type_attr, sensor_type);
        if (!group_node.is_valid()) {
            group_node = endpoint_node.add_node(group_type);
            group_node.emplace_node(type_attr).set_reported<uint8_t>(sensor_type);
        }
        cc_interview_require_attribute(group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::scale_bit_mask)));
    }

    void command_class_sensor_multilevel::require_sensor_value(attribute_store::attribute endpoint_node, uint8_t sensor_type)
    {
        const auto group_type = static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::SENSOR_MULTILEVEL_REPORT_GROUP);
        const auto type_attr  = static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_type);
        auto group_node       = find_group_by_sensor_type(endpoint_node, group_type, type_attr, sensor_type);
        if (!group_node.is_valid()) {
            group_node = endpoint_node.add_node(group_type);
            group_node.emplace_node(type_attr).set_reported<uint8_t>(sensor_type);
        }
        cc_interview_require_attribute(group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_value)));
    }

    // Walks the supported Sensor Types after after_type and issues the next missing query:
    // Supported Scale Get if scales are unknown, otherwise Get if no reading was received yet.
    void command_class_sensor_multilevel::advance_interview(attribute_store::attribute endpoint_node, uint8_t after_type)
    {
        const auto bit_mask = get_supported_sensor_bit_mask(endpoint_node);
        for (uint8_t next = next_supported_sensor_type(bit_mask, after_type); next != 0; next = next_supported_sensor_type(bit_mask, next)) {
            uint8_t scale_bit_mask = 0;
            if (!get_supported_scale_bit_mask_for_sensor_type(endpoint_node, next, scale_bit_mask)) {
                require_scale_bit_mask(endpoint_node, next);
                request_supported_scale(endpoint_node, next);
                return;
            }
            if (!has_report_for_sensor_type(endpoint_node, next)) {
                require_sensor_value(endpoint_node, next);
                request_sensor_reading(endpoint_node, next, first_supported_scale(scale_bit_mask));
                return;
            }
        }
    }

    void command_class_sensor_multilevel::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        if (supported_version >= SUPPORTED_GET_MIN_VERSION) {
            auto report_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_sensor_report_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_SENSOR_REPORT_GROUP));
            cc_interview_require_attribute(report_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_sensor_report_group_attributes_t::bit_mask)));
            auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_get_sensor_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_GET_SENSOR_GROUP));
            start_group_resolution(group_node, interview_resolution_options());
        } else {
            // Versions 1-4: Get has no fields, the node answers with its default Sensor Type.
            auto report_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::SENSOR_MULTILEVEL_REPORT_GROUP));
            cc_interview_require_attribute(report_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_value)));
            auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::SENSOR_MULTILEVEL_GET_GROUP));
            start_group_resolution(group_node, interview_resolution_options());
        }
    }

    sl_status_t command_class_sensor_multilevel::on_sensor_multilevel_supported_sensor_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_sensor_multilevel_attribute_map_t payload)
    {
        advance_interview(endpoint, 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_multilevel::on_sensor_multilevel_supported_scale_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_sensor_multilevel_attribute_map_t payload)
    {
        uint8_t sensor_type = 0;
        sensor_type         = get_value_or_default(payload, "sensor_type", sensor_type);

        // Only a report for the Sensor Type we asked for advances the interview.
        if (!matches_outstanding_request(endpoint,
                                         static_cast<attribute_store_type_t>(sensor_multilevel_supported_get_scale_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE_GROUP),
                                         static_cast<attribute_store_type_t>(sensor_multilevel_supported_get_scale_group_attributes_t::sensor_type),
                                         sensor_type)) {
            return SL_STATUS_OK;
        }

        stop_get_group(endpoint, static_cast<attribute_store_type_t>(sensor_multilevel_supported_get_scale_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE_GROUP));

        uint8_t scale_bit_mask = 0;
        get_supported_scale_bit_mask_for_sensor_type(endpoint, sensor_type, scale_bit_mask);
        if (!has_report_for_sensor_type(endpoint, sensor_type)) {
            require_sensor_value(endpoint, sensor_type);
            request_sensor_reading(endpoint, sensor_type, first_supported_scale(scale_bit_mask));
        } else {
            advance_interview(endpoint, sensor_type);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_multilevel::on_sensor_multilevel_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_sensor_multilevel_attribute_map_t payload)
    {
        uint8_t sensor_type = 0;
        sensor_type         = get_value_or_default(payload, "sensor_type", sensor_type);

        const auto get_group_type = static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::SENSOR_MULTILEVEL_GET_GROUP);

        // Versions 1-4: Get carries no Sensor Type, so any report completes it.
        if (endpoint_supported_version(endpoint) < SUPPORTED_GET_MIN_VERSION) {
            stop_get_group(endpoint, get_group_type);
            return SL_STATUS_OK;
        }

        // Only a report for the Sensor Type we asked for advances the interview.
        if (!matches_outstanding_request(endpoint, get_group_type, static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::sensor_type), sensor_type)) {
            return SL_STATUS_OK;
        }

        stop_get_group(endpoint, get_group_type);
        advance_interview(endpoint, sensor_type);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_multilevel::on_sensor_multilevel_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto sensor_type_node = group_node.child_by_type(static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::sensor_type));
        if (endpoint_supported_version(group_node.parent()) < SUPPORTED_GET_MIN_VERSION || !sensor_type_node.is_valid() || !sensor_type_node.desired_exists()) {
            return frame_generator->generate_no_args_frame();
        }

        frame_generator->add_raw_byte(sensor_type_node.desired<uint8_t>());

        auto scale_node = group_node.child_by_type(static_cast<attribute_store_type_t>(sensor_multilevel_get_group_attributes_t::scale));
        sensor_multilevel_get_properties1_t properties1;
        properties1.value                             = 0;
        properties1.flags.sensor_multilevel_get_scale = (scale_node.is_valid() && scale_node.desired_exists()) ? scale_node.desired<uint8_t>() : DEFAULT_SCALE;
        frame_generator->add_raw_byte(properties1.value);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_sensor_multilevel::on_sensor_multilevel_supported_get_scale_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto sensor_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_get_scale_group_attributes_t::sensor_type));
        if (!sensor_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_raw_byte(sensor_type_node.desired<uint8_t>());

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
