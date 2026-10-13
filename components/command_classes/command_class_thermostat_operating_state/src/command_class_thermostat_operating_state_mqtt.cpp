
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
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_operating_state_mqtt";

    command_class_thermostat_operating_state_mqtt::command_class_thermostat_operating_state_mqtt()
    {

        mqtt_callback_map.insert({"ThermostatOperatingStateGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatOperatingStateLoggingSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_supported_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatOperatingStateLoggingGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_GET_GROUP));
        command_class_thermostat_operating_state_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_supported_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_SUPPORTED_GET_GROUP));
        command_class_thermostat_operating_state_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        mqtt_payload_parser parser {payload, LOG_TAG.data()};

        std::vector<uint8_t> bit_mask;
        parser.parse("bit_mask", bit_mask);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_GET_GROUP));
        auto bit_mask_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::bit_mask));
        bit_mask_node.set_desired<std::vector<uint8_t>>(bit_mask);

        command_class_thermostat_operating_state_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
