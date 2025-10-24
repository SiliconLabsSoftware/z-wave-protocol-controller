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
#include "zwave_command_class_switch_all.h"
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
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_SWITCH_ALL_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_switch_all";

enum class switch_all_mode_t : uint8_t {
  DEVICE_EXCLUDED = 0x00,
  DEVICE_ONLY_ON  = 0x01,
  DEVICE_ONLY_OFF = 0x02,
  DEVICE_ON_OFF   = 0xFF
};

constexpr uint8_t SWITCH_ALL_ON_VALUE  = 1;
constexpr uint8_t SWITCH_ALL_OFF_VALUE = 0;

namespace
{
zwave_frame_generator frame_generator(COMMAND_CLASS_SWITCH_ALL);
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Resolution functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_switch_all_set_on_off(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute value_node(node);
    // Creating the frame
    if (value_node.type() != ATTRIBUTE(ON_OFF)) {
      return SL_STATUS_INVALID_TYPE;
    }
    auto mode = static_cast<switch_all_mode_t>(
      value_node.parent().child_by_type(ATTRIBUTE(MODE)).reported<uint8_t>());
    bool send_command = false;
    uint8_t zwave_command_id;
    auto on_off_desired = value_node.desired<uint8_t>();

    switch (on_off_desired) {
      case SWITCH_ALL_ON_VALUE:
        zwave_command_id = SWITCH_ALL_ON;
        send_command     = (mode == switch_all_mode_t::DEVICE_ONLY_ON
                        || mode == switch_all_mode_t::DEVICE_ON_OFF);
        break;
      case SWITCH_ALL_OFF_VALUE:
        zwave_command_id = SWITCH_ALL_OFF;
        send_command     = (mode == switch_all_mode_t::DEVICE_ONLY_OFF
                        || mode == switch_all_mode_t::DEVICE_ON_OFF);
        break;
      default:
        return SL_STATUS_INVALID_RANGE;
    }
    if (!send_command) {
      sl_log_debug(LOG_TAG,
                   "Not sending command %d since mode is %d",
                   zwave_command_id,
                   mode);
      return SL_STATUS_OK;
    }
    return frame_generator.generate_no_args_frame(zwave_command_id,
                                                  frame,
                                                  frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating switch_all Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_switch_all_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  if (attribute_store_get_node_type(node) == ATTRIBUTE(MODE)) {
    return frame_generator.generate_no_args_frame(SWITCH_ALL_GET,
                                                  frame,
                                                  frame_length);
  }
  return SL_STATUS_INVALID_TYPE;
}

static sl_status_t zwave_command_class_switch_all_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute value_node(node);
    // Creating the frame
    if (value_node.type() != ATTRIBUTE(MODE)) {
      return SL_STATUS_INVALID_TYPE;
    }
    frame_generator.initialize_frame(SWITCH_ALL_SET, frame, 3);
    frame_generator.add_value(value_node, DESIRED_ATTRIBUTE);
    frame_generator.validate_frame(frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating switch_all Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Frame parsing functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_switch_all_handle_report(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG, "Switch All Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = 3;

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG, "Invalid frame size for switch_all Report frame");
      return SL_STATUS_FAIL;
    }

    attribute_store::attribute mode
      = endpoint_node.child_by_type(ATTRIBUTE(MODE));
    parser.read_byte(mode);

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing switch_all Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Incoming commands handler
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_switch_all_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Frame too short, it should have not come here.
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case SWITCH_ALL_REPORT:
      return zwave_command_class_switch_all_handle_report(connection_info,
                                                          frame_data,
                                                          frame_length);
    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Attribute Store callback functions
///////////////////////////////////////////////////////////////////////////////
static void zwave_command_class_switch_all_on_version_attribute_update(
  attribute_store_node_t updated_node, attribute_store_change_t change)
{
  if (change == ATTRIBUTE_DELETED) {
    return;
  }

  // Confirm that we have a version attribute update
  assert(ATTRIBUTE(VERSION) == attribute_store_get_node_type(updated_node));

  attribute_store::attribute version_node(updated_node);
  // Do not create the attributes until we are sure of the version
  //zwave_cc_version_t supporting_node_version = 0;

  // Wait for the version
  if (!version_node.reported_exists()) {
    return;
  }
  //supporting_node_version = version_node.reported<uint8_t>();

  // Now we know we have a switch_all supporting endpoint.
  attribute_store::attribute endpoint_node
    = version_node.first_parent(ATTRIBUTE_ENDPOINT_ID);

  // Create the switch_all attributes
  endpoint_node.emplace_node(ATTRIBUTE(MODE));

  endpoint_node.emplace_node(ATTRIBUTE(ON_OFF));
}

sl_status_t
  zwave_command_class_check_ep_support(const dotdot_unid_t unid,
                                       const dotdot_endpoint_id_t endpoint)
{
  attribute_store::attribute node
    = attribute_store_network_helper_get_node_id_node(unid);
  attribute_store::attribute ep
    = node.child_by_type_and_value<>(ATTRIBUTE_ENDPOINT_ID, endpoint);

  if (ep.child_by_type(ATTRIBUTE(MODE)).is_valid())
    return SL_STATUS_OK;
  return SL_STATUS_FAIL;
}

sl_status_t zwave_command_class_unify_switch_all_write_attributes_callback(
  const dotdot_unid_t unid,
  const dotdot_endpoint_id_t endpoint,
  uic_mqtt_dotdot_callback_call_type_t call_type,
  uic_mqtt_dotdot_unify_switch_all_state_t value,
  uic_mqtt_dotdot_unify_switch_all_updated_state_t changed)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }

