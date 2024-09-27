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
#include "zwave_command_class_binary_switch.h"
#include "zwave_command_class_binary_switch_types.h"
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
#include "attribute_resolver.h"
#include "attribute_timeouts.h"
#include "sl_log.h"

// Cpp include
#include "attribute.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_BINARY_SWITCH_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_binary_switch";

// Constants
constexpr uint8_t VALUE_ON  = 0xFF;
constexpr uint8_t VALUE_OFF = 0x00;

// Cpp helpers
namespace
{
zwave_frame_generator frame_generator(COMMAND_CLASS_SWITCH_BINARY); //NOSONAR - false positive since it is warped in a namespace
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////
zwave_cc_version_t get_current_binary_switch_version(attribute_store_node_t node)
{
  zwave_cc_version_t version
    = zwave_command_class_get_version_from_node(node, COMMAND_CLASS_SWITCH_BINARY);

  if (version == 0) {
    sl_log_error(LOG_TAG, "Binary Switch Command Class Version not found");
  }

  return version;
}

///////////////////////////////////////////////////////////////////////////////
// Validation function
///////////////////////////////////////////////////////////////////////////////
bool zwave_command_class_binary_switch_validate_value(uint8_t &value)
{
  bool value_is_valid = false;
  if (value == VALUE_ON || value == VALUE_OFF) {
    value_is_valid = true;
  } else if (value >= 1 && value <= 99) {
    sl_log_info(LOG_TAG,
                "Forcing the value of Binary Switch to %d since we have %d",
                VALUE_ON,
                value);
    value          = VALUE_ON;
    value_is_valid = true;
  }

  return value_is_valid;
}

///////////////////////////////////////////////////////////////////////////////
// Resolution functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_binary_switch_get(
  [[maybe_unused]] attribute_store_node_t node,
  uint8_t *frame,
  uint16_t *frame_length)
{
  return frame_generator.generate_no_args_frame(SWITCH_BINARY_GET,
                                                frame,
                                                frame_length);
}

