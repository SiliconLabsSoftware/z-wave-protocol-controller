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

/**
 * @defgroup zwave_command_class_crc16
 * @brief CRC16 Encap support
 *
 * Provides utility functions for CRC16 Encap support.
 * 
 * You can check if endpoint node actually supports CRC16 Encap.
 * Also you can flag a node and endpoint as expecting a CRC16 response to make sure that
 * you use the same encapsulation as the sender.
 * @{
 */
 
#ifndef ZWAVE_COMMAND_CLASS_CRC16_H
#define ZWAVE_COMMAND_CLASS_CRC16_H

#include "sl_status.h"
#include "zwave_node_id_definitions.h"
#include "attribute_store.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if the CRC16 command class is supported by a given node and endpoint.
 * 
 * @param endpoint_node Endpoint node
 * 
 * @return false if the node is still in interviewing state
 * @return false if the node does not support CRC16
 * @return false if the node explicitly does not want to send crc16 frame
 * @return true if the node supports CRC16
 */
bool zwave_command_class_crc16_is_supported(
  attribute_store_node_t endpoint_node);

/**
 * @brief Clear the CRC16 expecting response flag for a given node and endpoint.
 * 
 * @note Nothing happens if the flag is not set.
 * 
 * @param node_id Node ID
 * @param endpoint_id Endpoint ID
 */
void zwave_command_class_crc16_clear_expect_crc16_response(
  zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id);

/**
 * @brief Mark the given node_id and endpoint_id as expecting a CRC16 response.
 * 
 * @note You can call this multiple times on the same node_id and endpoint_id to have multiples CRC16 responses
 * 
 * @param node_id Node ID
 * @param endpoint_id Endpoint ID
 */
void zwave_command_class_crc16_set_expect_crc16_response(
  zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id);

/**
 * @brief Check if the given node_id and endpoint_id is expecting a CRC16 response.
 * 
 * @param node_id Node ID
 * @param endpoint_id Endpoint ID
 * 
 * @return true if the node_id and endpoint_id is expecting a CRC16 response, false otherwise.
 */
bool zwave_command_class_crc16_is_expecting_crc16_response(
  zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id);

sl_status_t zwave_command_class_crc16_init();

#ifdef __cplusplus
} // extern "C"
#endif

#endif  //ZWAVE_COMMAND_CLASS_CRC16_H
/** @} end zwave_command_class_crc16 */
