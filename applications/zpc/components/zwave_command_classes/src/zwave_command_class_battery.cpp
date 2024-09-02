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
#include "zwave_command_class_battery.h"
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
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_BATTERY_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_battery";

// Cpp helpers
namespace
{
zwave_frame_generator frame_generator(COMMAND_CLASS_BATTERY); //NOSONAR - false positive since it is warped in a namespace
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////
zwave_cc_version_t get_current_battery_version(attribute_store_node_t node)
{
  zwave_cc_version_t version
    = zwave_command_class_get_version_from_node(node, COMMAND_CLASS_BATTERY);

  if (version == 0) {
    sl_log_error(LOG_TAG, "Battery Command Class Version not found");
  }

  return version;
}

///////////////////////////////////////////////////////////////////////////////
// Resolution functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t
  zwave_command_class_battery_get([[maybe_unused]] attribute_store_node_t node,
                                  uint8_t *frame,
                                  uint16_t *frame_length)
{
  return frame_generator.generate_no_args_frame(BATTERY_GET,
                                                frame,
                                                frame_length);
}

static sl_status_t zwave_command_class_battery_health_get(
  [[maybe_unused]] attribute_store_node_t node,
  uint8_t *frame,
  uint16_t *frame_length)
{
  return frame_generator.generate_no_args_frame(BATTERY_HEALTH_GET_V2,
                                                frame,
                                                frame_length);
}

///////////////////////////////////////////////////////////////////////////////
// Frame parsing functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_battery_handle_battery_report(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));
  // Get current version supported
  auto current_version = get_current_battery_version(endpoint_node);

  sl_log_debug(LOG_TAG, "Battery Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = current_version >= BATTERY_VERSION_V2
                                  ? sizeof(ZW_BATTERY_REPORT_V2_FRAME)
                                  : sizeof(ZW_BATTERY_REPORT_FRAME);

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG, "Invalid frame size for Battery Report frame");
      return SL_STATUS_FAIL;
    }

    // Parse the frame
    parser.read_byte(endpoint_node.child_by_type(ATTRIBUTE(BATTERY_LEVEL)));

    if (current_version >= 2) {
      // Parse properties 1
      parser.read_byte_with_bitmask(
        {{.bitmask = BATTERY_REPORT_PROPERTIES1_CHARGING_STATUS_MASK_V2,
          .destination_node
          = endpoint_node.child_by_type(ATTRIBUTE(CHARGING_STATUS))},
         {.bitmask = BATTERY_REPORT_PROPERTIES1_RECHARGEABLE_BIT_MASK_V2,
          .destination_node
          = endpoint_node.child_by_type(ATTRIBUTE(RECHARGEABLE))},
         {.bitmask = BATTERY_REPORT_PROPERTIES1_BACKUP_BATTERY_BIT_MASK_V2,
          .destination_node
          = endpoint_node.child_by_type(ATTRIBUTE(BACKUP_BATTERY))},
         {.bitmask = BATTERY_REPORT_PROPERTIES1_OVERHEATING_BIT_MASK_V2,
          .destination_node
          = endpoint_node.child_by_type(ATTRIBUTE(OVERHEATING))},
         {.bitmask = BATTERY_REPORT_PROPERTIES1_LOW_FLUID_BIT_MASK_V2,
          .destination_node
          = endpoint_node.child_by_type(ATTRIBUTE(LOW_FLUID))},
         {.bitmask = BATTERY_REPORT_PROPERTIES1_REPLACE_RECHARGE_MASK_V2,
          .destination_node
          = endpoint_node.child_by_type(ATTRIBUTE(REPLACE_RECHARGE))}});

      // Parse properties 2
      std::vector<zwave_frame_parser::bitmask_data> properties2_bitmask_data
        = {{.bitmask = BATTERY_REPORT_PROPERTIES2_DISCONNECTED_BIT_MASK_V2,
            .destination_node
            = endpoint_node.child_by_type(ATTRIBUTE(DISCONNECTED))}};

      if (current_version >= 3) {
        properties2_bitmask_data.push_back(
          {.bitmask
           = BATTERY_REPORT_PROPERTIES2_LOW_TEMPERATURE_STATUS_BIT_MASK_V3,
           .destination_node
           = endpoint_node.child_by_type(ATTRIBUTE(LOW_TEMPERATURE))});
      }

      parser.read_byte_with_bitmask(properties2_bitmask_data);
    }

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing Battery Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_battery_handle_battery_health_report(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)

