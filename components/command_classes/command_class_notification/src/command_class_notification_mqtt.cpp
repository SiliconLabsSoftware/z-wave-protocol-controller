
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
#include "command_class_notification.hpp"
#include "command_class_notification_core.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_notification_mqtt";

    command_class_notification_mqtt::command_class_notification_mqtt()
    {

        mqtt_callback_map.insert({"NotificationGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_notification_mqtt::mqtt_on_notification_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"NotificationSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_notification_mqtt::mqtt_on_notification_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"NotificationSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_notification_mqtt::mqtt_on_notification_supported_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"EventSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_notification_mqtt::mqtt_on_event_supported_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_notification_mqtt::mqtt_on_notification_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t v1_alarm_type     = 0;
        uint8_t notification_type = 0xFF;  // 0xFF = first supported type per spec
        uint8_t event             = 0;

        if (!payload.empty()) {
            mqtt_payload_parser parser {payload, LOG_TAG.data()};
            parser.parse_optional("v1_alarm_type", v1_alarm_type).parse_optional("notification_type", notification_type).parse_optional("event", event);
            if (parser.status() != SL_STATUS_OK) {
                return parser.status();
            }
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::NOTIFICATION_GET_GROUP));

        auto v1_alarm_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::v1_alarm_type));
        v1_alarm_type_node.set_desired(v1_alarm_type);

        auto notification_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::notification_type));
        notification_type_node.set_desired(notification_type);

        auto event_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::event));
        event_node.set_desired(event);

        command_class_notification_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification_mqtt::mqtt_on_notification_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t notification_type   = 0;
        uint8_t notification_status = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("notification_type", notification_type).parse("notification_status", notification_status);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(notification_set_group_attributes_t::NOTIFICATION_SET_GROUP));

        auto notification_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_set_group_attributes_t::notification_type));
        notification_type_node.set_desired(notification_type);

        auto notification_status_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_set_group_attributes_t::notification_status));
        notification_status_node.set_desired(notification_status);

        command_class_notification_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification_mqtt::mqtt_on_notification_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(notification_supported_get_group_attributes_t::NOTIFICATION_SUPPORTED_GET_GROUP));

        command_class_notification_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification_mqtt::mqtt_on_event_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t notification_type = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("notification_type", notification_type);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(event_supported_get_group_attributes_t::EVENT_SUPPORTED_GET_GROUP));

        auto notification_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(event_supported_get_group_attributes_t::notification_type));
        notification_type_node.set_desired(notification_type);

        command_class_notification_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
