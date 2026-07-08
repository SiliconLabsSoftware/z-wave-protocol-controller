/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef ZPC_MQTT_DEFINITIONS_H
#define ZPC_MQTT_DEFINITIONS_H

// C++
#include <functional>
#include <string>
#include <string_view>

// ZPC
#include "attribute.hpp"  // attribute_store::attribute

// Base structure for the MQTT topics :
//
// zpc/{home_id}/{node_id}/ep{endpoint_id}/
//
// Extracted by : get_endpoint_node_from_topic() in zpc_mqtt_utils.cpp

namespace zpc_mqtt
{
    /////////////////////// MQTT Keywords ///////////////////////
    // Desired keyword
    const std::string MQTT_DESIRED_KEYWORD = "Desired";
    // Reported keyword
    const std::string MQTT_REPORTED_KEYWORD = "Reported";
    // Attribute delimiter keyword
    const std::string MQTT_ATTRIBUTE_KEYWORD = "Attribute";
    // Command delimiter keyword
    const std::string MQTT_COMMAND_KEYWORD = "Command";
    // Payload value keyword :  { "MQTT_PAYLOAD_VALUE_KEYWORD" : 12 }
    const std::string MQTT_PAYLOAD_VALUE_KEYWORD = "value";
    /////////////////////// MQTT Keywords ///////////////////////

    /////////////////////// MQTT Topics /////////////////////////
    // Root name
    const std::string MQTT_ROOT_NAME = "zpc";
    // Base topic : zpc/{home_id}/{node_id}/
    inline constexpr std::string_view MQTT_BASE_TOPIC = "zpc/{home_id:8X}/{node_id:04X}/";
    // Endpoint (if applicable)
    inline constexpr std::string_view MQTT_ENDPOINT = "ep{endpoint_id}/";
    // Used to subscribe to command topic. Must match MQTT_BASE_TOPIC + MQTT_COMMAND_TOPIC
    inline constexpr std::string_view MQTT_COMMAND_BASE_TOPIC = "zpc/+/+/+/";
    // Used to subscribe to command topic (ZWaveCC/Command/MyCommand/)
    inline constexpr std::string_view MQTT_COMMAND_TOPIC = "{command_class}/Command/";
    // Attribute topic (ZWaveCC/Attribute/MyAttribute/)
    inline constexpr std::string_view MQTT_ATTRIBUTE_TOPIC = "{command_class}/Attribute{attribute_path}{attribute_name}/";
    // Base topic with endpoint: MQTT_BASE_TOPIC + MQTT_ENDPOINT
    inline constexpr std::string_view MQTT_BASE_TOPIC_WITH_ENDPOINT = "zpc/{home_id:8X}/{node_id:04X}/ep{endpoint_id}/";
    // Command subscription topic (MQTT_COMMAND_BASE_TOPIC + MQTT_COMMAND_TOPIC + "#")
    inline constexpr std::string_view MQTT_COMMAND_SUBSCRIBE_TOPIC = "zpc/+/+/+/{command_class}/Command/#";
    /////////////////////// MQTT Topics /////////////////////////

    /**
     * @brief Callback for MQTT commands
     *
     * For example zpc/CAFECAFE/0004/ep3/ZWaveCC/Command/MyCommand {"value": 12} will
     * trigger the callback for ZWaveCC command class handler with :
     *  - attribute : ATTRIBUTE_ENDPOINT_ID (3)
     *  - command_name : MyCommand
     *  - payload : {"value": 12}
     *
     * @param attribute Endpoint attribute associated with the command
     * @param command_name The name of the command
     * @param payload The payload of the command
     */
    using mqtt_command_callback = std::function<void(attribute_store::attribute, const std::string &, const std::string &)>;

    /**
     * @see register_value_transformation
     */
    using mqtt_value_encoder = std::function<std::string(const attribute_store::attribute &, attribute_store_node_value_state_t)>;

}  // namespace zpc_mqtt

#endif  // ZPC_MQTT_DEFINITIONS_H
