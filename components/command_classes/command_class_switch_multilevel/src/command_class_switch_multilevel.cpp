
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
#include "command_class_switch_multilevel.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_multilevel";

    command_class_switch_multilevel::command_class_switch_multilevel() {}

    // Base class only invokes on_interview for endpoints with reported CC version > 0 (i.e. that support this CC).
    void command_class_switch_multilevel::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        auto get_group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_get_group_attributes_t::SWITCH_MULTILEVEL_GET_GROUP));
        start_group_resolution(get_group_node, interview_resolution_options());

        if (supported_version >= 3) {
            auto supported_get_group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_supported_get_group_attributes_t::SWITCH_MULTILEVEL_SUPPORTED_GET_GROUP));
            start_group_resolution(supported_get_group_node, interview_resolution_options());
        }
    }

    static sl_status_t complete_switch_multilevel_interview(attribute_store::attribute endpoint, zwave_command_class_t command_class_id)
    {
        const auto report           = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_multilevel_report_group_attributes_t::SWITCH_MULTILEVEL_REPORT_GROUP));
        const auto version          = endpoint.child_by_type(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_SWITCH_MULTILEVEL));
        const auto supported_report = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_multilevel_supported_report_group_attributes_t::SWITCH_MULTILEVEL_SUPPORTED_REPORT_GROUP));
        if (report.is_valid() && (!version.reported_exists() || version.reported<uint8_t>() < 3 || supported_report.is_valid())) {
            zwave_command_class_base::set_cc_interview_state(endpoint, command_class_id, zwave_command_class_base::cc_interview_state::done);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_multilevel::on_switch_multilevel_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_switch_multilevel_attribute_map_t payload)
    {
        return complete_switch_multilevel_interview(endpoint, cc_properties.command_class_id);
    }

    sl_status_t command_class_switch_multilevel::on_switch_multilevel_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_switch_multilevel_attribute_map_t payload)
    {
        return complete_switch_multilevel_interview(endpoint, cc_properties.command_class_id);
    }

    sl_status_t command_class_switch_multilevel::on_switch_multilevel_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::value));
        if (!value_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(value_node, DESIRED_ATTRIBUTE);

        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::duration));
        if (!duration_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(duration_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_switch_multilevel::on_switch_multilevel_start_level_change_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto up_down_node            = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::up_down));
        auto ignore_start_level_node = group_node.child_by_type(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::ignore_start_level));

        switch_multilevel_start_level_change_properties1_t properties1 {};
        properties1.value = 0;

        if (up_down_node.is_valid() && up_down_node.desired_exists()) {
            properties1.flags.switch_multilevel_start_level_change_up_down = up_down_node.desired<uint8_t>();
        }
        if (ignore_start_level_node.is_valid() && ignore_start_level_node.desired_exists()) {
            properties1.flags.switch_multilevel_start_level_change_ignore_start_level = ignore_start_level_node.desired<uint8_t>();
        } else {
            properties1.flags.switch_multilevel_start_level_change_ignore_start_level = 1;
        }

        auto inc_dec_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::inc_dec));
        if (!inc_dec_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        properties1.flags.switch_multilevel_start_level_change_inc_dec = inc_dec_node.desired<uint8_t>();

        frame_generator->add_raw_byte(properties1.value);

        auto start_level_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::start_level));
        if (!start_level_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(start_level_node, DESIRED_ATTRIBUTE);

        auto dimming_duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::dimming_duration));
        if (!dimming_duration_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(dimming_duration_node, DESIRED_ATTRIBUTE);

        auto step_size_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::step_size));
        if (!step_size_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(step_size_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class