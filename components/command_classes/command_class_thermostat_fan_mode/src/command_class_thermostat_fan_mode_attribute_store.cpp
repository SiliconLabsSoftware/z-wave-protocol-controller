
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
#include "command_class_thermostat_fan_mode.hpp"

#include "command_class_thermostat_fan_mode_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_fan_mode_attribute_store";

    command_class_thermostat_fan_mode_attribute_store::command_class_thermostat_fan_mode_attribute_store() {}

    sl_status_t command_class_thermostat_fan_mode_attribute_store::on_thermostat_fan_mode_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_fan_mode_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::THERMOSTAT_FAN_MODE_REPORT_GROUP));

        uint8_t fan_mode   = 0;
        fan_mode           = get_value_or_default(attribute_map, "fan_mode", fan_mode);
        auto fan_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::fan_mode));
        fan_mode_node.set_reported<uint8_t>(fan_mode);

        uint8_t off   = 0;
        off           = get_value_or_default(attribute_map, "off", off);
        auto off_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::off));
        off_node.set_reported<uint8_t>(off);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_fan_mode_attribute_store::on_thermostat_fan_mode_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_fan_mode_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_report_group_attributes_t::THERMOSTAT_FAN_MODE_SUPPORTED_REPORT_GROUP));

        thermostat_fan_mode_supported_report_bit_mask_t bit_mask = {};
        bit_mask                                                 = get_value_or_default(attribute_map, "bit_mask", bit_mask);
        auto bit_mask_node                                       = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_report_group_attributes_t::bit_mask));
        bit_mask_node.set_reported<thermostat_fan_mode_supported_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class