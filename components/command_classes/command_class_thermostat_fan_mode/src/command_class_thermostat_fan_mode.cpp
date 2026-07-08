
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

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_fan_mode";

    command_class_thermostat_fan_mode::command_class_thermostat_fan_mode() {}

    void command_class_thermostat_fan_mode::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        auto supported_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_get_group_attributes_t::THERMOSTAT_FAN_MODE_SUPPORTED_GET_GROUP));
        start_group_resolution(supported_get_node);

        auto get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_get_group_attributes_t::THERMOSTAT_FAN_MODE_GET_GROUP));
        start_group_resolution(get_node);
    }

    sl_status_t command_class_thermostat_fan_mode::on_thermostat_fan_mode_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto fan_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::fan_mode));
        auto off_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::off));
        if (!fan_mode_node.desired_exists() || !off_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        thermostat_fan_mode_set_properties1_t properties1;
        properties1.value                                  = 0;
        properties1.flags.thermostat_fan_mode_set_fan_mode = fan_mode_node.desired<uint8_t>();
        properties1.flags.thermostat_fan_mode_set_off      = off_node.desired<uint8_t>();
        frame_generator->add_raw_byte(properties1.value);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class