static sl_status_t zwave_command_class_binary_switch_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute value_node(node);
    assert(value_node.is_valid() && value_node.type() == ATTRIBUTE(VALUE));

    auto current_version = get_current_binary_switch_version(node);

    // Compute expected size for set frame
    const uint8_t expected_frame_size
      = current_version >= 2 ? sizeof(ZW_SWITCH_BINARY_SET_V2_FRAME)
                             : sizeof(ZW_SWITCH_BINARY_SET_FRAME);

    // Creating the frame
    frame_generator.initialize_frame(SWITCH_BINARY_SET,
                                     frame,
                                     expected_frame_size);

    auto value = value_node.desired_or_reported<binary_switch_value_t>();
    frame_generator.add_raw_byte(static_cast<uint8_t>(value));

    if (current_version >= 2) {
      auto duration_node
        = value_node.parent().child_by_type(ATTRIBUTE(DURATION));

      auto duration = duration_node.desired_or_reported<binary_switch_duration_t>();
      frame_generator.add_raw_byte(static_cast<uint8_t>(duration));
    }

    frame_generator.validate_frame(frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating Binary Switch Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Frame parsing functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_binary_switch_handle_report(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));
  auto current_version = get_current_binary_switch_version(endpoint_node);

  sl_log_debug(LOG_TAG, "Binary Switch Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = (current_version >= 2)
                                  ? sizeof(ZW_SWITCH_BINARY_REPORT_V2_FRAME)
                                  : sizeof(ZW_SWITCH_BINARY_REPORT_FRAME);

  attribute_store::attribute state_node
    = endpoint_node.child_by_type(ATTRIBUTE(STATE));

  if (!state_node.is_valid()) {
    sl_log_error(
      LOG_TAG,
      "Can't find state node when parsing Binary Switch Report frame");
    return SL_STATUS_FAIL;
  }

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for Binary Switch Report frame");
      return SL_STATUS_FAIL;
    }

    auto value_node = state_node.child_by_type(ATTRIBUTE(VALUE));

    // Read value
    uint8_t current_value = parser.read_byte();
    sl_log_debug(LOG_TAG, "Binary Switch Report Value : %d", current_value);

    // Nodes should only report 0x00, 0xFE or 0xFF, but in case somebody
    // did something funny, we accept values 1..100
    if (!zwave_command_class_binary_switch_validate_value(current_value)) {
      sl_log_error(LOG_TAG,
                   "Invalid value for Binary Switch Report frame : %d",
                   current_value);
      return SL_STATUS_FAIL;
    }

    if (current_version >= 2) {
      auto duration_node = state_node.child_by_type(ATTRIBUTE(DURATION));
      // Get target value
      uint8_t target_value = parser.read_byte();

      sl_log_debug(LOG_TAG,
                   "Binary Switch Report Target Value : %d",
                   target_value);

      // Check if the target value is valid
      if (!zwave_command_class_binary_switch_validate_value(target_value)) {
        sl_log_error(LOG_TAG,
                     "Invalid target value for Binary Switch Report frame : %d",
                     current_value);
        return SL_STATUS_FAIL;
      }

      if (target_value == current_value) {
        value_node.set_reported<binary_switch_value_t>(current_value);
      } else {
        sl_log_info(LOG_TAG,
                    "Binary Switch waiting to reach target value : %d",
                    target_value);
      }
      // Read duration
      binary_switch_duration_t current_duration = parser.read_byte();

      sl_log_debug(LOG_TAG,
                   "Binary Switch Report Duration : %d",
                   current_duration);

      // If duration is 0xFF (default factory), we consider this to be instantaneous
      if (current_duration == 0xFF) {
        duration_node.set_reported<binary_switch_duration_t>(0);
        sl_log_info(
          LOG_TAG,
          "Forcing the duration of Binary Switch to %d since we received %d",
          0,
          0xFF);
      } else {
        duration_node.set_reported<binary_switch_duration_t>(current_duration);
      }
    } else {
      // In v1 we only have the value so we set it without check
      value_node.set_reported<binary_switch_value_t>(current_value);
    }

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing Binary Switch Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Attribute Store callback functions
///////////////////////////////////////////////////////////////////////////////
static void zwave_command_class_binary_switch_on_version_attribute_update(
  attribute_store_node_t updated_node, attribute_store_change_t change)
{
  if (change == ATTRIBUTE_DELETED) {
    return;
  }

  if (is_zwave_command_class_filtered_for_root_device(
        COMMAND_CLASS_SWITCH_BINARY,
        updated_node)) {
    return;
  }

  // Confirm that we have a version attribute update
  assert(ATTRIBUTE(VERSION) == attribute_store_get_node_type(updated_node));

  // Do not create the attributes until we are sure of the version
  uint8_t supporting_node_version = 0;
  attribute_store_get_reported(updated_node,
                               &supporting_node_version,
                               sizeof(supporting_node_version));

  // Wait that the version becomes non-zero.
  if (supporting_node_version == 0) {
    return;
  }

  // Now we know we have a Binary Switch supporting endpoint.
  attribute_store::attribute endpoint_node(
    attribute_store_get_first_parent_with_type(updated_node,
                                               ATTRIBUTE_ENDPOINT_ID));

  // We keep legacy implementation, so we need to create the state attribute
  auto state_node = endpoint_node.emplace_node(ATTRIBUTE(STATE));
  // Emplace value state
  state_node.emplace_node(ATTRIBUTE(VALUE));

  if (supporting_node_version >= 2) {
    state_node.emplace_node(ATTRIBUTE(DURATION));
  }
}

///////////////////////////////////////////////////////////////////////////////
// Incoming commands handler
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_binary_switch_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Frame too short, it should have not come here.
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case SWITCH_BINARY_REPORT:
      return zwave_command_class_binary_switch_handle_report(connection_info,
                                                             frame_data,
                                                             frame_length);
    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_binary_switch_init()
{
  // Attribute store callbacks
  attribute_store_register_callback_by_type(
    zwave_command_class_binary_switch_on_version_attribute_update,
    ATTRIBUTE(VERSION));

  // Attribute resolver rules
  attribute_resolver_register_rule(ATTRIBUTE(VALUE),
                                   zwave_command_class_binary_switch_set,
                                   zwave_command_class_binary_switch_get);

  // The support side of things: Register our handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  handler.support_handler         = NULL;
  handler.control_handler = &zwave_command_class_binary_switch_control_handler;
  // Not supported, so this does not really matter
  handler.minimal_scheme             = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  handler.manual_security_validation = false;
  handler.command_class              = COMMAND_CLASS_SWITCH_BINARY;
  handler.version                    = SWITCH_BINARY_VERSION_V2;
  handler.command_class_name         = "Binary Switch";
  handler.comments                   = "";

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}
