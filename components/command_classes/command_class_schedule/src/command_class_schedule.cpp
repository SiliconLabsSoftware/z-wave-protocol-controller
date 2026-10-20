
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
#include <vector>

#include "command_class_schedule.hpp"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_schedule";

    command_class_schedule::command_class_schedule() {}

    void command_class_schedule::start_schedule_supported_get(attribute_store::attribute endpoint_node, uint8_t schedule_id_block)
    {
        auto get_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP));
        get_group.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);
        command_class_schedule_core::start_group_resolution(get_group);
    }

    void command_class_schedule::start_schedule_supported_commands_get(attribute_store::attribute endpoint_node, uint8_t schedule_id_block)
    {
        auto get_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP));
        get_group.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);
        command_class_schedule_core::start_group_resolution(get_group);
    }

    void command_class_schedule::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        constexpr uint8_t first_schedule_id_block = 0;

        auto supported_report = find_or_create_supported_report_group_by_schedule_id_block(endpoint_node, first_schedule_id_block);
        cc_interview_require_attribute(supported_report.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_id)));
        start_schedule_supported_get(endpoint_node, first_schedule_id_block);

        // Schedule Supported Commands Get/Report is v4+.
        if (supported_version >= 4) {
            auto supported_commands_report = find_or_create_supported_commands_report_group_by_schedule_id_block(endpoint_node, first_schedule_id_block);
            cc_interview_require_attribute(supported_commands_report.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::command_class_list_length)));
            start_schedule_supported_commands_get(endpoint_node, first_schedule_id_block);
        }
    }

    sl_status_t command_class_schedule::on_schedule_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_schedule_attribute_map_t payload)
    {
        (void)connection_info;

        uint8_t schedule_id_block = 0;
        schedule_id_block         = get_value_or_default(payload, "schedule_id_block", schedule_id_block);

        uint8_t number_of_supported_schedule_blocks = 0;
        number_of_supported_schedule_blocks         = get_value_or_default(payload, "number_of_supported_schedule_blocks", number_of_supported_schedule_blocks);

        if (number_of_supported_schedule_blocks == 0) {
            return SL_STATUS_OK;
        }

        const uint8_t next_schedule_id_block = static_cast<uint8_t>(schedule_id_block + 1);
        if (next_schedule_id_block >= number_of_supported_schedule_blocks) {
            return SL_STATUS_OK;
        }

        auto next_report = find_or_create_supported_report_group_by_schedule_id_block(endpoint, next_schedule_id_block);
        cc_interview_require_attribute(next_report.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_id)));
        start_schedule_supported_get(endpoint, next_schedule_id_block);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule::on_schedule_supported_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto schedule_id_block_node = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::schedule_id_block));
        if (!schedule_id_block_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(schedule_id_block_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_schedule::on_command_schedule_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto schedule_id_node             = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::schedule_id));
        auto schedule_id_block_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::schedule_id_block));
        auto start_year_node              = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_year));
        auto start_month_node             = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_month));
        auto recurrence_offset_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::recurrence_offset));
        auto start_day_of_month_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_day_of_month));
        auto recurrence_mode_node         = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::recurrence_mode));
        auto start_weekday_node           = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_weekday));
        auto start_hour_node              = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_hour));
        auto duration_type_node           = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::duration_type));
        auto start_minute_node            = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_minute));
        auto relative_node                = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::relative));
        auto duration_byte_node           = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::duration_byte));
        auto reports_to_follow_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::reports_to_follow));
        auto number_of_cmd_to_follow_node = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::number_of_cmd_to_follow));
        auto vg1_node                     = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::vg1));

        if (!schedule_id_node.desired_exists() || !schedule_id_block_node.desired_exists() || !start_year_node.desired_exists() || !start_month_node.desired_exists() || !recurrence_offset_node.desired_exists() || !start_day_of_month_node.desired_exists() || !recurrence_mode_node.desired_exists()
            || !start_weekday_node.desired_exists() || !start_hour_node.desired_exists() || !duration_type_node.desired_exists() || !start_minute_node.desired_exists() || !relative_node.desired_exists() || !duration_byte_node.desired_exists() || !reports_to_follow_node.desired_exists()
            || !number_of_cmd_to_follow_node.desired_exists() || !vg1_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(schedule_id_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(schedule_id_block_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(start_year_node, DESIRED_ATTRIBUTE);
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = start_month_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 4, .node = recurrence_offset_node, .node_value_state = DESIRED_ATTRIBUTE},
        });
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = start_day_of_month_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 5, .node = recurrence_mode_node, .node_value_state = DESIRED_ATTRIBUTE},
        });
        frame_generator->add_value(start_weekday_node, DESIRED_ATTRIBUTE);
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = start_hour_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 5, .node = duration_type_node, .node_value_state = DESIRED_ATTRIBUTE},
        });
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = start_minute_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 6, .node = relative_node, .node_value_state = DESIRED_ATTRIBUTE},
        });
        frame_generator->add_value(duration_byte_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(reports_to_follow_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(number_of_cmd_to_follow_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(vg1_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_schedule::on_command_schedule_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto schedule_id_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::schedule_id));
        auto schedule_id_block_node = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::schedule_id_block));
        auto aid_ro_ctl_node        = group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::aid_ro_ctl));

        if (!schedule_id_node.desired_exists() || !schedule_id_block_node.desired_exists() || !aid_ro_ctl_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(schedule_id_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(schedule_id_block_node, DESIRED_ATTRIBUTE);
        frame_generator->add_shifted_values({
          {.left_shift = 7, .node = aid_ro_ctl_node, .node_value_state = DESIRED_ATTRIBUTE},
        });

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_schedule::on_schedule_remove_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto schedule_id_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::schedule_id));
        auto schedule_id_block_node = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::schedule_id_block));

        if (!schedule_id_node.desired_exists() || !schedule_id_block_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(schedule_id_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(schedule_id_block_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_schedule::on_schedule_state_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto schedule_id_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_id));
        auto schedule_state_node    = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_state));
        auto schedule_id_block_node = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_id_block));

        if (!schedule_id_node.desired_exists() || !schedule_state_node.desired_exists() || !schedule_id_block_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(schedule_id_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(schedule_state_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(schedule_id_block_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_schedule::on_schedule_state_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto schedule_id_block_node = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_get_group_attributes_t::schedule_id_block));
        if (!schedule_id_block_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(schedule_id_block_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_schedule::on_schedule_supported_commands_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto schedule_id_block_node = group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::schedule_id_block));
        if (!schedule_id_block_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(schedule_id_block_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
