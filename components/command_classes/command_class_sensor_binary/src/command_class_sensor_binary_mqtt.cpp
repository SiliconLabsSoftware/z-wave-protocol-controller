
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
#include "command_class_sensor_binary.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_sensor_binary_mqtt";

    command_class_sensor_binary_mqtt::command_class_sensor_binary_mqtt()
    {
        mqtt_callback_map.insert({"SensorBinaryGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SensorBinarySupportedGetSensor", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_supported_get_sensor_command(endpoint_node, payload);
                                  }});
        mqtt_register_command_handler();
    }

    sl_status_t command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t sensor_type = 0xFF;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("sensor_type", sensor_type);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto get_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
        auto type_node = get_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
        type_node.set_desired(sensor_type);

        command_class_sensor_binary_core::start_group_resolution(get_group);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_supported_get_sensor_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_get_sensor_group_attributes_t::SENSOR_BINARY_SUPPORTED_GET_SENSOR_GROUP));
        command_class_sensor_binary_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class