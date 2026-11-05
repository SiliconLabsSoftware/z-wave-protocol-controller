/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <vector>

#include "command_class_user_credential.hpp"
#include "command_class_user_credential_constants.hpp"
#include "command_class_user_credential_events.hpp"
#include "command_class_user_credential_rules.hpp"
#include "component_connector.hpp"
#include "ZW_classcmd.h"

namespace zwave_command_class
{
    using namespace command_class_user_credential_types;
    using namespace command_class_user_credential_constants;

    command_class_user_credential::command_class_user_credential()
    {
        component_connector connector;
        connector.connect_typed<command_class_user_credential_events_t, user_credential_presence_query_payload_t>(command_class_user_credential_events_t::COMMAND_CLASS_USER_CREDENTIAL_PRESENCE_QUERY,
                                                                                                                  [](const user_credential_presence_query_payload_t &payload) { return on_user_code_presence_query(payload); });
    }

    sl_status_t command_class_user_credential::on_user_code_presence_query(const user_credential_presence_query_payload_t &payload)
    {
        const bool present = command_class_user_credential_rules::endpoint_advertises_command_class(payload.endpoint, TARGET_COMMAND_CLASS);
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_user_credential_events_t::COMMAND_CLASS_USER_CREDENTIAL_PRESENCE_RESPONSE), user_credential_presence_response_payload_t {payload.endpoint, present});
        return SL_STATUS_OK;
    }

    attribute_store::attribute command_class_user_credential::desired_child(attribute_store::attribute group, attribute_store_type_t type)
    {
        auto node = group.child_by_type(type);
        if (!node.is_valid() || !node.desired_exists()) {
            return {};
        }
        return node;
    }

    void command_class_user_credential::request_user_capabilities(attribute_store::attribute endpoint)
    {
        auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::USER_CAPABILITIES_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::number_of_supported_user_unique_identifiers)));
        start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(user_capabilities_get_group_attributes_t::USER_CAPABILITIES_GET_GROUP)), interview_resolution_options());
    }

    void command_class_user_credential::request_credential_capabilities(attribute_store::attribute endpoint)
    {
        auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::CREDENTIAL_CAPABILITIES_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::number_of_supported_credential_types)));
        start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(credential_capabilities_get_group_attributes_t::CREDENTIAL_CAPABILITIES_GET_GROUP)), interview_resolution_options());
    }

    void command_class_user_credential::request_admin_pin_code(attribute_store::attribute endpoint)
    {
        if (!command_class_user_credential_rules::admin_pin_code_get_is_supported(endpoint)) {
            return;
        }
        auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(admin_pin_code_report_group_attributes_t::ADMIN_PIN_CODE_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(admin_pin_code_report_group_attributes_t::admin_pin_code_length)));
        start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(admin_pin_code_get_group_attributes_t::ADMIN_PIN_CODE_GET_GROUP)), interview_resolution_options());
    }

    void command_class_user_credential::request_user(attribute_store::attribute endpoint, uint16_t user_unique_identifier)
    {
        auto report = endpoint.add_node(static_cast<attribute_store_type_t>(user_report_group_attributes_t::USER_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(user_report_group_attributes_t::next_user_unique_identifier)));
        auto get = endpoint.emplace_node(static_cast<attribute_store_type_t>(user_get_group_attributes_t::USER_GET_GROUP));
        get.emplace_node(static_cast<attribute_store_type_t>(user_get_group_attributes_t::user_unique_identifier)).set_desired(user_unique_identifier);
        start_group_resolution(get, interview_resolution_options());
    }

    void command_class_user_credential::request_credential(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot)
    {
        using report_attributes_t    = credential_report_group_attributes_t;
        const auto report_group_type = static_cast<attribute_store_type_t>(report_attributes_t::CREDENTIAL_REPORT_GROUP);
        auto report                  = attribute_store::attribute {};
        for (auto candidate: endpoint.children(report_group_type)) {
            auto candidate_user = candidate.child_by_type(static_cast<attribute_store_type_t>(report_attributes_t::user_unique_identifier));
            auto candidate_type = candidate.child_by_type(static_cast<attribute_store_type_t>(report_attributes_t::credential_type));
            auto candidate_slot = candidate.child_by_type(static_cast<attribute_store_type_t>(report_attributes_t::credential_slot));
            if (candidate_user.reported_exists() && candidate_type.reported_exists() && candidate_slot.reported_exists() && candidate_user.reported<uint16_t>() == user_unique_identifier && candidate_type.reported<uint8_t>() == credential_type
                && candidate_slot.reported<uint16_t>() == credential_slot) {
                report = candidate;
                break;
            }
        }
        if (!report.is_valid()) {
            report = endpoint.add_node(report_group_type);
        }
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(credential_report_group_attributes_t::next_credential_type)));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(credential_report_group_attributes_t::next_credential_slot)));
        auto get = endpoint.emplace_node(static_cast<attribute_store_type_t>(credential_get_group_attributes_t::CREDENTIAL_GET_GROUP));
        get.emplace_node(static_cast<attribute_store_type_t>(credential_get_group_attributes_t::user_unique_identifier)).set_desired(user_unique_identifier);
        get.emplace_node(static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_type)).set_desired(credential_type);
        get.emplace_node(static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_slot)).set_desired(credential_slot);
        start_group_resolution(get, interview_resolution_options());
    }

    void command_class_user_credential::forget_stored_users_and_credentials(attribute_store::attribute endpoint)
    {
        for (const auto group_type: {static_cast<attribute_store_type_t>(user_report_group_attributes_t::USER_REPORT_GROUP), static_cast<attribute_store_type_t>(credential_report_group_attributes_t::CREDENTIAL_REPORT_GROUP)}) {
            for (auto group = endpoint.child_by_type(group_type); group.is_valid(); group = endpoint.child_by_type(group_type)) {
                group.delete_node();
            }
        }
    }

    void command_class_user_credential::request_checksum_or_users(attribute_store::attribute endpoint)
    {
        forget_stored_users_and_credentials(endpoint);
        if (command_class_user_credential_rules::all_users_checksum_is_supported(endpoint)) {
            auto report = endpoint.emplace_node(static_cast<attribute_store_type_t>(all_users_checksum_report_group_attributes_t::ALL_USERS_CHECKSUM_REPORT_GROUP));
            cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(all_users_checksum_report_group_attributes_t::all_users_checksum)));
            start_group_resolution(endpoint.emplace_node(static_cast<attribute_store_type_t>(all_users_checksum_get_group_attributes_t::ALL_USERS_CHECKSUM_GET_GROUP)), interview_resolution_options());
            return;
        }
        request_user(endpoint, DELETE_ALL_IDENTIFIER);
    }

    void command_class_user_credential::on_interview(attribute_store::attribute endpoint_node, uint8_t /*supported_version*/)
    {
        request_user_capabilities(endpoint_node);
    }

    sl_status_t command_class_user_credential::on_user_capabilities_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t /*payload*/)
    {
        const auto count = command_class_user_credential_rules::supported_user_count(endpoint);
        if (count.has_value()) {
            component_connector connector;
            const user_credential_supported_user_count_payload_t event {endpoint, *count};
            connector.fire_event(static_cast<uint32_t>(command_class_user_credential_events_t::COMMAND_CLASS_USER_CREDENTIAL_SUPPORTED_USER_COUNT), event);
        }
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        if (count.has_value() && *count == 0 && command_class_user_credential_rules::endpoint_advertises_command_class(endpoint, COMMAND_CLASS_USER_CODE)) {
            forget_stored_users_and_credentials(endpoint);
            return SL_STATUS_OK;
        }
        request_credential_capabilities(endpoint);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential::on_credential_capabilities_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t /*payload*/)
    {
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        request_admin_pin_code(endpoint);
        request_checksum_or_users(endpoint);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential::on_all_users_checksum_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload)
    {
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        const uint16_t checksum = get_value_or_default<uint16_t>(payload, "all_users_checksum", 0);
        if (checksum != 0) {
            request_user(endpoint, DELETE_ALL_IDENTIFIER);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential::on_user_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload)
    {
        const uint16_t user_id = get_value_or_default<uint16_t>(payload, "user_unique_identifier", 0);
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        if (command_class_user_credential_rules::user_schedule_is_supported(endpoint) && user_id != DELETE_ALL_IDENTIFIER) {
            component_connector connector;
            const user_credential_interview_user_schedule_payload_t event {endpoint, TARGET_COMMAND_CLASS, user_id};
            connector.fire_event(static_cast<uint32_t>(command_class_user_credential_events_t::COMMAND_CLASS_USER_CREDENTIAL_INTERVIEW_USER_SCHEDULE), event);
        }
        if (user_id != DELETE_ALL_IDENTIFIER) {
            request_credential(endpoint, user_id, CREDENTIAL_TYPE_NONE, FIRST_SLOT);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential::on_credential_report_parsed(const zwave_controller_connection_info_t * /*connection_info*/, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload)
    {
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        const uint16_t user_id         = get_value_or_default<uint16_t>(payload, "user_unique_identifier", 0);
        const uint8_t credential_type  = get_value_or_default<uint8_t>(payload, "credential_type", CREDENTIAL_TYPE_NONE);
        const uint16_t credential_slot = get_value_or_default<uint16_t>(payload, "credential_slot", FIRST_SLOT);
        const uint8_t next_type        = get_value_or_default<uint8_t>(payload, "next_credential_type", 0);
        const uint16_t next_slot       = get_value_or_default<uint16_t>(payload, "next_credential_slot", 0);

        auto get            = endpoint.child_by_type(static_cast<attribute_store_type_t>(credential_get_group_attributes_t::CREDENTIAL_GET_GROUP));
        auto requested_user = desired_child(get, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::user_unique_identifier));
        auto requested_type = desired_child(get, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_type));
        auto requested_slot = desired_child(get, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_slot));
        if (requested_user.is_valid() && requested_type.is_valid() && requested_slot.is_valid() && requested_user.desired<uint16_t>() == user_id && (requested_type.desired<uint8_t>() != credential_type || requested_slot.desired<uint16_t>() != credential_slot)) {
            request_credential(endpoint, user_id, credential_type, credential_slot);
            return SL_STATUS_OK;
        }

        if (next_type != CREDENTIAL_TYPE_NONE || next_slot != FIRST_SLOT) {
            request_credential(endpoint, user_id, next_type, next_slot);
            return SL_STATUS_OK;
        }
        uint16_t next_user = 0;
        for (auto group: endpoint.children(static_cast<attribute_store_type_t>(user_report_group_attributes_t::USER_REPORT_GROUP))) {
            auto id_node = group.child_by_type(static_cast<attribute_store_type_t>(user_report_group_attributes_t::user_unique_identifier));
            if (!id_node.reported_exists() || id_node.reported<uint16_t>() != user_id) {
                continue;
            }
            auto next = group.child_by_type(static_cast<attribute_store_type_t>(user_report_group_attributes_t::next_user_unique_identifier));
            if (next.reported_exists()) {
                next_user = next.reported<uint16_t>();
            }
        }
        if (next_user != DELETE_ALL_IDENTIFIER) {
            request_user(endpoint, next_user);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential::on_user_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group       = args.node;
        auto operation   = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::operation_type));
        auto identifier  = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_unique_identifier));
        auto user_type   = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_type));
        auto active      = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_active_state));
        auto rule        = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::credential_rule));
        auto timeout     = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::expiring_timeout_minutes));
        auto encoding    = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_name_encoding));
        auto name_length = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_name_length));
        auto name        = desired_child(group, static_cast<attribute_store_type_t>(user_set_group_attributes_t::user_name));
        if (!operation.is_valid() || !identifier.is_valid() || !user_type.is_valid() || !active.is_valid() || !rule.is_valid() || !timeout.is_valid() || !encoding.is_valid() || !name_length.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        std::vector<uint8_t> user_name;
        if (name.is_valid()) {
            user_name = name.desired<std::vector<uint8_t>>();
        }
        if (!command_class_user_credential_rules::
              user_set_is_valid(group.parent(), operation.desired<uint8_t>(), identifier.desired<uint16_t>(), user_type.desired<uint8_t>(), active.desired<uint8_t>(), rule.desired<uint8_t>(), timeout.desired<uint16_t>(), encoding.desired<uint8_t>(), name_length.desired<uint8_t>(), user_name)) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(operation, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(identifier, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(user_type, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(active, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(rule, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(timeout, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(encoding, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(name_length, DESIRED_ATTRIBUTE);
        if (name.is_valid()) {
            args.set_frame_generator->add_value(name, DESIRED_ATTRIBUTE);
        }
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_user_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto identifier = desired_child(args.node, static_cast<attribute_store_type_t>(user_get_group_attributes_t::user_unique_identifier));
        if (!identifier.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_credential_rules::user_identifier_is_supported(args.node.parent(), identifier.desired<uint16_t>(), true)) {
            return SL_STATUS_FAIL;
        }
        args.get_frame_generator->add_value(identifier, DESIRED_ATTRIBUTE);
        return args.get_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_credential_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto identifier = desired_child(args.node, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::user_unique_identifier));
        auto type       = desired_child(args.node, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_type));
        auto slot       = desired_child(args.node, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_slot));
        auto operation  = desired_child(args.node, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::operation_type));
        auto length     = desired_child(args.node, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_length));
        auto data       = desired_child(args.node, static_cast<attribute_store_type_t>(credential_set_group_attributes_t::credential_data));
        if (!identifier.is_valid() || !type.is_valid() || !slot.is_valid() || !operation.is_valid() || !length.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        std::vector<uint8_t> credential_data;
        if (data.is_valid()) {
            credential_data = data.desired<std::vector<uint8_t>>();
        }
        if (!command_class_user_credential_rules::credential_set_is_valid(args.node.parent(), identifier.desired<uint16_t>(), type.desired<uint8_t>(), slot.desired<uint16_t>(), operation.desired<uint8_t>(), length.desired<uint8_t>(), credential_data)) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(identifier, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(type, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(slot, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(operation, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(length, DESIRED_ATTRIBUTE);
        if (data.is_valid()) {
            args.set_frame_generator->add_value(data, DESIRED_ATTRIBUTE);
        }
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_credential_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto identifier = desired_child(args.node, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::user_unique_identifier));
        auto type       = desired_child(args.node, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_type));
        auto slot       = desired_child(args.node, static_cast<attribute_store_type_t>(credential_get_group_attributes_t::credential_slot));
        if (!identifier.is_valid() || !type.is_valid() || !slot.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_credential_rules::credential_get_is_valid(args.node.parent(), identifier.desired<uint16_t>(), type.desired<uint8_t>(), slot.desired<uint16_t>())) {
            return SL_STATUS_FAIL;
        }
        args.get_frame_generator->add_value(identifier, DESIRED_ATTRIBUTE);
        args.get_frame_generator->add_value(type, DESIRED_ATTRIBUTE);
        args.get_frame_generator->add_value(slot, DESIRED_ATTRIBUTE);
        return args.get_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_credential_learn_start_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto identifier = desired_child(args.node, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::user_unique_identifier));
        auto type       = desired_child(args.node, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::credential_type));
        auto slot       = desired_child(args.node, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::credential_slot));
        auto operation  = desired_child(args.node, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::operation_type));
        auto timeout    = desired_child(args.node, static_cast<attribute_store_type_t>(credential_learn_start_group_attributes_t::credential_learn_timeout));
        if (!identifier.is_valid() || !type.is_valid() || !slot.is_valid() || !operation.is_valid() || !timeout.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_credential_rules::credential_learn_start_is_valid(args.node.parent(), identifier.desired<uint16_t>(), type.desired<uint8_t>(), slot.desired<uint16_t>(), operation.desired<uint8_t>())) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(identifier, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(type, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(slot, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(operation, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(timeout, DESIRED_ATTRIBUTE);
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_credential_learn_cancel_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        return args.set_frame_generator->generate_no_args_frame();
    }

    sl_status_t command_class_user_credential::on_user_credential_association_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto type        = desired_child(args.node, static_cast<attribute_store_type_t>(user_credential_association_set_group_attributes_t::credential_type));
        auto slot        = desired_child(args.node, static_cast<attribute_store_type_t>(user_credential_association_set_group_attributes_t::credential_slot));
        auto destination = desired_child(args.node, static_cast<attribute_store_type_t>(user_credential_association_set_group_attributes_t::destination_user_unique_identifier));
        if (!type.is_valid() || !slot.is_valid() || !destination.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_credential_rules::user_credential_association_set_is_valid(args.node.parent(), type.desired<uint8_t>(), slot.desired<uint16_t>(), destination.desired<uint16_t>())) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(type, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(slot, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(destination, DESIRED_ATTRIBUTE);
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_all_users_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        if (!command_class_user_credential_rules::all_users_checksum_get_is_supported(args.node.parent())) {
            return SL_STATUS_FAIL;
        }
        return args.get_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_user_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto identifier = desired_child(args.node, static_cast<attribute_store_type_t>(user_checksum_get_group_attributes_t::user_unique_identifier));
        if (!identifier.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_credential_rules::user_checksum_get_is_supported(args.node.parent(), identifier.desired<uint16_t>())) {
            return SL_STATUS_FAIL;
        }
        args.get_frame_generator->add_value(identifier, DESIRED_ATTRIBUTE);
        return args.get_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_credential_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto type = desired_child(args.node, static_cast<attribute_store_type_t>(credential_checksum_get_group_attributes_t::credential_type));
        if (!type.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        if (!command_class_user_credential_rules::credential_checksum_get_is_supported(args.node.parent(), type.desired<uint8_t>())) {
            return SL_STATUS_FAIL;
        }
        args.get_frame_generator->add_value(type, DESIRED_ATTRIBUTE);
        return args.get_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_admin_pin_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto length = desired_child(args.node, static_cast<attribute_store_type_t>(admin_pin_code_set_group_attributes_t::admin_pin_code_length));
        auto code   = desired_child(args.node, static_cast<attribute_store_type_t>(admin_pin_code_set_group_attributes_t::admin_code));
        if (!length.is_valid()) {
            return SL_STATUS_NOT_READY;
        }
        std::vector<uint8_t> admin_code;
        if (code.is_valid()) {
            admin_code = code.desired<std::vector<uint8_t>>();
        }
        if (!command_class_user_credential_rules::admin_pin_code_set_is_valid(args.node.parent(), length.desired<uint8_t>(), admin_code)) {
            return SL_STATUS_FAIL;
        }
        args.set_frame_generator->add_value(length, DESIRED_ATTRIBUTE);
        if (code.is_valid()) {
            args.set_frame_generator->add_value(code, DESIRED_ATTRIBUTE);
        }
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_user_credential::on_admin_pin_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        if (!command_class_user_credential_rules::admin_pin_code_get_is_supported(args.node.parent())) {
            return SL_STATUS_FAIL;
        }
        return args.get_frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
