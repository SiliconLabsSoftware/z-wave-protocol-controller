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
#include "command_class_switch_color.hpp"

#include "command_class_switch_color_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_color_attribute_store";

    command_class_switch_color_attribute_store::command_class_switch_color_attribute_store() {}

    sl_status_t command_class_switch_color_attribute_store::on_switch_color_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_switch_color_attribute_map_t attribute_map)
    {
        switch_color_supported_report_color_component_mask_t color_component_mask = 0;
        color_component_mask                                                      = get_value_or_default(attribute_map, "color_component_mask", color_component_mask);

        auto report_group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_supported_report_group_attributes_t::SWITCH_COLOR_SUPPORTED_REPORT_GROUP));

        auto color_component_mask_node = report_group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_supported_report_group_attributes_t::color_component_mask));
        color_component_mask_node.set_reported<switch_color_supported_report_color_component_mask_t>(color_component_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_color_attribute_store::on_switch_color_report_received_store(attribute_store::attribute endpoint_node, command_class_switch_color_attribute_map_t attribute_map)
    {
        switch_color_report_color_component_id_t color_component_id = 0;
        color_component_id                                          = get_value_or_default(attribute_map, "color_component_id", color_component_id);

        switch_color_report_current_value_t current_value = 0;
        current_value                                     = get_value_or_default(attribute_map, "current_value", current_value);

        switch_color_report_target_value_t target_value = 0;
        target_value                                    = get_value_or_default(attribute_map, "target_value", target_value);

        switch_color_report_duration_t duration = 0;
        duration                                = get_value_or_default(attribute_map, "duration", duration);

        attribute_store::attribute report_group_node;
        for (auto child: endpoint_node.children(static_cast<attribute_store_type_t>(switch_color_report_group_attributes_t::SWITCH_COLOR_REPORT_GROUP))) {
            auto ccid_node = child.child_by_type(static_cast<attribute_store_type_t>(switch_color_report_group_attributes_t::color_component_id));
            if (ccid_node.is_valid() && ccid_node.reported_exists() && ccid_node.reported<uint8_t>() == color_component_id) {
                report_group_node = child;
                break;
            }
        }
        if (!report_group_node.is_valid()) {
            report_group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_report_group_attributes_t::SWITCH_COLOR_REPORT_GROUP));
            auto ccid_node    = report_group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_report_group_attributes_t::color_component_id));
            ccid_node.set_reported<switch_color_report_color_component_id_t>(color_component_id);
        }

        auto current_value_node = report_group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_report_group_attributes_t::current_value));
        current_value_node.set_reported<switch_color_report_current_value_t>(current_value);

        auto target_value_node = report_group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_report_group_attributes_t::target_value));
        target_value_node.set_reported<switch_color_report_target_value_t>(target_value);

        auto duration_node = report_group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_report_group_attributes_t::duration));
        duration_node.set_reported<switch_color_report_duration_t>(duration);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class