
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
#include "command_class_barrier_operator.hpp"
#include "attribute_resolver.h"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "command_class_barrier_operator_constants.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_barrier_operator";

    using namespace command_class_barrier_operator_constants;

    bool command_class_barrier_operator::is_supported_subsystem_type(uint8_t subsystem_type)
    {
        return subsystem_type == BARRIER_OPERATOR_SUBSYSTEM_AUDIBLE || subsystem_type == BARRIER_OPERATOR_SUBSYSTEM_VISUAL;
    }

    std::optional<uint8_t> command_class_barrier_operator::next_supported_subsystem(const std::vector<uint8_t> &bit_mask, uint8_t after_subsystem_type)
    {
        for (uint8_t subsystem_type = static_cast<uint8_t>(after_subsystem_type + 1); subsystem_type <= BARRIER_OPERATOR_SUBSYSTEM_VISUAL; ++subsystem_type) {
            const size_t bit_index  = static_cast<size_t>(subsystem_type - 1);
            const size_t byte_index = bit_index / 8;
            if (byte_index < bit_mask.size() && (bit_mask[byte_index] & static_cast<uint8_t>(1U << (bit_index % 8))) != 0) {
                return subsystem_type;
            }
        }
        return std::nullopt;
    }

    command_class_barrier_operator::command_class_barrier_operator()
    {
        // Constructor body - can be empty or contain initialization logic
    }

    void command_class_barrier_operator::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)supported_version;

        auto status_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_report_group_attributes_t::BARRIER_OPERATOR_REPORT_GROUP));
        cc_interview_require_attribute(status_report.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_report_group_attributes_t::state)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_get_group_attributes_t::BARRIER_OPERATOR_GET_GROUP)), interview_resolution_options());

        auto capabilities_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::BARRIER_OPERATOR_SIGNAL_SUPPORTED_REPORT_GROUP));
        cc_interview_require_attribute(capabilities_report.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::bit_mask)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_get_group_attributes_t::BARRIER_OPERATOR_SIGNAL_SUPPORTED_GET_GROUP)), interview_resolution_options());
    }

    void command_class_barrier_operator::require_supported_subsystem_reports(attribute_store::attribute endpoint_node, const std::vector<uint8_t> &bit_mask)
    {
        for (uint8_t subsystem_type = BARRIER_OPERATOR_SUBSYSTEM_AUDIBLE; subsystem_type <= BARRIER_OPERATOR_SUBSYSTEM_VISUAL; ++subsystem_type) {
            const size_t bit_index  = static_cast<size_t>(subsystem_type - 1);
            const size_t byte_index = bit_index / 8;
            if (byte_index >= bit_mask.size() || (bit_mask[byte_index] & static_cast<uint8_t>(1U << (bit_index % 8))) == 0) {
                continue;
            }
            auto report_group = find_or_create_report_group_by_subsystem_type(endpoint_node, subsystem_type);
            auto report_state = report_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::subsystem_state));
            if (!report_state.reported_exists()) {
                cc_interview_require_attribute(report_state);
            }
        }
    }

    void command_class_barrier_operator::request_next_supported_subsystem_report(attribute_store::attribute endpoint_node, uint8_t after_subsystem_type)
    {
        const auto bit_mask = get_supported_subsystems(endpoint_node);
        for (auto subsystem_type = next_supported_subsystem(bit_mask, after_subsystem_type); subsystem_type.has_value(); subsystem_type = next_supported_subsystem(bit_mask, subsystem_type.value())) {
            if (has_report_for_subsystem_type(endpoint_node, subsystem_type.value())) {
                continue;
            }
            auto get_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNALING_GET_GROUP));
            get_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::subsystem_type)).set_desired(subsystem_type.value());
            start_group_resolution(get_group, interview_resolution_options());
            return;
        }
    }

    void command_class_barrier_operator::start_default_subsystem_activation(attribute_store::attribute endpoint_node, uint8_t after_subsystem_type)
    {
        const auto bit_mask       = get_supported_subsystems(endpoint_node);
        const auto subsystem_type = next_supported_subsystem(bit_mask, after_subsystem_type);
        if (!subsystem_type.has_value()) {
            return;
        }

        auto set_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNAL_SET_GROUP));
        set_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_type)).set_desired(subsystem_type.value());
        set_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_state)).set_desired(BARRIER_OPERATOR_SUBSYSTEM_ON);
        start_group_resolution(set_group);
        attribute_resolver_set_resolution_listener(set_group, on_default_subsystem_activation_completed);
    }

    void command_class_barrier_operator::on_default_subsystem_activation_completed(attribute_store_node_t set_group_node)
    {
        attribute_resolver_clear_resolution_listener(set_group_node, on_default_subsystem_activation_completed);
        auto set_group      = attribute_store::attribute(set_group_node);
        auto subsystem_type = set_group.child_by_type(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_type));
        if (!subsystem_type.is_valid() || !subsystem_type.desired_exists()) {
            return;
        }
        const auto endpoint_node = attribute_store::attribute(attribute_store_get_first_parent_with_type(set_group_node, ATTRIBUTE_ENDPOINT_ID));
        start_default_subsystem_activation(endpoint_node, subsystem_type.desired<uint8_t>());
    }

    sl_status_t command_class_barrier_operator::on_barrier_operator_signal_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_barrier_operator_attribute_map_t payload)
    {
        (void)connection_info;
        (void)payload;
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        const auto bit_mask = get_supported_subsystems(endpoint);
        require_supported_subsystem_reports(endpoint, bit_mask);
        start_default_subsystem_activation(endpoint, 0);
        request_next_supported_subsystem_report(endpoint, 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator::on_barrier_operator_event_signaling_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_barrier_operator_attribute_map_t payload)
    {
        (void)connection_info;
        if (!cc_interview_is_open(endpoint)) {
            return SL_STATUS_OK;
        }
        uint8_t subsystem_type = 0;
        subsystem_type         = get_value_or_default(payload, "subsystem_type", subsystem_type);
        auto get_group         = endpoint.child_by_type(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNALING_GET_GROUP));
        auto requested_type    = get_group.child_by_type(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::subsystem_type));
        if (!requested_type.is_valid() || !requested_type.desired_exists() || requested_type.desired<uint8_t>() != subsystem_type) {
            return SL_STATUS_OK;
        }

        request_next_supported_subsystem_report(endpoint, subsystem_type);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator::on_barrier_operator_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node   = args.node;
        auto target_value = group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_set_group_attributes_t::target_value));
        if (!target_value.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        const auto value = target_value.desired<uint8_t>();
        if (value != BARRIER_OPERATOR_TARGET_CLOSE && value != BARRIER_OPERATOR_TARGET_OPEN) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        args.set_frame_generator->add_value(target_value, DESIRED_ATTRIBUTE);
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_barrier_operator::on_barrier_operator_event_signal_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node      = args.node;
        auto subsystem_type  = group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_type));
        auto subsystem_state = group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_state));
        if (!subsystem_type.desired_exists() || !subsystem_state.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        const auto type  = subsystem_type.desired<uint8_t>();
        const auto state = subsystem_state.desired<uint8_t>();
        if (!is_supported_subsystem_type(type) || (state != BARRIER_OPERATOR_SUBSYSTEM_OFF && state != BARRIER_OPERATOR_SUBSYSTEM_ON)) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        args.set_frame_generator->add_value(subsystem_type, DESIRED_ATTRIBUTE);
        args.set_frame_generator->add_value(subsystem_state, DESIRED_ATTRIBUTE);
        return args.set_frame_generator->generate_frame();
    }

    sl_status_t command_class_barrier_operator::on_barrier_operator_event_signaling_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node     = args.node;
        auto subsystem_type = group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::subsystem_type));
        if (!subsystem_type.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        const auto type = subsystem_type.desired<uint8_t>();
        if (!is_supported_subsystem_type(type)) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        args.get_frame_generator->add_value(subsystem_type, DESIRED_ATTRIBUTE);
        return args.get_frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
