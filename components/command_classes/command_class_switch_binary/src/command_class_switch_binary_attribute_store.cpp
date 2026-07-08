
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
#include "command_class_switch_binary.hpp"

#include "command_class_switch_binary_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_binary_attribute_store";

    command_class_switch_binary_attribute_store::command_class_switch_binary_attribute_store() {}

    sl_status_t command_class_switch_binary_attribute_store::on_switch_binary_report_received_store(attribute_store::attribute endpoint_node, command_class_switch_binary_attribute_map_t attribute_map)
    {

        // TODOX: Add support for other attributes
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));

        switch_binary_report_current_value_t current_value = 0;
        current_value                                      = get_value_or_default(attribute_map, "current_value", current_value);
        auto current_value_node                            = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value));
        current_value_node.set_reported<switch_binary_report_current_value_t>(current_value);

        switch_binary_report_target_value_t target_value = 0;
        target_value                                     = get_value_or_default(attribute_map, "target_value", target_value);
        auto target_value_node                           = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::target_value));
        target_value_node.set_reported<switch_binary_report_target_value_t>(target_value);

        switch_binary_report_duration_t duration = 0;
        duration                                 = get_value_or_default(attribute_map, "duration", duration);
        auto duration_node                       = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::duration));
        duration_node.set_reported<switch_binary_report_duration_t>(duration);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class