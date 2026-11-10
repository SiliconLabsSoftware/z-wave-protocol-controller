
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
#include "command_class_barrier_operator.hpp"

#include "command_class_barrier_operator_attribute_store.hpp"

#include "command_class_barrier_operator_generated_types.hpp"
#include "command_class_barrier_operator_constants.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_barrier_operator_attribute_store";

    using namespace command_class_barrier_operator_constants;

    command_class_barrier_operator_attribute_store::command_class_barrier_operator_attribute_store() {}

    attribute_store::attribute command_class_barrier_operator_attribute_store::find_report_group_by_subsystem_type(attribute_store::attribute endpoint_node, uint8_t subsystem_type)
    {
        const auto report_group_type        = static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNALING_REPORT_GROUP);
        const auto subsystem_type_attribute = static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::subsystem_type);
        for (auto report_group: endpoint_node.children(report_group_type)) {
            auto type_node = report_group.child_by_type(subsystem_type_attribute);
            if (type_node.is_valid() && type_node.reported_exists() && type_node.reported<uint8_t>() == subsystem_type) {
                return report_group;
            }
        }
        return attribute_store::attribute(ATTRIBUTE_STORE_INVALID_NODE);
    }

    attribute_store::attribute command_class_barrier_operator_attribute_store::find_or_create_report_group_by_subsystem_type(attribute_store::attribute endpoint_node, uint8_t subsystem_type)
    {
        auto report_group = find_report_group_by_subsystem_type(endpoint_node, subsystem_type);
        if (report_group.is_valid()) {
            return report_group;
        }
        report_group = endpoint_node.add_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNALING_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::subsystem_type)).set_reported<uint8_t>(subsystem_type);
        return report_group;
    }

    bool command_class_barrier_operator_attribute_store::has_report_for_subsystem_type(attribute_store::attribute endpoint_node, uint8_t subsystem_type)
    {
        auto report_group = find_report_group_by_subsystem_type(endpoint_node, subsystem_type);
        if (!report_group.is_valid()) {
            return false;
        }
        auto state_node = report_group.child_by_type(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::subsystem_state));
        return state_node.is_valid() && state_node.reported_exists();
    }

    std::vector<uint8_t> command_class_barrier_operator_attribute_store::get_supported_subsystems(attribute_store::attribute endpoint_node)
    {
        auto report_group = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::BARRIER_OPERATOR_SIGNAL_SUPPORTED_REPORT_GROUP));
        if (!report_group.is_valid()) {
            return {};
        }
        auto bit_mask_node = report_group.child_by_type(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::bit_mask));
        if (!bit_mask_node.is_valid() || !bit_mask_node.reported_exists()) {
            return {};
        }
        return bit_mask_node.reported<std::vector<uint8_t>>();
    }

    sl_status_t command_class_barrier_operator_attribute_store::on_barrier_operator_report_received_store(attribute_store::attribute endpoint_node, command_class_barrier_operator_attribute_map_t attribute_map)
    {
        if (!attribute_map.contains("state")) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        barrier_operator_report_state_t state = 0;
        state                                 = get_value_or_default(attribute_map, "state", state);
        if (state > BARRIER_OPERATOR_STATE_MAXIMUM && state < BARRIER_OPERATOR_STATE_MINIMUM_UNKNOWN) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_report_group_attributes_t::BARRIER_OPERATOR_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_report_group_attributes_t::state)).set_reported(state);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator_attribute_store::on_barrier_operator_signal_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_barrier_operator_attribute_map_t attribute_map)
    {
        barrier_operator_signal_supported_report_bit_mask_t bit_mask {};
        bit_mask = get_value_or_default(attribute_map, "bit_mask", bit_mask);
        if (bit_mask.empty() || bit_mask.size() > BARRIER_OPERATOR_CAPABILITIES_MAXIMUM_SIZE) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::BARRIER_OPERATOR_SIGNAL_SUPPORTED_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::bit_mask)).set_reported(bit_mask);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator_attribute_store::on_barrier_operator_event_signaling_report_received_store(attribute_store::attribute endpoint_node, command_class_barrier_operator_attribute_map_t attribute_map)
    {
        if (!attribute_map.contains("subsystem_type") || !attribute_map.contains("subsystem_state")) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        barrier_operator_event_signaling_report_subsystem_type_t subsystem_type   = 0;
        subsystem_type                                                            = get_value_or_default(attribute_map, "subsystem_type", subsystem_type);
        barrier_operator_event_signaling_report_subsystem_state_t subsystem_state = 0;
        subsystem_state                                                           = get_value_or_default(attribute_map, "subsystem_state", subsystem_state);
        if ((subsystem_type != BARRIER_OPERATOR_SUBSYSTEM_AUDIBLE && subsystem_type != BARRIER_OPERATOR_SUBSYSTEM_VISUAL) || (subsystem_state != BARRIER_OPERATOR_SUBSYSTEM_OFF && subsystem_state != BARRIER_OPERATOR_SUBSYSTEM_ON)) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        auto report_group = find_or_create_report_group_by_subsystem_type(endpoint_node, subsystem_type);
        report_group.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::subsystem_state)).set_reported(subsystem_state);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
