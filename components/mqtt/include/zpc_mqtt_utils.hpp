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

#ifndef ZPC_MQTT_UTILS_HPP
#define ZPC_MQTT_UTILS_HPP

// ZPC
#include "attribute.hpp"  // attribute_store::attribute

namespace zpc_mqtt
{
    namespace utils
    {
        /**
         * @brief Get endpoint node from a topic
         *
         * The topic should be in the format zpc/{home_id}/{node_id}/ep{endpoint_id}/
         * It will look for the following attributes in the attribute store:
         * - ATTRIBUTE_HOME_ID
         * - ATTRIBUTE_NODE_ID
         * - ATTRIBUTE_ENDPOINT_ID
         * That matches the topic. If any of the attributes is not found, it will return an invalid attribute.
         *
         * @param topic The topic to extract the endpoint node from
         *
         * @return attribute_store::attribute The found endpoint node attribute or an invalid attribute if no node was found.
         *
         */
        attribute_store::attribute get_endpoint_node_from_topic(const std::string &topic);

        struct zwave_command_info {
                std::string command_class_name;
                std::string command_name;
        };
        /**
         * @brief Get the command info from a topic
         *
         * Extract the command class name and the command name from a topic.
         * The topic should contain : {command_class}/MQTT_COMMAND_KEYWORD/{command_name}
         *
         * @param topic The topic to extract the command info from
         *
         * @return zwave_command_info The command class name and the command name. If no match, will return an empty object.
         */
        zwave_command_info get_command_info_from_topic(const std::string &topic);

        /**
         * @brief Get the base topic based on an attribute
         *
         * It will extract the information of the parent node to return something like :
         * zpc/{home_id}/{node_id}/ep{endpoint_id}/
         *
         * @param attribute The attribute to get the base topic from
         */
        std::string get_base_topic_from_attribute(const attribute_store::attribute &attribute);

    }  // namespace utils
}  // namespace zpc_mqtt

#endif  // ZPC_MQTT_UTILS_HPP