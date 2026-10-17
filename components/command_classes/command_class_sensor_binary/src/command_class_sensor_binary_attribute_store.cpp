
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

#include "command_class_sensor_binary.hpp"
#include "command_class_sensor_binary_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_sensor_binary_attribute_store";

    command_class_sensor_binary_attribute_store::command_class_sensor_binary_attribute_store() {}

    attribute_store::attribute command_class_sensor_binary_attribute_store::find_report_group_by_sensor_type(attribute_store::attribute endpoint_node, uint8_t sensor_type)
    {
        const auto report_type    = static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP);
        const auto type_attr_type = static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type);
        for (auto group: endpoint_node.children(report_type)) {
            auto type_node = group.child_by_type(type_attr_type);
            if (type_node.is_valid() && type_node.reported_exists() && type_node.reported<uint8_t>() == sensor_type) {
                return group;
            }
        }
        return attribute_store::attribute(ATTRIBUTE_STORE_INVALID_NODE);
    }

    attribute_store::attribute command_class_sensor_binary_attribute_store::find_or_create_report_group_by_sensor_type(attribute_store::attribute endpoint_node, uint8_t sensor_type)
    {
        auto group = find_report_group_by_sensor_type(endpoint_node, sensor_type);
        if (group.is_valid()) {
            return group;
        }
        group = endpoint_node.add_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type)).set_reported<uint8_t>(sensor_type);
        return group;
    }

    sl_status_t command_class_sensor_binary_attribute_store::on_sensor_binary_supported_sensor_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_binary_attribute_map_t attribute_map)
    {
        sensor_binary_supported_sensor_report_bit_mask_t bit_mask = {};
        bit_mask                                                  = get_value_or_default(attribute_map, "bit_mask", bit_mask);

        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask)).set_reported<sensor_binary_supported_sensor_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_binary_attribute_store::on_sensor_binary_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_binary_attribute_map_t attribute_map)
    {
        sensor_binary_report_sensor_type_t sensor_type   = 0;
        sensor_binary_report_sensor_value_t sensor_value = 0;
        sensor_type                                      = get_value_or_default(attribute_map, "sensor_type", sensor_type);
        sensor_value                                     = get_value_or_default(attribute_map, "sensor_value", sensor_value);

        auto group = find_or_create_report_group_by_sensor_type(endpoint_node, sensor_type);
        group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).set_reported<sensor_binary_report_sensor_value_t>(sensor_value);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class