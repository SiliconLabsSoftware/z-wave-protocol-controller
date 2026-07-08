
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
#include "command_class_basic.hpp"

#include "command_class_basic_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_basic_attribute_store";

    command_class_basic_attribute_store::command_class_basic_attribute_store() {}

    sl_status_t command_class_basic_attribute_store::on_basic_report_received_store(attribute_store::attribute endpoint_node, command_class_basic_attribute_map_t attribute_map)
    {
        // Extract values from attribute map
        basic_report_current_value_t current_value = 0;
        current_value                              = get_value_or_default(attribute_map, "current_value", current_value);

        basic_report_duration_t duration = 0;
        duration                         = get_value_or_default(attribute_map, "duration", duration);

        basic_report_target_value_t target_value = 0;
        target_value                             = get_value_or_default(attribute_map, "target_value", target_value);

        // Find or create the report group node
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(basic_report_group_attributes_t::BASIC_REPORT_GROUP));

        // Store the values
        auto current_value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(basic_report_group_attributes_t::current_value));
        current_value_node.set_reported<basic_report_current_value_t>(current_value);

        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(basic_report_group_attributes_t::duration));
        duration_node.set_reported<basic_report_duration_t>(duration);

        auto target_value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(basic_report_group_attributes_t::target_value));
        target_value_node.set_reported<basic_report_target_value_t>(target_value);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class