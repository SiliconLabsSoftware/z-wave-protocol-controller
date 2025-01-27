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

/**
 * @file zigpc_zigpc_ucl_int.hpp
 * @defgroup zigpc_ucl_util ZigPC UCL utility functions
 * @ingroup zigpc_ucl
 * @brief Utility functions used in manipulating UCL messages
 *
 * @{
 */

#ifndef ZIGPC_UCL_INT_HPP
#define ZIGPC_UCL_INT_HPP

#include <nlohmann/json.hpp>

// Unify shared includes
#include <sl_status.h>
#include <uic_mqtt.h>

// ZigPC includes
#include <zigpc_common_zigbee.h>
#include <zigpc_net_mgmt.h>
#include <zigpc_net_mgmt_notify.h>

// Component includes
#include "zigpc_ucl.hpp"

/**
 * @brief UCL topic and payload manipulators in the Zigbee Protocol Controller.
 *
 */
namespace zigpc_ucl
{
constexpr char LOG_TAG[] = "zigpc_ucl";
constexpr char LOG_FMT_JSON_ERROR[] = "%s: Unable to parse JSON payload: %s";

/**
 * @brief UCL utility functions available to build and parse MQTT topics and
 * payloads.
 *
 */
namespace mqtt
{
/**
 * @brief Parse the Incoming JSON payload into the passed in property tree.
 *
 * @param payload     Incoming character array of payload.
 * @param jsn         Json object to populate payload to.
 * @return sl_status_t SL_STATUS_OK on success, SL_STATUS_NULL_POINTER on
 * invalid character array passed in,
 */
sl_status_t parse_payload(const char *payload, nlohmann::json &jsn);

/**
 * @brief Subscribe to a UCL topic based on the type and data passed in.
 *
 * @param topic_type    UCL topic type.
 * @param topic_data    Data to be populated in topic.
 * @param cb            Callback to invoke on messages to the topic.
 * @return sl_status_t  SL_STATUS_OK on success, SL_STATUS_NULL_POINTER on
 * invalid callback provided, MQTT API based error otherwise.
 */
sl_status_t subscribe(zigpc_ucl::mqtt::topic_type_t topic_type,
                      zigpc_ucl::mqtt::topic_data_t topic_data,
                      mqtt_message_callback_t cb);

/**
 * @brief Publish payload to a UCL topic based on the type and data passed in.
 *
 * @param topic_type    UCL topic type.
 * @param topic_data    Data to be populated in topic.
 * @param payload       Payload to publish.
 * @param payload_size  Size of payload to publish.
 * @param retain        Publish the payload as retained or not.
 * @return sl_status_t  SL_STATUS_OK on success, SL_STATUS_NULL_POINTER on
 * invalid payload provided, MQTT API based error otherwise.
 */
sl_status_t publish(zigpc_ucl::mqtt::topic_type_t topic_type,
                    zigpc_ucl::mqtt::topic_data_t topic_data,
                    const char *payload,
                    size_t payload_size,
                    bool retain);

/**
 * @brief Unretain a UCL topic based on the specific topic type and topic
 * variables passed in.
 *
 * @param topic_type    Specific UCL topic type.
 * @param topic_data    Variables used to populate specific UCL topic type.
 * @return sl_status_t  SL_STATUS_OK on success, or topic building error
 * otherwise.
 */
sl_status_t unretain(zigpc_ucl::mqtt::topic_type_t topic_type,
                     zigpc_ucl::mqtt::topic_data_t topic_data);

}  // namespace mqtt

/**
 * @brief UCL operations manipulating the following topics:
 * - ucl/by-unid/+/ProtocolController/NetworkManagement
 * - ucl/by-unid/+/ProtocolController/NetworkManagement/Write
 *
 */
namespace pc_nwmgmt
{
/**
 * @brief Event handler when the network has initialized. This handler will
 * initialized any MQTT subscriptions needed to receieve PC NetworkManagement
 * commands.
 *
 * @param pc_eui64      ProtocolController device identifier.
 * @return sl_status_t  SL_STATUS_OK on success, MQTT API error otherwise.
 */
sl_status_t on_net_init(zigbee_eui64_uint_t pc_eui64);

/**
 * @brief MQTT subscription handler for receiving network management
 * updates. This function will send a dynamically allocated string from the
 * character array passed in as a the MQTT message.
 *
 * @param topic           Character array containing MQTT topic triggered
 * @param message         Character array containing MQTT payload
 * @param message_length  Length of MQTT payload
 */
void on_write_mqtt(const char *topic,
                   const char *message,
                   const size_t message_length);

/**
 * @brief Event handler when the network state has been updated. This handler
 * will update the NetworkManagement MQTT topic based on the data received.
 *
 * @param state_update  Network status update information.
 * @return sl_status_t  SL_STATUS_OK on success, MQTT API error otherwise.
 */
sl_status_t
  on_net_state_update(zigpc_net_mgmt_on_network_state_update_t &state);

}  // namespace pc_nwmgmt

}  // namespace zigpc_ucl

#endif /* ZIGPC_UCL_INT_HPP */

/** @} end zigpc_ucl_util */
