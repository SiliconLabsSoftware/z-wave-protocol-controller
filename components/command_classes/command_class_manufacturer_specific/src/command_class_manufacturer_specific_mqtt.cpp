
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
#include "command_class_manufacturer_specific.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_manufacturer_specific_mqtt";

    command_class_manufacturer_specific_mqtt::command_class_manufacturer_specific_mqtt()
    {

        mqtt_callback_map.insert({"ManufacturerSpecificGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_manufacturer_specific_mqtt::mqtt_on_manufacturer_specific_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"DeviceSpecificGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_manufacturer_specific_mqtt::mqtt_on_device_specific_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_manufacturer_specific_mqtt::mqtt_on_manufacturer_specific_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(manufacturer_specific_get_group_attributes_t::MANUFACTURER_SPECIFIC_GET_GROUP));
        command_class_manufacturer_specific_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_manufacturer_specific_mqtt::mqtt_on_device_specific_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t device_id_type = 0;

        if (!payload.empty()) {
            mqtt_payload_parser parser {payload, LOG_TAG.data()};
            parser.parse_nested("properties1").parse("device_id_type", device_id_type);
            if (parser.status() != SL_STATUS_OK) {
                return parser.status();
            }
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_get_group_attributes_t::DEVICE_SPECIFIC_GET_GROUP));

        auto device_id_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_get_group_attributes_t::device_id_type));
        device_id_type_node.set_desired(device_id_type);

        command_class_manufacturer_specific_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class