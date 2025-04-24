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
// Includes from this component
#include "zwave_command_class_crc16.h"
#include "zwave_command_classes_utils.h"

// Generic includes
#include <stdlib.h>
#include <assert.h>
#include <algorithm>
#include <utility> // make_pair

// Includes from other ZPC Components
#include "zwave_command_class_indices.h"
#include "zwave_command_handler.h"
#include "zpc_attribute_store_network_helper.h"
#include "attribute_store_defined_attribute_types.h"
#include "ZW_classcmd.h"
#include "zpc_attribute_resolver.h"

// Includes from other Unify Components
#include "dotdot_mqtt.h"
#include "dotdot_mqtt_generated_commands.h"
#include "attribute_store_helper.h"
#include "attribute_resolver.h"
#include "attribute_timeouts.h"
#include "sl_log.h"

// Cpp include
#include "attribute.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"


// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_CRC16_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_crc16";

using crc16_disabled_flag_t = uint8_t;
constexpr uint8_t CRC16_DISABLED_FLAG_OFF = 0x00;
constexpr uint8_t CRC16_DISABLED_FLAG_ON  = 0x01;

namespace
{
using connection_info_pair_t = std::pair<zwave_node_id_t, zwave_endpoint_id_t>;
std::vector<connection_info_pair_t> // NOSONAR : false positive
  expecting_crc16_response;  // NOSONAR : false positive
}

///////////////////////////////////////////////////////////////////////////////
// Helper
///////////////////////////////////////////////////////////////////////////////
bool zwave_command_class_crc16_is_supported(attribute_store_node_t node)
{
  attribute_store::attribute endpoint_node(node);

  auto network_status_node = endpoint_node.parent().child_by_type(
    DOTDOT_ATTRIBUTE_ID_STATE_NETWORK_STATUS);
  if (!network_status_node.reported_exists()) {
    return false;
  }

  if (network_status_node.reported<NodeStateNetworkStatus>()
      != ZCL_NODE_STATE_NETWORK_STATUS_ONLINE_FUNCTIONAL) {
    return false;
  }

  auto disabled_crc16_node
    = endpoint_node.child_by_type(ATTRIBUTE(DISABLE_CRC16));

  if (disabled_crc16_node.is_valid() && disabled_crc16_node.reported_exists()
      && disabled_crc16_node.reported<crc16_disabled_flag_t>()
           == CRC16_DISABLED_FLAG_ON) {
    return false;
  }

  // Check if the endpoint supports CRC16
  return endpoint_node.child_by_type(ATTRIBUTE(VERSION)).is_valid();
}

///////////////////////////////////////////////////////////////////////////////
// Current state
///////////////////////////////////////////////////////////////////////////////

std::vector<connection_info_pair_t>::iterator
  find_expecting_crc16_response(zwave_node_id_t node_id,
                                zwave_endpoint_id_t endpoint_id)
{
  return std::find_if(expecting_crc16_response.begin(),
                      expecting_crc16_response.end(),
                      [node_id, endpoint_id](const auto &pair) {
                        return pair.first == node_id
                               && pair.second == endpoint_id;
                      });
}

void zwave_command_class_crc16_clear_expect_crc16_response(
  zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id)
{
  sl_log_debug(LOG_TAG,
               "Clearing expecting CRC16 response for node %d, endpoint %d",
               node_id,
               endpoint_id);
  auto it = find_expecting_crc16_response(node_id, endpoint_id);
  if (it != expecting_crc16_response.end()) {
    sl_log_debug(LOG_TAG,
                 "Removing expecting CRC16 response for node %d, endpoint %d",
                 node_id,
                 endpoint_id);
    expecting_crc16_response.erase(it);
  }
}

void zwave_command_class_crc16_set_expect_crc16_response(
  zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id)
{
  sl_log_debug(LOG_TAG,
               "Setting expecting CRC16 response for node %d, endpoint %d",
               node_id,
               endpoint_id);
  expecting_crc16_response.emplace_back(std::make_pair(node_id, endpoint_id));
}

bool zwave_command_class_crc16_is_expecting_crc16_response(
  zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id)
{
  return find_expecting_crc16_response(node_id, endpoint_id)
         != expecting_crc16_response.end();
}

///////////////////////////////////////////////////////////////////////////////
// Incoming commands handler
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_crc16_support_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  sl_log_critical(LOG_TAG, "CRC16 Encapsulation command received");

  switch (frame_data[COMMAND_INDEX]) {
    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Attribute Store callback functions
///////////////////////////////////////////////////////////////////////////////
static void zwave_command_class_crc16_on_version_attribute_update(
  attribute_store_node_t updated_node, attribute_store_change_t change)
{
  if (change == ATTRIBUTE_DELETED) {
    return;
  }

  // Confirm that we have a version attribute update
  assert(ATTRIBUTE(VERSION) == attribute_store_get_node_type(updated_node));

  attribute_store::attribute version_node(updated_node);

  // Wait for the version
  if (!version_node.reported_exists()) {
    return;
  }

  // Now we know we have a CRC16 supporting endpoint.
  attribute_store::attribute endpoint_node
    = version_node.first_parent(ATTRIBUTE_ENDPOINT_ID);

  auto disable_crc16_node
    = endpoint_node.emplace_node(ATTRIBUTE(DISABLE_CRC16));

  // By default we enable CRC16 response if supported
  if (!disable_crc16_node.reported_exists()) {
    disable_crc16_node.set_reported<crc16_disabled_flag_t>(
      CRC16_DISABLED_FLAG_OFF);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_crc16_init()
{
  // Attribute store callbacks
  attribute_store_register_callback_by_type(
    zwave_command_class_crc16_on_version_attribute_update,
    ATTRIBUTE(VERSION));

  // The support side of things: Register our handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  // Need to register the support handler to be included in the NIF
  handler.support_handler         = &zwave_command_class_crc16_support_handler;
  handler.control_handler         = NULL;
  // Not supported, so this does not really matter
  handler.minimal_scheme             = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  handler.manual_security_validation = false;
  handler.command_class              = COMMAND_CLASS_CRC_16_ENCAP;
  handler.version                    = CRC_16_ENCAP_VERSION;
  handler.command_class_name         = "CRC16 Encapsulation";
  handler.comments                   = "";

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}

