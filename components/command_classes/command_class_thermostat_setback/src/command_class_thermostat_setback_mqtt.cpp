
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
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_setback_mqtt";

    command_class_thermostat_setback_mqtt::command_class_thermostat_setback_mqtt()
    {

        mqtt_callback_map.insert({"ThermostatSetbackGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatSetbackSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_set_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_get_group_attributes_t::THERMOSTAT_SETBACK_GET_GROUP));
        command_class_thermostat_setback_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t setback_type  = 0;
        uint8_t setback_state = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("setback_type", setback_type);
        parser.parse("setback_state", setback_state);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::THERMOSTAT_SETBACK_SET_GROUP));
        auto type_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_type));
        type_node.set_desired<uint8_t>(setback_type);
        auto state_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_state));
        state_node.set_desired<uint8_t>(setback_state);

        command_class_thermostat_setback_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
