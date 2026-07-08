
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

// Base class
#include "zpc_mqtt_command_registration.hpp"

// Other components
#include "zpc_mqtt_utils.hpp"  // get_endpoint_node_from_topic

// fmt
#include "fmt/format.h"

// Unity
#include "log.h"

namespace zpc_mqtt
{

    constexpr char LOG_TAG[] = "zpc_mqtt_command_registration";

    void zpc_mqtt_command_registration::register_command(const std::string &command_class_name, const mqtt_command_callback &callback)
    {
        command_registration_map[command_class_name].push_back(callback);
    }

    std::string zpc_mqtt_command_registration::get_command_topic(const std::string &command_class_name)
    {
        return fmt::format(MQTT_COMMAND_SUBSCRIBE_TOPIC, fmt::arg("command_class", command_class_name));
    }

    void zpc_mqtt_command_registration::execute_command_callback(const std::string &topic, const std::string &payload) const
    {
        // Extract endpoint node from topic
        auto endpoint_node = utils::get_endpoint_node_from_topic(topic);
        // If anything went wrong, no need to display anything, all the info should be already displayed
        if (!endpoint_node.is_valid()) {
            return;
        }

        // Then get the command info (name and class)
        auto command_info = utils::get_command_info_from_topic(topic);
        // If anything went wrong, no need to display anything, all the info should be already displayed
        if (command_info.command_class_name.empty() || command_info.command_name.empty()) {
            return;
        }

        // Check if we have a callback registered for this command class
        if (!command_registration_map.contains(command_info.command_class_name)) {
            sl_log_warning(LOG_TAG,
                           "No command callback registered for command class %s. "
                           "Ignoring MQTT topic '%s'.",
                           command_info.command_class_name.c_str(),
                           topic.c_str());
            return;
        }

        // Then we notify the listeners to this command class
        for (const auto &callback: command_registration_map.at(command_info.command_class_name)) {
            callback(endpoint_node, command_info.command_name, payload);
        }
    }

}  // namespace zpc_mqtt