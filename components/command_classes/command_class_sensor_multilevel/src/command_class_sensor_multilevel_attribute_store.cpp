
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
#include "command_class_sensor_multilevel.hpp"

#include "command_class_sensor_multilevel_attribute_store.hpp"
#include "command_class_sensor_multilevel_constants.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_sensor_multilevel_attribute_store";

    command_class_sensor_multilevel_attribute_store::command_class_sensor_multilevel_attribute_store() {}

    attribute_store::attribute command_class_sensor_multilevel_attribute_store::find_group_by_sensor_type(attribute_store::attribute endpoint_node, attribute_store_type_t group_type, attribute_store_type_t sensor_type_attr, uint8_t sensor_type)
    {
        for (auto group_node: endpoint_node.children(group_type)) {
            auto type_node = group_node.child_by_type(sensor_type_attr);
            if (type_node.is_valid() && type_node.reported_exists() && type_node.reported<uint8_t>() == sensor_type) {
                return group_node;
            }
        }
        return attribute_store::attribute(ATTRIBUTE_STORE_INVALID_NODE);
    }

    std::vector<uint8_t> command_class_sensor_multilevel_attribute_store::get_supported_sensor_bit_mask(attribute_store::attribute endpoint_node)
    {
        auto group_node = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(sensor_multilevel_supported_sensor_report_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_SENSOR_REPORT_GROUP));
        if (!group_node.is_valid()) {
            return {};
        }
        auto mask_node = group_node.child_by_type(static_cast<attribute_store_type_t>(sensor_multilevel_supported_sensor_report_group_attributes_t::bit_mask));
        if (!mask_node.is_valid() || !mask_node.reported_exists()) {
            return {};
        }
        return mask_node.reported<std::vector<uint8_t>>();
    }

    bool command_class_sensor_multilevel_attribute_store::get_supported_scale_bit_mask_for_sensor_type(attribute_store::attribute endpoint_node, uint8_t sensor_type, uint8_t &out_scale_bit_mask)
    {
        auto group_node = find_group_by_sensor_type(endpoint_node,
                                                    static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_SCALE_REPORT_GROUP),
                                                    static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::sensor_type),
                                                    sensor_type);
        if (!group_node.is_valid()) {
            return false;
        }
        auto mask_node = group_node.child_by_type(static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::scale_bit_mask));
        if (!mask_node.is_valid() || !mask_node.reported_exists()) {
            return false;
        }
        out_scale_bit_mask = mask_node.reported<uint8_t>();
        return true;
    }

    bool command_class_sensor_multilevel_attribute_store::has_report_for_sensor_type(attribute_store::attribute endpoint_node, uint8_t sensor_type)
    {
        auto group_node = find_group_by_sensor_type(endpoint_node, static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::SENSOR_MULTILEVEL_REPORT_GROUP), static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_type), sensor_type);
        if (!group_node.is_valid()) {
            return false;
        }
        auto value_node = group_node.child_by_type(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_value));
        return value_node.is_valid() && value_node.reported_exists();
    }

    sl_status_t command_class_sensor_multilevel_attribute_store::on_sensor_multilevel_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_multilevel_attribute_map_t attribute_map)
    {
        sensor_multilevel_report_sensor_type_t sensor_type = 0;
        sensor_type                                        = get_value_or_default(attribute_map, "sensor_type", sensor_type);

        const auto group_type = static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::SENSOR_MULTILEVEL_REPORT_GROUP);
        const auto type_attr  = static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_type);
        attribute_store::attribute group_node(ATTRIBUTE_STORE_INVALID_NODE);
        // Versions 1-4 keep a single report group, which interview marks as required before the Get.
        if (endpoint_supported_version(endpoint_node) < command_class_sensor_multilevel_constants::SUPPORTED_GET_MIN_VERSION) {
            group_node = endpoint_node.emplace_node(group_type);
        } else {
            group_node = find_group_by_sensor_type(endpoint_node, group_type, type_attr, sensor_type);
            if (!group_node.is_valid()) {
                group_node = endpoint_node.add_node(group_type);
            }
        }

        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_type)).set_reported<uint8_t>(sensor_type);

        // The parsed struct_byte fields are masked but not shifted; decode them through the generated union.
        sensor_multilevel_report_level_t level;
        level.value = get_value_or_default<uint8_t>(attribute_map, "size", 0) | get_value_or_default<uint8_t>(attribute_map, "scale", 0) | get_value_or_default<uint8_t>(attribute_map, "precision", 0);

        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::size)).set_reported<uint8_t>(level.flags.sensor_multilevel_report_size);
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::scale)).set_reported<uint8_t>(level.flags.sensor_multilevel_report_scale);
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::precision)).set_reported<uint8_t>(level.flags.sensor_multilevel_report_precision);

        sensor_multilevel_report_sensor_value_t sensor_value = {};
        sensor_value                                         = get_value_or_default(attribute_map, "sensor_value", sensor_value);
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_report_group_attributes_t::sensor_value)).set_reported<sensor_multilevel_report_sensor_value_t>(sensor_value);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_multilevel_attribute_store::on_sensor_multilevel_supported_sensor_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_multilevel_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_sensor_report_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_SENSOR_REPORT_GROUP));

        sensor_multilevel_supported_sensor_report_bit_mask_t bit_mask = {};
        bit_mask                                                      = get_value_or_default(attribute_map, "bit_mask", bit_mask);
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_sensor_report_group_attributes_t::bit_mask)).set_reported<sensor_multilevel_supported_sensor_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_multilevel_attribute_store::on_sensor_multilevel_supported_scale_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_multilevel_attribute_map_t attribute_map)
    {
        sensor_multilevel_supported_scale_report_sensor_type_t sensor_type = 0;
        sensor_type                                                        = get_value_or_default(attribute_map, "sensor_type", sensor_type);

        const auto group_type = static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::SENSOR_MULTILEVEL_SUPPORTED_SCALE_REPORT_GROUP);
        auto group_node       = find_group_by_sensor_type(endpoint_node, group_type, static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::sensor_type), sensor_type);
        if (!group_node.is_valid()) {
            group_node = endpoint_node.add_node(group_type);
        }

        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::sensor_type)).set_reported<uint8_t>(sensor_type);

        // scale_bit_mask occupies the low nibble, so the masked value needs no shift.
        uint8_t scale_bit_mask = 0;
        scale_bit_mask         = get_value_or_default(attribute_map, "scale_bit_mask", scale_bit_mask);
        group_node.emplace_node(static_cast<attribute_store_type_t>(sensor_multilevel_supported_scale_report_group_attributes_t::scale_bit_mask)).set_reported<uint8_t>(scale_bit_mask);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
