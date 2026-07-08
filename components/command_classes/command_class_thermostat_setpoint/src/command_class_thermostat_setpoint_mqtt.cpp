
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
#include "command_class_thermostat_setpoint_constants.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_setpoint_mqtt";

    command_class_thermostat_setpoint_mqtt::command_class_thermostat_setpoint_mqtt()
    {

        mqtt_callback_map.insert({"ThermostatSetpointGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatSetpointReport", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_report_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatSetpointSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatSetpointSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_supported_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatSetpointSupportedReport", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_supported_report_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatSetpointCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_capabilities_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ThermostatSetpointCapabilitiesReport", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_capabilities_report_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t setpoint_type = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("level").parse("setpoint_type", setpoint_type);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
        auto type_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type));
        type_node.set_desired(setpoint_type);
        command_class_thermostat_setpoint_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_report_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "Thermostat setpoint report not implemented");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t setpoint_type = 0;
        uint8_t size          = 0;
        uint8_t scale         = 0;
        uint8_t precision     = 0;
        std::vector<uint8_t> value_bytes;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("level").parse("setpoint_type", setpoint_type);
        parser.parse_nested("level2").parse("size", size).parse("scale", scale).parse("precision", precision);
        parser.parse("value", value_bytes);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        if (!command_class_thermostat_setpoint_constants::is_valid_size(size)) {
            sl_log_warning(LOG_TAG.data(), "ThermostatSetpointSet: invalid size %u; must be 1, 2 or 4", size);
            return SL_STATUS_INVALID_PARAMETER;
        }
        if (!command_class_thermostat_setpoint_constants::is_valid_scale(scale)) {
            sl_log_warning(LOG_TAG.data(), "ThermostatSetpointSet: invalid scale %u; must be 0 (Celsius) or 1 (Fahrenheit)", scale);
            return SL_STATUS_INVALID_PARAMETER;
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
        auto type_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type));
        type_node.set_desired(setpoint_type);
        auto size_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size));
        size_node.set_desired(size);
        auto scale_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale));
        scale_node.set_desired(scale);
        auto precision_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision));
        precision_node.set_desired(precision);
        auto value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value));
        value_node.set_desired(value_bytes);

        command_class_thermostat_setpoint_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_get_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_GET_GROUP));
        command_class_thermostat_setpoint_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_supported_report_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "Thermostat setpoint supported report not implemented");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t setpoint_type = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("setpoint_type", setpoint_type);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
        auto type_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type));
        type_node.set_desired(setpoint_type);
        command_class_thermostat_setpoint_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_capabilities_report_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "Thermostat setpoint capabilities report not implemented");
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class