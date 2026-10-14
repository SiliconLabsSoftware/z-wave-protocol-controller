
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

#include <bit>
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>
#include <vector>

#include "command_class_user_code.hpp"

#include "ZW_classcmd.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_code";

    command_class_user_code::command_class_user_code() {}

    size_t command_class_user_code::count_set_bits(const std::vector<uint8_t> &bit_mask)
    {
        size_t count = 0;
        for (uint8_t byte: bit_mask) {
            count += static_cast<size_t>(std::popcount(byte));
        }
        return count;
    }

    void command_class_user_code::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP));
        if (supported_version >= 2) {
            invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP));
            invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::USER_CODE_KEYPAD_MODE_REPORT_GROUP));
            invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::ADMIN_CODE_REPORT_GROUP));
        }

        auto users_number_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP));
        cc_interview_require_attribute(users_number_report.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::supported_users)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(users_number_get_group_attributes_t::USERS_NUMBER_GET_GROUP)), interview_resolution_options());

        if (supported_version >= 2) {
            auto capabilities_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP));
            cc_interview_require_attribute(capabilities_report.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ac_support)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_get_group_attributes_t::USER_CODE_CAPABILITIES_GET_GROUP)), interview_resolution_options());
        }
    }

    sl_status_t command_class_user_code::on_user_code_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload)
    {
        (void)connection_info;

        const uint8_t ac_support = get_value_or_default(payload, "ac_support", static_cast<uint8_t>(0));
        if (ac_support != 0) {
            auto admin_code_report = endpoint.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::ADMIN_CODE_REPORT_GROUP));
            cc_interview_require_attribute(admin_code_report.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code_length)));
            start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(admin_code_get_group_attributes_t::ADMIN_CODE_GET_GROUP)), interview_resolution_options());
        }

        const std::vector<uint8_t> empty_mask;
        const auto keypad_modes_mask = get_value_or_default(payload, "supported_keypad_modes_bit_mask", empty_mask);
        if (count_set_bits(keypad_modes_mask) > 1) {
            auto keypad_mode_report = endpoint.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::USER_CODE_KEYPAD_MODE_REPORT_GROUP));
            cc_interview_require_attribute(keypad_mode_report.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::keypad_mode)));
            start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_get_group_attributes_t::USER_CODE_KEYPAD_MODE_GET_GROUP)), interview_resolution_options());
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code::on_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto user_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::user_identifier));
        if (!user_identifier_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(user_identifier_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto user_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_identifier));
        auto user_id_status_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_id_status));
        auto user_code_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_code));
        if (!user_identifier_node.desired_exists() || !user_id_status_node.desired_exists() || !user_code_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(user_identifier_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(user_id_status_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(user_code_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_extended_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto user_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::user_identifier));
        auto report_more_node     = group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::report_more));
        if (!user_identifier_node.desired_exists() || !report_more_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(user_identifier_node, DESIRED_ATTRIBUTE);
        frame_generator->add_raw_byte(report_more_node.desired<uint8_t>() & 0x01);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_extended_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto number_of_user_codes_node = group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes));
        auto vg1_node                  = group_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1));
        if (!number_of_user_codes_node.desired_exists() || !vg1_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(number_of_user_codes_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(vg1_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_user_code_keypad_mode_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto keypad_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode));
        if (!keypad_mode_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(keypad_mode_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_admin_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto admin_code_length_node = group_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length));
        auto admin_code_node        = group_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code));
        if (!admin_code_length_node.desired_exists() || !admin_code_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_raw_byte(admin_code_length_node.desired<uint8_t>() & 0x0F);
        frame_generator->add_value(admin_code_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
