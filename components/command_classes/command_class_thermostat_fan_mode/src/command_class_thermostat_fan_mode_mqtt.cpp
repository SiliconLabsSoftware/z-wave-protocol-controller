
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

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_fan_mode_mqtt";

    command_class_thermostat_fan_mode_mqtt::command_class_thermostat_fan_mode_mqtt()
    {

        mqtt_callback_map.insert({"ThermostatFanModeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatFanModeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatFanModeSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_supported_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_get_group_attributes_t::THERMOSTAT_FAN_MODE_GET_GROUP));
        command_class_thermostat_fan_mode_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t fan_mode = 0;
        uint8_t off      = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("fan_mode", fan_mode);
        parser.parse_nested("properties1").parse("off", off);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
        auto fan_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::fan_mode));
        fan_mode_node.set_desired(fan_mode);
        auto off_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::off));
        off_node.set_desired(off);

        command_class_thermostat_fan_mode_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_get_group_attributes_t::THERMOSTAT_FAN_MODE_SUPPORTED_GET_GROUP));
        command_class_thermostat_fan_mode_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class