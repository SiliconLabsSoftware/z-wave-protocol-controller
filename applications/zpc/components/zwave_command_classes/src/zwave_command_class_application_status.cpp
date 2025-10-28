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
#include "zwave_command_class_application_status.h"
#include "zwave_command_class_application_status_types.h"
#include "zwave_command_classes_utils.h"

// Generic includes
#include <stdlib.h>
#include <assert.h>

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
#include "attribute_store.h"
#include "attribute_resolver.h"
#include "attribute_timeouts.h"
#include "sl_log.h"

// Cpp include
#include "attribute.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_APPLICATION_STATUS_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_application_status";

namespace
{
zwave_frame_generator frame_generator(COMMAND_CLASS_APPLICATION_STATUS);
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////
zwave_cc_version_t
  get_current_application_status_version(attribute_store_node_t node)
{
  zwave_cc_version_t version = zwave_command_class_get_version_from_node(
    node,
    COMMAND_CLASS_APPLICATION_STATUS);

  if (version == 0) {
    sl_log_error(LOG_TAG, "Application_status Command Class Version not found");
  }

  return version;
}

///////////////////////////////////////////////////////////////////////////////
// Validation function
///////////////////////////////////////////////////////////////////////////////
bool zwave_command_class_application_status_validate_reject_value(uint8_t value)
{
  return (value == 0x00 || value == 0x01);
}

///////////////////////////////////////////////////////////////////////////////
// Frame parsing functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_application_status_handle_status_busy(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG,
               "Application Status Report Application Busy frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = sizeof(ZW_APPLICATION_BUSY_FRAME);

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    // Validate frame size with desired size
    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for Application Status Report "
                   "Application Busy frame");
      return SL_STATUS_FAIL;
    }

    // Create the busy status attributes if not exists
    auto busy_status_node = endpoint_node.emplace_node(ATTRIBUTE(BUSY_STATUS));

    //Read Status value from report frame and store attribute
    parser.read_byte(busy_status_node);

    // Create the wait time attributes if not exists, Read Wait time from report frame store attribute
    parser.read_byte(busy_status_node.emplace_node(ATTRIBUTE(WAIT_TIME)));

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing Application Status Report Application "
                 "Busy frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_application_status_handle_status_reject(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG,
               "Application Status Report Application Reject frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = sizeof(ZW_APPLICATION_REJECTED_REQUEST_FRAME);

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    // Validate frame size with desired size
    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for Application Status Report "
                   "Application Reject frame");
      return SL_STATUS_FAIL;
    }

    //Read Status and store attribute
    application_busy_status status = parser.read_byte();
    sl_log_debug(LOG_TAG, "Application Status Reject Status : %d", status);

    if (!zwave_command_class_application_status_validate_reject_value(status)) {
      return SL_STATUS_FAIL;
    }

    // Create the reject status attributes if not exists
    auto reject_status_node
      = endpoint_node.emplace_node(ATTRIBUTE(REJECT_STATUS));
    reject_status_node.set_reported<application_reject_request>(status);

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing Application Status Report Application "
                 "Reject frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Incoming commands handler
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_application_status_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Frame too short, it should have not come here.
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case APPLICATION_BUSY:
      return zwave_command_class_application_status_handle_status_busy(
        connection_info,
        frame_data,
        frame_length);
    case APPLICATION_REJECTED_REQUEST:
      return zwave_command_class_application_status_handle_status_reject(
        connection_info,
        frame_data,
        frame_length);
    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Attribute Store callback functions
///////////////////////////////////////////////////////////////////////////////
static void zwave_command_class_application_status_on_version_attribute_update(
  attribute_store_node_t updated_node, attribute_store_change_t change)
{
  if (change == ATTRIBUTE_DELETED) {
    return;
  }

  // Confirm that we have a version attribute update
  assert(ATTRIBUTE(VERSION) == attribute_store_get_node_type(updated_node));

  attribute_store::attribute version_node(updated_node);
  // Do not create the attributes until we are sure of the version
  zwave_cc_version_t supporting_node_version = 0;

  // Wait for the version
  if (!version_node.reported_exists()) {
    return;
  }
  supporting_node_version = version_node.reported<uint8_t>();

  // Wait that the version becomes non-zero.
  if (supporting_node_version == 0) {
    return;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_application_status_init()
{
  // Attribute store callbacks
  attribute_store_register_callback_by_type(
    zwave_command_class_application_status_on_version_attribute_update,
    ATTRIBUTE(VERSION));

  // The support side of things: Register our handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  handler.support_handler         = NULL;
  handler.control_handler
    = &zwave_command_class_application_status_control_handler;
  // Not supported, so this does not really matter
  handler.minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME;
  handler.manual_security_validation = false;
  handler.command_class              = COMMAND_CLASS_APPLICATION_STATUS;
  handler.version                    = APPLICATION_STATUS_VERSION;
  handler.command_class_name         = "Application Status";
  handler.comments                   = "";

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}
