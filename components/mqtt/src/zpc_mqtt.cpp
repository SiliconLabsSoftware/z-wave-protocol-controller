/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "zpc_mqtt.hpp"

// Other modules
#include "zpc_mqtt_definitions.hpp"             // MQTT_PAYLOAD_VALUE_KEYWORD
#include "zpc_mqtt_attribute_registration.hpp"  // zpc_mqtt_attribute_registration
#include "zpc_mqtt_command_registration.hpp"    // zpc_mqtt_command_registration

// Utils
#include "log.h"

// ZPC
#include "attribute.hpp"                        // attribute_store::attribute
#include "attribute_callbacks.hpp"              // register_callback_by_type, attribute_store_node_value_state_t, ...
#include "attribute_store_type_registration.h"  // attribute_store_get_storage_type

// MQTT
#include "mqtt_handler.hpp"

// Format
#include <fmt/format.h>

// Cpp
#include <set>
#include <map>
#include <functional>
namespace zpc_mqtt
{
    constexpr char LOG_TAG[] = "zpc_mqtt";

    namespace
    {
        zpc_mqtt_attribute_registration attribute_registration;
        zpc_mqtt_command_registration command_registration;
    }  // namespace

    void register_value_transformation(attribute_store_type_t attribute_type, const mqtt_value_encoder &encoder)
    {
        attribute_registration.register_encoder(attribute_type, encoder);
    }

    void registration_callback(attribute_store_node_t node, attribute_store_change_t change, attribute_store_node_value_state_t value_state)
    {
        // We ignore every attribute created since we don't want to publish anything yet
        //
        // We add a guard to prevent publishing when the attribute is deleted and the
        // value state is desired. The deletion event should only be triggered for
        // reported attributes but we never know.
        if (change == ATTRIBUTE_CREATED || (change == ATTRIBUTE_DELETED && value_state == DESIRED_ATTRIBUTE)) {
            return;
        }

        // Wrapper around the node
        attribute_store::attribute updated_node(node);
        std::string payload_str;

        // If we have an updated value, create a payload with the value
        // Otherwise we send an empty payload to mark the deletion
        if (change == ATTRIBUTE_UPDATED) {
            // If wanted value state is not present, return
            if (!updated_node.exists(value_state)) {
                return;
            }

            // Get the value
            payload_str = attribute_registration.get_attribute_payload(updated_node, value_state);
        }

        std::set<attribute_store_node_value_state_t> value_states = {value_state};
        // When we delete an attribute, we need to publish the deletion for the
        // desired attribute as well since it is not done automatically.
        if (change == ATTRIBUTE_DELETED) {
            value_states.insert(DESIRED_ATTRIBUTE);
        }
        for (auto current_value_state: value_states) {
            auto attribute_topic = attribute_registration.get_complete_attribute_topic(updated_node, current_value_state);

            sl_log_debug(LOG_TAG, "MQTT publish : %s %s", attribute_topic.c_str(), payload_str.c_str());

            if (attribute_topic.empty()) {
                sl_log_error(LOG_TAG, "Empty topic. Cannot publish to MQTT.");
                return;
            }

            // Publish mqtt topic with the value

            zwave_component::mqtt_handler::get_instance().publish(attribute_topic, payload_str, true);
        }
    }

    sl_status_t publish_report(const char *topic, const char *message, size_t message_length, bool retain)
    {
        if (topic == nullptr || message == nullptr || message_length == 0) {
            sl_log_error(LOG_TAG, "Invalid parameters for publish_report.");
            return SL_STATUS_FAIL;
        }

        // Convert C strings to std::string and publish
        zwave_component::mqtt_handler::get_instance().publish(std::string(topic), std::string(message, message_length), retain);

        return SL_STATUS_OK;
    }

    sl_status_t register_attribute(attribute_store_type_t attribute_type, const std::string &command_class_name, const std::string &attribute_name)
    {
        if (attribute_name.empty()) {
            sl_log_critical(LOG_TAG, "Attribute name is empty. Cannot register attribute to MQTT.");
            return SL_STATUS_FAIL;
        }

        if (command_class_name.empty()) {
            sl_log_critical(LOG_TAG, "Command class name is empty. Cannot register attribute to MQTT.");
            return SL_STATUS_FAIL;
        }

        auto storage_type = attribute_store_get_storage_type(attribute_type);

        if (storage_type == EMPTY_STORAGE_TYPE || storage_type == INVALID_STORAGE_TYPE || storage_type == UNKNOWN_STORAGE_TYPE || storage_type == FIXED_SIZE_STRUCT_STORAGE_TYPE) {
            sl_log_critical(LOG_TAG, "Not supported attribute type. Cannot register attribute to MQTT.");
            return SL_STATUS_FAIL;
        }

        // Register the attribute into the map
        attribute_registration.register_attribute_type(attribute_type, command_class_name, attribute_name);

        attribute_store::register_callback_by_type_and_state([attribute_name, command_class_name](attribute_store_node_t node, attribute_store_change_t change) { zpc_mqtt::registration_callback(node, change, DESIRED_ATTRIBUTE); }, attribute_type, DESIRED_ATTRIBUTE);

        attribute_store::register_callback_by_type_and_state([attribute_name, command_class_name](attribute_store_node_t node, attribute_store_change_t change) { zpc_mqtt::registration_callback(node, change, REPORTED_ATTRIBUTE); }, attribute_type, REPORTED_ATTRIBUTE);

        return SL_STATUS_OK;
    }

    sl_status_t register_command(const std::string &command_class_name, const mqtt_command_callback &callback)
    {
        if (command_class_name.empty()) {
            sl_log_critical(LOG_TAG, "Command class name is empty. Cannot register command to MQTT.");
            return SL_STATUS_FAIL;
        }

        if (callback == nullptr) {
            sl_log_critical(LOG_TAG, "Command callback is empty. Cannot register command to MQTT.");
            return SL_STATUS_FAIL;
        }

        // Register command to our internal map
        command_registration.register_command(command_class_name, callback);

        // Subscribe to the command topic with a direct callback
        zwave_component::mqtt_handler::get_instance().subscribe(zpc_mqtt::zpc_mqtt_command_registration::get_command_topic(command_class_name), [](const std::string &topic, const std::string &message) {
            sl_log_debug(LOG_TAG, "MQTT subscribe : %s %s", topic.c_str(), message.c_str());
            // Execute the real command callback
            command_registration.execute_command_callback(topic, message);
        });

        return SL_STATUS_OK;
    }

}  // namespace zpc_mqtt
