
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
#include "command_class_thermostat_setpoint.hpp"

#include "command_class_thermostat_setpoint_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_setpoint_attribute_store";

    command_class_thermostat_setpoint_attribute_store::command_class_thermostat_setpoint_attribute_store() {}

    attribute_store::attribute command_class_thermostat_setpoint_attribute_store::find_report_group_by_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type)
    {
        const auto report_type        = static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::THERMOSTAT_SETPOINT_REPORT_GROUP);
        const auto setpoint_type_attr = static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::setpoint_type);
        for (auto group_node: endpoint_node.children(report_type)) {
            auto type_node = group_node.child_by_type(setpoint_type_attr);
            if (type_node.is_valid() && type_node.reported_exists() && type_node.reported<uint8_t>() == setpoint_type) {
                return group_node;
            }
        }
        return attribute_store::attribute(ATTRIBUTE_STORE_INVALID_NODE);
    }

    attribute_store::attribute command_class_thermostat_setpoint_attribute_store::find_capabilities_report_group_by_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type)
    {
        const auto cap_type           = static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_REPORT_GROUP);
        const auto setpoint_type_attr = static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::setpoint_type);
        for (auto group_node: endpoint_node.children(cap_type)) {
            auto type_node = group_node.child_by_type(setpoint_type_attr);
            if (type_node.is_valid() && type_node.reported_exists() && type_node.reported<uint8_t>() == setpoint_type) {
                return group_node;
            }
        }
        return attribute_store::attribute(ATTRIBUTE_STORE_INVALID_NODE);
    }

    bool command_class_thermostat_setpoint_attribute_store::get_reported_scale_for_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type, uint8_t &out_scale)
    {
        attribute_store::attribute group_node = find_report_group_by_setpoint_type(endpoint_node, setpoint_type);
        if (!group_node.is_valid()) {
            return false;
        }
        auto scale_node = group_node.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::scale));
        if (!scale_node.is_valid() || !scale_node.reported_exists()) {
            return false;
        }
        out_scale = scale_node.reported<uint8_t>();
        return true;
    }

    bool command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_for_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type, std::vector<uint8_t> &out_min_value, std::vector<uint8_t> &out_max_value)
    {
        attribute_store::attribute group_node = find_capabilities_report_group_by_setpoint_type(endpoint_node, setpoint_type);
        if (!group_node.is_valid()) {
            return false;
        }
        auto min_node = group_node.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::min_value));
        auto max_node = group_node.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::maxvalue));
        if (!min_node.is_valid() || !min_node.reported_exists() || !max_node.is_valid() || !max_node.reported_exists()) {
            return false;
        }
        out_min_value = min_node.reported<std::vector<uint8_t>>();
        out_max_value = max_node.reported<std::vector<uint8_t>>();
        return !out_min_value.empty() && !out_max_value.empty();
    }

    sl_status_t command_class_thermostat_setpoint_attribute_store::on_thermostat_setpoint_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_setpoint_attribute_map_t attribute_map)
    {
        uint8_t setpoint_type = 0;
        setpoint_type         = get_value_or_default(attribute_map, "setpoint_type", setpoint_type);

        attribute_store::attribute group_node = find_report_group_by_setpoint_type(endpoint_node, setpoint_type);
        if (!group_node.is_valid()) {
            group_node = endpoint_node.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::THERMOSTAT_SETPOINT_REPORT_GROUP));
        }

        auto type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::setpoint_type));
        type_node.set_reported<uint8_t>(setpoint_type);

        uint8_t size   = 0;
        size           = get_value_or_default(attribute_map, "size", size);
        auto size_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::size));
        size_node.set_reported<uint8_t>(size);

        uint8_t scale   = 0;
        scale           = get_value_or_default(attribute_map, "scale", scale);
        auto scale_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::scale));
        scale_node.set_reported<uint8_t>(scale);

        uint8_t precision   = 0;
        precision           = get_value_or_default(attribute_map, "precision", precision);
        auto precision_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::precision));
        precision_node.set_reported<uint8_t>(precision);

        thermostat_setpoint_report_value_t value = {};
        value                                    = get_value_or_default(attribute_map, "value", value);
        auto value_node                          = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::value));
        value_node.set_reported<thermostat_setpoint_report_value_t>(value);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_attribute_store::on_thermostat_setpoint_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_setpoint_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_report_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_REPORT_GROUP));

        thermostat_setpoint_supported_report_bit_mask_t bit_mask = {};
        bit_mask                                                 = get_value_or_default(attribute_map, "bit_mask", bit_mask);
        auto bit_mask_node                                       = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_report_group_attributes_t::bit_mask));
        bit_mask_node.set_reported<thermostat_setpoint_supported_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_attribute_store::on_thermostat_setpoint_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_setpoint_attribute_map_t attribute_map)
    {
        uint8_t setpoint_type = 0;
        setpoint_type         = get_value_or_default(attribute_map, "setpoint_type", setpoint_type);

        attribute_store::attribute group_node = find_capabilities_report_group_by_setpoint_type(endpoint_node, setpoint_type);
        if (!group_node.is_valid()) {
            group_node = endpoint_node.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_REPORT_GROUP));
        }

        auto type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::setpoint_type));
        type_node.set_reported<uint8_t>(setpoint_type);

        thermostat_setpoint_capabilities_report_min_value_t min_value = {};
        min_value                                                     = get_value_or_default(attribute_map, "min_value", min_value);
        auto min_value_node                                           = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::min_value));
        min_value_node.set_reported<thermostat_setpoint_capabilities_report_min_value_t>(min_value);

        thermostat_setpoint_capabilities_report_maxvalue_t max_value = {};
        max_value                                                    = get_value_or_default(attribute_map, "maxvalue", max_value);
        auto max_value_node                                          = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::maxvalue));
        max_value_node.set_reported<thermostat_setpoint_capabilities_report_maxvalue_t>(max_value);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class