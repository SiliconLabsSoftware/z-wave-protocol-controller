
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

#include "command_class_door_lock.hpp"

#include "zpc_mqtt.hpp"
#include "zpc_mqtt_utils.hpp"

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_door_lock_mqtt";

    command_class_door_lock_mqtt::command_class_door_lock_mqtt()
    {
        mqtt_callback_map.insert({"DoorLockOperationGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_door_lock_mqtt::mqtt_on_door_lock_operation_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"DoorLockOperationSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_door_lock_mqtt::mqtt_on_door_lock_operation_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"DoorLockConfigurationGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_door_lock_mqtt::mqtt_on_door_lock_configuration_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"DoorLockConfigurationSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_door_lock_mqtt::mqtt_on_door_lock_configuration_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"DoorLockCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_door_lock_mqtt::mqtt_on_door_lock_capabilities_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_door_lock_mqtt::mqtt_on_door_lock_operation_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_get_group_attributes_t::DOOR_LOCK_OPERATION_GET_GROUP));
        command_class_door_lock_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_door_lock_mqtt::mqtt_on_door_lock_operation_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t door_lock_mode = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("door_lock_mode", door_lock_mode);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_set_group_attributes_t::DOOR_LOCK_OPERATION_SET_GROUP));

        auto door_lock_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_set_group_attributes_t::door_lock_mode));
        door_lock_mode_node.set_desired(door_lock_mode);

        command_class_door_lock_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_door_lock_mqtt::mqtt_on_door_lock_configuration_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_get_group_attributes_t::DOOR_LOCK_CONFIGURATION_GET_GROUP));
        command_class_door_lock_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_door_lock_mqtt::mqtt_on_door_lock_configuration_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t operation_type               = 0;
        uint8_t inside_door_handles_enabled  = 0;
        uint8_t outside_door_handles_enabled = 0;
        uint8_t lock_timeout_minutes         = 0;
        uint8_t lock_timeout_seconds         = 0;
        uint16_t auto_relock_time            = 0;
        uint16_t hold_and_release_time       = 0;
        uint8_t ta                           = 0;
        uint8_t btb                          = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("operation_type", operation_type).parse("lock_timeout_minutes", lock_timeout_minutes).parse("lock_timeout_seconds", lock_timeout_seconds).parse("auto_relock_time", auto_relock_time).parse("hold_and_release_time", hold_and_release_time);
        parser.parse_nested("properties1").parse("inside_door_handles_enabled", inside_door_handles_enabled).parse("outside_door_handles_enabled", outside_door_handles_enabled);
        parser.parse_nested("properties2").parse("ta", ta).parse("btb", btb);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::DOOR_LOCK_CONFIGURATION_SET_GROUP));

        auto operation_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::operation_type));
        operation_type_node.set_desired(operation_type);

        auto inside_door_handles_enabled_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::inside_door_handles_enabled));
        inside_door_handles_enabled_node.set_desired(inside_door_handles_enabled);

        auto outside_door_handles_enabled_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::outside_door_handles_enabled));
        outside_door_handles_enabled_node.set_desired(outside_door_handles_enabled);

        auto lock_timeout_minutes_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::lock_timeout_minutes));
        lock_timeout_minutes_node.set_desired(lock_timeout_minutes);

        auto lock_timeout_seconds_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::lock_timeout_seconds));
        lock_timeout_seconds_node.set_desired(lock_timeout_seconds);

        auto auto_relock_time_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::auto_relock_time));
        auto_relock_time_node.set_desired(auto_relock_time);

        auto hold_and_release_time_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::hold_and_release_time));
        hold_and_release_time_node.set_desired(hold_and_release_time);

        auto ta_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::ta));
        ta_node.set_desired(ta);

        auto btb_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::btb));
        btb_node.set_desired(btb);

        command_class_door_lock_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_door_lock_mqtt::mqtt_on_door_lock_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_get_group_attributes_t::DOOR_LOCK_CAPABILITIES_GET_GROUP));
        command_class_door_lock_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