  attribute_store::attribute home
    = attribute_store_network_helper_get_home_id_node(unid);

  if (changed.mode) {
    attribute_store::attribute node
      = attribute_store_network_helper_get_node_id_node(unid);
    attribute_store::attribute ep
      = node.child_by_type_and_value<>(ATTRIBUTE_ENDPOINT_ID, endpoint);
    attribute_store::attribute mode
      = ep.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SWITCH_ALL_MODE);

    sl_log_debug(LOG_TAG, "write callback set desired mode");
    mode.set_desired<>(value.mode);
  }
  if (changed.on_off) {
    for (auto node: home.children()) {
      auto ep = node.child_by_type(ATTRIBUTE_ENDPOINT_ID);
      if (!ep.child_by_type(ATTRIBUTE(MODE)).is_valid()) {
        sl_log_debug(LOG_TAG,
                     "Switch All CC not supported by ep %u of node %x",
                     ep.reported<uint8_t>(),
                     node.reported<uint16_t>());
        continue;
      }
      auto on_off
        = ep.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SWITCH_ALL_ON_OFF);

      sl_log_debug(LOG_TAG, "write callback set desired on_off");
      on_off.set_desired<>(value.on_off);
    }
  }

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_switch_all_init()
{
  // Attribute store callbacks
  attribute_store_register_callback_by_type(
    zwave_command_class_switch_all_on_version_attribute_update,
    ATTRIBUTE(VERSION));

  // Attribute resolver rules
  attribute_resolver_register_rule(ATTRIBUTE(MODE),
                                   zwave_command_class_switch_all_set,
                                   zwave_command_class_switch_all_get);
  attribute_resolver_register_rule(ATTRIBUTE(ON_OFF),
                                   zwave_command_class_switch_all_set_on_off,
                                   nullptr);
  // Attribute write callback
  uic_mqtt_dotdot_set_unify_switch_all_write_attributes_callback(
    zwave_command_class_unify_switch_all_write_attributes_callback);

  // The support side of things: Register our handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  handler.support_handler         = NULL;
  handler.control_handler = &zwave_command_class_switch_all_control_handler;
  // Not supported, so this does not really matter
  handler.minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME;
  handler.manual_security_validation = false;
  handler.command_class              = COMMAND_CLASS_SWITCH_ALL;
  handler.version                    = SWITCH_ALL_VERSION;
  handler.command_class_name         = "switch_all";
  handler.comments                   = "";

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}