{
  // Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG, "Battery Report Health frame received");

  // Compute expected size for report frame
  const uint8_t report_min_size = 5;

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(report_min_size, report_min_size + 3)) {
      sl_log_error(LOG_TAG, "Invalid frame size for Battery Health Report frame");
      return SL_STATUS_FAIL;
    }

    parser.read_byte(
      endpoint_node.child_by_type(ATTRIBUTE(HEALTH_MAXIMUM_CAPACITY)));

    // We don't put the size in the attribute store, we only read it for next parsing
    auto read_data = parser.read_byte_with_bitmask(
      {{.bitmask = BATTERY_HEALTH_REPORT_PROPERTIES1_PRECISION_MASK_V2,
        .destination_node
        = endpoint_node.child_by_type(ATTRIBUTE(HEALTH_PRECISION))},
       {.bitmask = BATTERY_HEALTH_REPORT_PROPERTIES1_SCALE_MASK_V2,
        .destination_node
        = endpoint_node.child_by_type(ATTRIBUTE(HEALTH_SCALE))},
       {.bitmask = BATTERY_HEALTH_REPORT_PROPERTIES1_SIZE_MASK_V2}});

    // Parse temperature size
    auto temperature_size = read_data[BATTERY_HEALTH_REPORT_PROPERTIES1_SIZE_MASK_V2];
    sl_log_debug(LOG_TAG, "Temperature size: %d", temperature_size);

    parser.read_sequential<int32_t>(temperature_size,
      endpoint_node.child_by_type(ATTRIBUTE(HEALTH_BATTERY_TEMPERATURE)));
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing Battery Report Health frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}
///////////////////////////////////////////////////////////////////////////////
// Incoming commands handler
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_battery_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Frame too short, it should have not come here.
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case BATTERY_REPORT:
      return zwave_command_class_battery_handle_battery_report(connection_info,
                                                               frame_data,
                                                               frame_length);
    case BATTERY_HEALTH_REPORT_V2:
      return zwave_command_class_battery_handle_battery_health_report(
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
static void zwave_command_class_battery_on_version_attribute_update(
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

  // Now we know we have a battery supporting endpoint.
  attribute_store::attribute endpoint_node
    = version_node.first_parent(ATTRIBUTE_ENDPOINT_ID);

  // Create the battery attributes
  std::vector<attribute_store_node_t> battery_attributes;
  battery_attributes.push_back(ATTRIBUTE(BATTERY_LEVEL));

  if (supporting_node_version >= BATTERY_VERSION_V2) {
    // Battery report
    battery_attributes.push_back(ATTRIBUTE(CHARGING_STATUS));
    battery_attributes.push_back(ATTRIBUTE(RECHARGEABLE));
    battery_attributes.push_back(ATTRIBUTE(BACKUP_BATTERY));
    battery_attributes.push_back(ATTRIBUTE(OVERHEATING));
    battery_attributes.push_back(ATTRIBUTE(LOW_FLUID));
    battery_attributes.push_back(ATTRIBUTE(REPLACE_RECHARGE));
    battery_attributes.push_back(ATTRIBUTE(DISCONNECTED));
    // Health attributes
    battery_attributes.push_back(ATTRIBUTE(HEALTH_MAXIMUM_CAPACITY));
    battery_attributes.push_back(ATTRIBUTE(HEALTH_SCALE));
    battery_attributes.push_back(ATTRIBUTE(HEALTH_PRECISION));
    battery_attributes.push_back(ATTRIBUTE(HEALTH_BATTERY_TEMPERATURE));
  }

  if (supporting_node_version >= BATTERY_VERSION_V3) {
    // Battery report
    battery_attributes.push_back(ATTRIBUTE(LOW_TEMPERATURE));
  }

  for (auto attribute: battery_attributes) {
    endpoint_node.emplace_node(attribute);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_battery_init()
{
  // Attribute store callbacks
  attribute_store_register_callback_by_type(
    zwave_command_class_battery_on_version_attribute_update,
    ATTRIBUTE(VERSION));

  // Attribute resolver rules
  attribute_resolver_register_rule(ATTRIBUTE(BATTERY_LEVEL),
                                   NULL,
                                   zwave_command_class_battery_get);

  attribute_resolver_register_rule(ATTRIBUTE(HEALTH_MAXIMUM_CAPACITY),
                                   NULL,
                                   zwave_command_class_battery_health_get);

  // The support side of things: Register our handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  handler.support_handler         = NULL;
  handler.control_handler = &zwave_command_class_battery_control_handler;
  // Not supported, so this does not really matter
  handler.minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME;
  handler.manual_security_validation = false;
  handler.command_class              = COMMAND_CLASS_BATTERY;
  handler.version                    = BATTERY_VERSION_V3;
  handler.command_class_name         = "Battery";
  handler.comments                   = "";

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}
