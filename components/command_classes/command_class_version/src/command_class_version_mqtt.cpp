
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
#include "command_class_version.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_version_mqtt";

    command_class_version_mqtt::command_class_version_mqtt()
    {

        mqtt_callback_map.insert({"VersionCommandClassGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_version_mqtt::mqtt_on_version_command_class_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"VersionGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_version_mqtt::mqtt_on_version_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"VersionCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_version_mqtt::mqtt_on_version_capabilities_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"VersionZwaveSoftwareGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_version_mqtt::mqtt_on_version_zwave_software_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_version_mqtt::mqtt_on_version_command_class_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t target_value = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("requested_command_class", target_value);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node                   = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_command_class_get_group_attributes_t::VERSION_COMMAND_CLASS_GET_GROUP));
        auto requested_command_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(version_command_class_get_group_attributes_t::requested_command_class));
        requested_command_class_node.set_desired(target_value);
        command_class_version_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version_mqtt::mqtt_on_version_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_get_group_attributes_t::VERSION_GET_GROUP));
        command_class_version_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_version_mqtt::mqtt_on_version_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_capabilities_get_group_attributes_t::VERSION_CAPABILITIES_GET_GROUP));
        command_class_version_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_version_mqtt::mqtt_on_version_zwave_software_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_get_group_attributes_t::VERSION_ZWAVE_SOFTWARE_GET_GROUP));
        command_class_version_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class