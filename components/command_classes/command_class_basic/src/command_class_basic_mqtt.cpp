
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
#include "command_class_basic.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_basic_mqtt";

    command_class_basic_mqtt::command_class_basic_mqtt()
    {

        mqtt_callback_map.insert({"BasicGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_basic_mqtt::mqtt_on_basic_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"BasicSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_basic_mqtt::mqtt_on_basic_set_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_basic_mqtt::mqtt_on_basic_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        // Ensure the get group node exists and trigger query
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(basic_get_group_attributes_t::BASIC_GET_GROUP));
        group_node.clear_reported();
        command_class_basic_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_basic_mqtt::mqtt_on_basic_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t value = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("value", value);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        // Set desired value to trigger frame transmission
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(basic_set_group_attributes_t::BASIC_SET_GROUP));
        auto value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(basic_set_group_attributes_t::value));
        value_node.set_desired(value);
        command_class_basic_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class