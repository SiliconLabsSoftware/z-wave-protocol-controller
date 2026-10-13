
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

#include "command_class_thermostat_fan_state.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_fan_state";

    command_class_thermostat_fan_state::command_class_thermostat_fan_state() {}

    void command_class_thermostat_fan_state::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)supported_version;

        invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::THERMOSTAT_FAN_STATE_REPORT_GROUP));

        auto report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::THERMOSTAT_FAN_STATE_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::fan_operating_state)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_state_get_group_attributes_t::THERMOSTAT_FAN_STATE_GET_GROUP)));
    }

}  // namespace zwave_command_class
