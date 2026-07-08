
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

// Base class
#include "command_class_switch_color.hpp"
#include "command_class_switch_color_constants.hpp"
#include "command_class_switch_color_core.hpp"

// Z-Wave
#include "ZW_classcmd.h"
#include "attribute_store_helper.h"
#include "attribute.hpp"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_color";

    // The frame parser reads the 2-byte color_component_mask as big-endian (MSB first),
    // but the Z-Wave spec defines byte 1 as components 0-7 and byte 2 as components 8-15.
    // Swap bytes so bit N corresponds to color component ID N.
    static uint16_t color_mask_to_native(uint16_t big_endian_mask)
    {
        return static_cast<uint16_t>((big_endian_mask >> 8) | (big_endian_mask << 8));
    }

    static std::optional<uint8_t> next_supported_color_component(uint16_t mask, uint8_t after)
    {
        for (uint8_t bit = after; bit < command_class_switch_color_constants::COLOR_COMPONENT_MASK_BITS; ++bit) {
            if ((mask & (1U << bit)) != 0U) {
                return bit;
            }
        }
        return std::nullopt;
    }

    command_class_switch_color::command_class_switch_color() {}

    void command_class_switch_color::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)supported_version;
        auto supported_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_supported_get_group_attributes_t::SWITCH_COLOR_SUPPORTED_GET_GROUP));
        start_group_resolution(supported_get_node);
    }

    sl_status_t command_class_switch_color::on_switch_color_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_switch_color_attribute_map_t payload)
    {
        (void)connection_info;

        switch_color_supported_report_color_component_mask_t raw_mask = 0;
        raw_mask                                                      = get_value_or_default(payload, "color_component_mask", raw_mask);
        const uint16_t mask                                           = color_mask_to_native(raw_mask);

        auto first_component = next_supported_color_component(mask, 0);
        if (!first_component.has_value()) {
            sl_log_debug(LOG_TAG.data(), "No supported color components found in mask 0x%04X", mask);
            return SL_STATUS_OK;
        }

        auto get_group_node          = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_color_get_group_attributes_t::SWITCH_COLOR_GET_GROUP));
        auto color_component_id_node = get_group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_get_group_attributes_t::color_component_id));
        color_component_id_node.set_desired<uint8_t>(first_component.value());
        start_group_resolution(get_group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_color::on_switch_color_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_switch_color_attribute_map_t payload)
    {
        (void)connection_info;
        (void)payload;

        auto report_group_node = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_color_supported_report_group_attributes_t::SWITCH_COLOR_SUPPORTED_REPORT_GROUP));
        if (!report_group_node.is_valid()) {
            return SL_STATUS_OK;
        }

        auto mask_node = report_group_node.child_by_type(static_cast<attribute_store_type_t>(switch_color_supported_report_group_attributes_t::color_component_mask));
        if (!mask_node.is_valid() || !mask_node.reported_exists()) {
            return SL_STATUS_OK;
        }

        const uint16_t mask = color_mask_to_native(mask_node.reported<uint16_t>());

        auto get_group_node          = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_color_get_group_attributes_t::SWITCH_COLOR_GET_GROUP));
        auto color_component_id_node = get_group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_get_group_attributes_t::color_component_id));
        if (!color_component_id_node.desired_exists()) {
            return SL_STATUS_OK;
        }

        const uint8_t current_id = color_component_id_node.desired<uint8_t>();
        auto next_component      = next_supported_color_component(mask, current_id + 1);
        if (!next_component.has_value()) {
            return SL_STATUS_OK;
        }

        color_component_id_node.set_desired<uint8_t>(next_component.value());
        start_group_resolution(get_group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_color::on_switch_color_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto color_component_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_get_group_attributes_t::color_component_id));
        if (!color_component_id_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(color_component_id_node, DESIRED_ATTRIBUTE);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_switch_color::on_switch_color_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto color_component_count_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_set_group_attributes_t::color_component_count));
        if (!color_component_count_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(color_component_count_node, DESIRED_ATTRIBUTE);

        auto vg1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_set_group_attributes_t::vg1));
        if (!vg1_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(vg1_node, DESIRED_ATTRIBUTE);

        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_set_group_attributes_t::duration));
        if (!duration_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(duration_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_switch_color::on_switch_color_start_level_change_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto ignore_node   = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::ignore_start_state));
        auto up_down_node  = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::up_down));
        auto ccid_node     = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::color_component_id));
        auto start_node    = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::start_level));
        auto duration_node = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::duration));

        uint8_t properties1 = 0;
        if (ignore_node.is_valid() && ignore_node.desired_exists()) {
            properties1 |= (ignore_node.desired<uint8_t>() & 1) << 5;
        }
        if (up_down_node.is_valid() && up_down_node.desired_exists()) {
            properties1 |= (up_down_node.desired<uint8_t>() & 1) << 6;
        }
        frame_generator->add_raw_byte(properties1);
        if (ccid_node.is_valid() && ccid_node.desired_exists()) {
            frame_generator->add_value(ccid_node, DESIRED_ATTRIBUTE);
        }
        if (start_node.is_valid() && start_node.desired_exists()) {
            frame_generator->add_value(start_node, DESIRED_ATTRIBUTE);
        }
        if (duration_node.is_valid() && duration_node.desired_exists()) {
            frame_generator->add_value(duration_node, DESIRED_ATTRIBUTE);
        }
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_switch_color::on_switch_color_stop_level_change_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto ccid_node = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_color_stop_level_change_group_attributes_t::color_component_id));
        if (ccid_node.is_valid() && ccid_node.desired_exists()) {
            frame_generator->add_value(ccid_node, DESIRED_ATTRIBUTE);
        }
        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class