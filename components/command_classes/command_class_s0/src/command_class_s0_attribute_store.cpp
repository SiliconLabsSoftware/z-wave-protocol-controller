
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
#include <vector>

// Base class
#include "command_class_s0.hpp"

#include "command_class_s0_attribute_store.hpp"
#include "command_class_s0_types.hpp"
#include "attribute_store_helper.h"
#include "zwave_command_class_indices.h"

namespace zwave_command_class
{
    // Log tag
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_s0_attribute_store";

    command_class_s0_attribute_store::command_class_s0_attribute_store()
    {
        register_attribute_types(attributes);
    }

    static void strip_controlled_command_classes_from_supported_list(s0_commands_supported_report_cc_list_t &supported_cc_list)
    {
        size_t index = 0;
        while (index < supported_cc_list.size()) {
            if (supported_cc_list[index] == COMMAND_CLASS_CONTROL_MARK) {
                supported_cc_list.erase(supported_cc_list.begin() + static_cast<std::ptrdiff_t>(index), supported_cc_list.end());
                return;
            }
            if (supported_cc_list[index] >= EXTENDED_COMMAND_CLASS_IDENTIFIER_START) {
                if (index + 1 >= supported_cc_list.size()) {
                    break;
                }
                index += 2;
            } else {
                index += 1;
            }
        }
    }

    sl_status_t command_class_s0_attribute_store::on_s0_commands_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_s0_attribute_map_t attribute_map)
    {
        auto group_node         = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(s0_commands_supported_report_group_attributes_t::S0_COMMANDS_SUPPORTED_REPORT_GROUP));
        auto command_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(s0_commands_supported_report_group_attributes_t::command_class));

        reports_to_follow_t reports_to_follow          = get_value_or_default(attribute_map, "reports_to_follow", reports_to_follow_t(0));
        reports_to_follow_t previous_reports_to_follow = get_value_or_default(attribute_map, "previous_reports_to_follow", reports_to_follow_t(0));

        attribute_store::attribute reports_to_follow_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(s0_reports_to_follow_attributes_t::S0_REPORTS_TO_FOLLOW_ATTRIBUTE));

        const bool has_supported_cc_list = attribute_map.contains("supported_cc_list");
        const bool continuing_sequence   = previous_reports_to_follow > 0 && reports_to_follow < previous_reports_to_follow;
        const bool starting_new_sequence = !continuing_sequence;

        if (continuing_sequence && has_supported_cc_list) {
            s0_commands_supported_report_cc_list_t supported_cc_list = get_value_or_default(attribute_map, "supported_cc_list", s0_commands_supported_report_cc_list_t {});
            attribute_store_append_to_reported(command_class_node, supported_cc_list.data(), static_cast<uint8_t>(supported_cc_list.size()));
        } else if (has_supported_cc_list && starting_new_sequence) {
            s0_commands_supported_report_cc_list_t supported_cc_list = get_value_or_default(attribute_map, "supported_cc_list", s0_commands_supported_report_cc_list_t {});
            command_class_node.set_reported<s0_commands_supported_report_cc_list_t>(supported_cc_list);
        } else if (starting_new_sequence && previous_reports_to_follow == 0) {
            command_class_node.set_reported<s0_commands_supported_report_cc_list_t>(s0_commands_supported_report_cc_list_t {});
        }

        // Match S2: supported CCs precede COMMAND_CLASS_CONTROL_MARK (0xEF); controlled CCs follow.
        // Strip only when the segmented report is complete so the mark is not dropped mid-sequence.
        if (reports_to_follow == 0 && command_class_node.reported_exists()) {
            s0_commands_supported_report_cc_list_t stored_supported_cc_list = command_class_node.reported<s0_commands_supported_report_cc_list_t>();
            strip_controlled_command_classes_from_supported_list(stored_supported_cc_list);
            command_class_node.set_reported<s0_commands_supported_report_cc_list_t>(stored_supported_cc_list);
        }

        reports_to_follow_node.set_reported(reports_to_follow);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
