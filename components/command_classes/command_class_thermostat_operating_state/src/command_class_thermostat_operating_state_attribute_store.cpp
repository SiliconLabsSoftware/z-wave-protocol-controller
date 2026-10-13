
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

#include <string_view>

#include "command_class_thermostat_operating_state.hpp"
#include "command_class_thermostat_operating_state_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_operating_state_attribute_store";

    command_class_thermostat_operating_state_attribute_store::command_class_thermostat_operating_state_attribute_store() {}

    sl_status_t command_class_thermostat_operating_state_attribute_store::on_thermostat_operating_state_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_operating_state_attribute_map_t attribute_map)
    {
        uint8_t operating_state = get_value_or_default(attribute_map, "operating_state", static_cast<uint8_t>(0));

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_REPORT_GROUP));
        auto state_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::operating_state));
        state_node.set_reported<uint8_t>(operating_state);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_operating_state_attribute_store::on_thermostat_operating_logging_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_operating_state_attribute_map_t attribute_map)
    {
        auto bit_mask = get_value_or_default(attribute_map, "bit_mask", thermostat_operating_logging_supported_report_bit_mask_t {});

        auto group_node    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT_GROUP));
        auto bit_mask_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::bit_mask));
        bit_mask_node.set_reported<thermostat_operating_logging_supported_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_operating_state_attribute_store::on_thermostat_operating_state_logging_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_operating_state_attribute_map_t attribute_map)
    {
        uint8_t reports_to_follow = get_value_or_default(attribute_map, "reports_to_follow", static_cast<uint8_t>(0));
        auto vg1                  = get_value_or_default(attribute_map, "vg1", thermostat_operating_state_logging_report_vg1_t {});

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_REPORT_GROUP));

        auto reports_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::reports_to_follow));
        reports_node.set_reported<uint8_t>(reports_to_follow);

        std::vector<uint8_t> vg1_marker;
        vg1_marker.reserve(vg1.size());
        for (const auto &item: vg1) {
            vg1_marker.push_back(item.properties1.value);
        }

        auto vg1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::vg1));
        vg1_node.set_reported<std::vector<uint8_t>>(vg1_marker);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
