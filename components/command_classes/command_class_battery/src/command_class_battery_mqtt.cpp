
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
#include "command_class_battery.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

namespace zwave_command_class
{
    // Log tag
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_battery_mqtt";

    command_class_battery_mqtt::command_class_battery_mqtt()
    {

        mqtt_callback_map.insert({"BatteryGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_battery_mqtt::mqtt_on_battery_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"BatteryHealthGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_battery_mqtt::mqtt_on_battery_health_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_battery_mqtt::mqtt_on_battery_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(battery_get_group_attributes_t::BATTERY_GET_GROUP));
        command_class_battery_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_battery_mqtt::mqtt_on_battery_health_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(battery_health_get_group_attributes_t::BATTERY_HEALTH_GET_GROUP));
        command_class_battery_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class