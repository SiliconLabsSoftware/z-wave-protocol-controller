
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
#include "command_class_thermostat_mode.hpp"

#include "command_class_thermostat_mode_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_mode_attribute_store";

    command_class_thermostat_mode_attribute_store::command_class_thermostat_mode_attribute_store() {}

    sl_status_t command_class_thermostat_mode_attribute_store::on_thermostat_mode_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_mode_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_report_group_attributes_t::THERMOSTAT_MODE_REPORT_GROUP));
        auto mode_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_report_group_attributes_t::mode));

        if (mode_node.reported_exists()) {
            m_thermostat_mode_reported_before_update = mode_node.reported<uint8_t>();
        } else {
            m_thermostat_mode_reported_before_update = std::nullopt;
        }

        uint8_t mode = 0;
        mode         = get_value_or_default(attribute_map, "mode", mode);
        mode_node.set_reported<uint8_t>(mode);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_mode_attribute_store::on_thermostat_mode_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_mode_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_supported_report_group_attributes_t::THERMOSTAT_MODE_SUPPORTED_REPORT_GROUP));

        thermostat_mode_supported_report_bit_mask_t bit_mask = {};
        bit_mask                                             = get_value_or_default(attribute_map, "bit_mask", bit_mask);
        auto bit_mask_node                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_supported_report_group_attributes_t::bit_mask));
        bit_mask_node.set_reported<thermostat_mode_supported_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class