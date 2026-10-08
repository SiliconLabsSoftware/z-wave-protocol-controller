
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

#include "command_class_thermostat_setback.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_setback";

    command_class_thermostat_setback::command_class_thermostat_setback() {}

    void command_class_thermostat_setback::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)supported_version;

        invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::THERMOSTAT_SETBACK_REPORT_GROUP));

        auto report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::THERMOSTAT_SETBACK_REPORT_GROUP));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_type)));
        cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_state)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_get_group_attributes_t::THERMOSTAT_SETBACK_GET_GROUP)));
    }

    sl_status_t command_class_thermostat_setback::on_thermostat_setback_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto setback_type_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_type));
        auto setback_state_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_state));
        if (!setback_type_node.desired_exists() || !setback_state_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        thermostat_setback_set_properties1_t properties1;
        properties1.value                                     = 0;
        properties1.flags.thermostat_setback_set_setback_type = setback_type_node.desired<uint8_t>();
        frame_generator->add_raw_byte(properties1.value);
        frame_generator->add_raw_byte(setback_state_node.desired<uint8_t>());

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
