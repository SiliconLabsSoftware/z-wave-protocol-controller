
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
#include "command_class_switch_binary.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_binary_mqtt";

    command_class_switch_binary_mqtt::command_class_switch_binary_mqtt()
    {

        mqtt_callback_map.insert({"SwitchBinaryGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_binary_mqtt::mqtt_on_switch_binary_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchBinarySet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_switch_binary_mqtt::mqtt_on_switch_binary_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_get_group_attributes_t::SWITCH_BINARY_GET_GROUP));
        group_node.clear_reported();

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t target_value = 0;
        uint8_t duration     = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("target_value", target_value).parse("duration", duration);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));

        auto target_value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::target_value));
        target_value_node.set_desired(target_value);

        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::duration));
        duration_node.set_desired(duration);

        command_class_switch_binary_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class