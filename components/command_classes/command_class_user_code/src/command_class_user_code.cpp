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

#include "command_class_user_code.hpp"
#include "command_class_user_code_constants.hpp"
#include "command_class_user_code_rules.hpp"

namespace zwave_command_class
{
    using namespace command_class_user_code_types;
    using namespace command_class_user_code_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_code";

    attribute_store::attribute command_class_user_code::desired_child(attribute_store::attribute group, attribute_store_type_t type)
    {
        auto node = group.child_by_type(type);
        if (!node.is_valid() || !node.desired_exists()) {
            return {};
        }
        return node;
    }

    void command_class_user_code::request_users_number(attribute_store::attribute endpoint)
    {
        auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::supported_users)));
        start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(users_number_get_group_attributes_t::USERS_NUMBER_GET_GROUP)), interview_resolution_options());
    }

    void command_class_user_code::request_v1_user(attribute_store::attribute endpoint, uint8_t identifier)
    {
        auto report = endpoint.add_node(static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::USER_CODE_REPORT_GROUP));
        report.emplace_node(static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_identifier)).set_reported(identifier);
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_id_status)));
        auto get = endpoint.emplace_node(static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::USER_CODE_GET_GROUP));
        get.emplace_node(static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::user_identifier)).set_desired(identifier);
        start_group_resolution(get, interview_resolution_options());
    }

    void command_class_user_code::request_extended_user(attribute_store::attribute endpoint, uint16_t identifier)
    {
        auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::EXTENDED_USER_CODE_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::next_user_identifier)));
        auto get = endpoint.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::EXTENDED_USER_CODE_GET_GROUP));
        get.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::user_identifier)).set_desired(identifier);
        // CC:0063.02.0C.11.008: 1 asks a MUCR node for more than the requested user.
        const uint8_t report_more = command_class_user_code_rules::capability_flag_is_set(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::mucr_support)) ? 1 : 0;
        get.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::report_more)).set_desired(report_more);
        start_group_resolution(get, interview_resolution_options());
    }

    void command_class_user_code::request_keypad_or_users(attribute_store::attribute endpoint)
    {
        if (command_class_user_code_rules::multiple_keypad_modes_are_supported(endpoint)) {
            auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::USER_CODE_KEYPAD_MODE_REPORT_GROUP));
            cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::keypad_mode)));
            start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_get_group_attributes_t::USER_CODE_KEYPAD_MODE_GET_GROUP)), interview_resolution_options());
            return;
        }
        if (command_class_user_code_rules::supported_user_total(endpoint) == 0) {
            return;
        }
        request_extended_user(endpoint, FIRST_USER_IDENTIFIER);
    }

    void command_class_user_code::request_admin_or_continue(attribute_store::attribute endpoint)
    {
        if (command_class_user_code_rules::capability_flag_is_set(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ac_support))) {
            auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::ADMIN_CODE_REPORT_GROUP));
            cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code_length)));
            start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(admin_code_get_group_attributes_t::ADMIN_CODE_GET_GROUP)), interview_resolution_options());
            return;
        }
        request_keypad_or_users(endpoint);
    }

    void command_class_user_code::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        if (supported_version >= 2) {
            auto report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP));
            cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ac_support)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_get_group_attributes_t::USER_CODE_CAPABILITIES_GET_GROUP)), interview_resolution_options());
            return;
        }
        request_users_number(endpoint_node);
    }

    sl_status_t command_class_user_code::on_user_code_capabilities_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t /*payload*/)
    {
        if (cc_interview_is_open(endpoint)) {
            request_users_number(endpoint);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code::on_users_number_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t /*payload*/)
    {
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        if (command_class_user_code_rules::peer_version(endpoint) >= 2) {
            request_admin_or_continue(endpoint);
            return SL_STATUS_OK;
        }
        if (command_class_user_code_rules::supported_user_total(endpoint) == 0) {
            return SL_STATUS_OK;
        }
        request_v1_user(endpoint, FIRST_USER_IDENTIFIER);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code::on_admin_code_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t /*payload*/)
    {
        if (cc_interview_is_open(endpoint)) {
            request_keypad_or_users(endpoint);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code::on_user_code_keypad_mode_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t /*payload*/)
    {
        if (cc_interview_is_open(endpoint) && command_class_user_code_rules::supported_user_total(endpoint) != 0) {
            request_extended_user(endpoint, FIRST_USER_IDENTIFIER);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code::on_extended_user_code_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload)
    {
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        const uint16_t next = get_value_or_default<extended_user_code_report_next_user_identifier_t>(payload, "next_user_identifier", static_cast<uint16_t>(0));
        // CC:0063.02.0D.11.00B: 0 means this user is the last one set.
        if (next != 0) {
            request_extended_user(endpoint, next);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code::on_user_code_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload)
    {
        if (!cc_interview_is_open(endpoint) || command_class_user_code_rules::peer_version(endpoint) >= 2) {
            return SL_STATUS_OK;
        }
        const uint8_t identifier = get_value_or_default<user_code_report_user_identifier_t>(payload, "user_identifier", static_cast<uint8_t>(0));
        if (identifier > 0 && identifier < command_class_user_code_rules::supported_user_total(endpoint)) {
            request_v1_user(endpoint, static_cast<uint8_t>(identifier + 1));
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code::on_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group = args.node;
        auto id    = desired_child(group, static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::user_identifier));
        if (!id.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        const uint8_t identifier = id.desired<uint8_t>();
        // CC:0063.01.02.11.004: identifier 0 is not a User Code Get.
        if (identifier == 0 || !command_class_user_code_rules::user_identifier_is_supported(group.parent(), identifier)) {
            return SL_STATUS_FAIL;
        }
        args.get_frame_generator->add_value(id, DESIRED_ATTRIBUTE);
        return args.get_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group  = args.node;
        auto id     = desired_child(group, static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_identifier));
        auto status = desired_child(group, static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_id_status));
        auto code   = desired_child(group, static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_code));
        if (!id.is_valid() || !status.is_valid() || !code.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        const uint8_t identifier = id.desired<uint8_t>();
        const auto digits        = code.desired<std::vector<uint8_t>>();
        if (!command_class_user_code_rules::user_code_v1_set_is_valid(status.desired<uint8_t>(), digits) || !command_class_user_code_rules::user_identifier_is_supported(group.parent(), identifier)) {
            return SL_STATUS_FAIL;
        }
        if (identifier == 0 && status.desired<uint8_t>() != STATUS_AVAILABLE) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(id, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(status, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(code, DESIRED_ATTRIBUTE);
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_extended_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group = args.node;
        auto id    = desired_child(group, static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::user_identifier));
        auto more  = desired_child(group, static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::report_more));
        if (!id.is_valid() || !more.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        const uint16_t identifier = id.desired<uint16_t>();
        if (identifier == 0 || !command_class_user_code_rules::user_identifier_is_supported(group.parent(), identifier)) {
            return SL_STATUS_FAIL;
        }
        args.get_frame_generator->add_value(id, DESIRED_ATTRIBUTE);
        args.get_frame_generator->add_value(more, DESIRED_ATTRIBUTE);
        return args.get_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_extended_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group = args.node;
        auto count = desired_child(group, static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes));
        auto vg1   = desired_child(group, static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1));
        if (!count.is_valid() || !vg1.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_code_rules::extended_user_code_blocks_are_valid(group.parent(), count.desired<uint8_t>(), vg1.desired<std::vector<uint8_t>>())) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(count, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(vg1, DESIRED_ATTRIBUTE);
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_user_code_keypad_mode_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto mode = desired_child(args.node, static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode));
        if (!mode.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_code_rules::keypad_mode_set_is_supported(args.node.parent(), mode.desired<uint8_t>())) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(mode, DESIRED_ATTRIBUTE);
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_admin_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto length = desired_child(args.node, static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length));
        if (!length.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        std::vector<uint8_t> code;
        auto code_node = args.node.child_by_type(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code));
        if (code_node.is_valid() && code_node.desired_exists()) {
            code = code_node.desired<std::vector<uint8_t>>();
        }
        if (!command_class_user_code_rules::admin_code_is_valid(args.node.parent(), length.desired<uint8_t>(), code)) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(length, DESIRED_ATTRIBUTE);
        if (!code.empty()) {
            args.set_frame_generator->add_value(code_node, DESIRED_ATTRIBUTE);
        }
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_code::on_user_code_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        if (!command_class_user_code_rules::user_code_checksum_get_is_supported(args.node.parent())) {
            return SL_STATUS_FAIL;
        }
        return args.get_frame_generator->generate_no_args_frame();
    }

}  // namespace zwave_command_class
