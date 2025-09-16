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
#include "zwave_command_class_schedule_entry_lock.h"
#include "zwave_command_classes_utils.h"
#include "schedule_entry_lock_server.h"

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
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_schedule_entry_lock";

// Cpp helpers
namespace
{
zwave_frame_generator frame_generator(
  COMMAND_CLASS_SCHEDULE_ENTRY_LOCK);  //NOSONAR - false positive since it is warped in a namespace
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////
zwave_cc_version_t
  get_current_schedule_entry_lock_version(attribute_store_node_t node)
{
  zwave_cc_version_t version = zwave_command_class_get_version_from_node(
    node,
    COMMAND_CLASS_SCHEDULE_ENTRY_LOCK);

  if (version == 0) {
    sl_log_error(LOG_TAG,
                 "schedule_entry_lock Command Class Version not found");
  }

  return version;
}

///////////////////////////////////////////////////////////////////////////////
// Resolution functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_schedule_entry_lock_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  if (attribute_store_get_node_type(node) == ATTRIBUTE(SLOTS_WEEK_DAY)) {
    return frame_generator.generate_no_args_frame(
      SCHEDULE_ENTRY_TYPE_SUPPORTED_GET,
      frame,
      frame_length);
  }

  return SL_STATUS_INVALID_TYPE;
}

static sl_status_t zwave_command_class_schedule_entry_lock_time_offset_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  if (attribute_store_get_node_type(node) == ATTRIBUTE(HOUR_TZO)) {
    return frame_generator.generate_no_args_frame(
      SCHEDULE_ENTRY_LOCK_TIME_OFFSET_GET_V2,
      frame,
      frame_length);
  }
  return SL_STATUS_INVALID_TYPE;
}

static sl_status_t zwave_command_class_schedule_entry_lock_week_day_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  if (attribute_store_get_node_type(node)
      == ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION)) {
    try {
      // Retrieve the node attributes
      attribute_store::attribute weekday_schedule_set_action_node(node);
      auto weekday_schedule_slot_id_node = weekday_schedule_set_action_node.parent();
      auto user_id_node = weekday_schedule_slot_id_node.first_parent(
        ATTRIBUTE(USER_IDENTIFIER));

      // Compute expected size for the set frame
      const uint8_t expected_frame_size = 4;

      // Initialize the frame for Schedule Entry Lock Time Offset Set command
      frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_WEEK_DAY_GET,
                                       frame,
                                       expected_frame_size);
      frame_generator.add_value(user_id_node, DESIRED_OR_REPORTED_ATTRIBUTE);
      frame_generator.add_value(weekday_schedule_slot_id_node,
                                DESIRED_OR_REPORTED_ATTRIBUTE);
      // Validate the constructed frame and set the frame length
      frame_generator.validate_frame(frame_length);
    } catch (const std::exception &e) {
      // Log any error that occurs during the frame generation process
      sl_log_error(LOG_TAG,
                   "Error while generating Schedule Entry Lock Set frame: %s",
                   e.what());
      return SL_STATUS_FAIL;
    }
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_year_day_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  if (attribute_store_get_node_type(node)
      == ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION)) {
    try {
      // Retrieve the node attributes
      attribute_store::attribute yearday_schedule_set_action_node(node);
      auto yearday_schedule_slot_id_node = yearday_schedule_set_action_node.parent();
      auto user_id_node = yearday_schedule_slot_id_node.first_parent(
        ATTRIBUTE(USER_IDENTIFIER));

      // Compute expected size for the set frame
      const uint8_t expected_frame_size = 4;

      // Initialize the frame for Schedule Entry Lock Time Offset Set command
      frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_YEAR_DAY_GET,
                                       frame,
                                       expected_frame_size);
      frame_generator.add_value(user_id_node, DESIRED_OR_REPORTED_ATTRIBUTE);
      frame_generator.add_value(yearday_schedule_slot_id_node,
                                DESIRED_OR_REPORTED_ATTRIBUTE);
      // Validate the constructed frame and set the frame length
      frame_generator.validate_frame(frame_length);
    } catch (const std::exception &e) {
      // Log any error that occurs during the frame generation process
      sl_log_error(LOG_TAG,
                   "Error while generating Schedule Entry Lock Set frame: %s",
                   e.what());
      return SL_STATUS_FAIL;
    }
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_daily_repeating_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  if (attribute_store_get_node_type(node)
      == ATTRIBUTE(DAILY_REPEATING_SET_ACTION)) {
    try {
      // Retrieve the node attributes
      attribute_store::attribute dailyrepeating_schedule_set_action_node(node);
      auto dailyrepeating_schedule_slot_id_node = dailyrepeating_schedule_set_action_node.parent();
      auto user_id_node = dailyrepeating_schedule_slot_id_node.first_parent(
        ATTRIBUTE(USER_IDENTIFIER));

      // Compute expected size for the set frame
      const uint8_t expected_frame_size = 4;

      // Initialize the frame for Schedule Entry Lock Time Offset Set command
      frame_generator.initialize_frame(
        SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_GET_V3,
        frame,
        expected_frame_size);
      frame_generator.add_value(user_id_node, DESIRED_OR_REPORTED_ATTRIBUTE);
      frame_generator.add_value(dailyrepeating_schedule_slot_id_node,
                                DESIRED_OR_REPORTED_ATTRIBUTE);
      // Validate the constructed frame and set the frame length
      frame_generator.validate_frame(frame_length);
    } catch (const std::exception &e) {
      // Log any error that occurs during the frame generation process
      sl_log_error(LOG_TAG,
                   "Error while generating Schedule Entry Lock Set frame: %s",
                   e.what());
      return SL_STATUS_FAIL;
    }
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_time_offset_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    // Retrieve the node attributes
    attribute_store::attribute hour_tzo_node(node);
    auto endpoint_node   = hour_tzo_node.parent();
    auto minute_tzo_node = endpoint_node.child_by_type(ATTRIBUTE(MINUTE_TZO));
    auto sign_tzo_node = endpoint_node.child_by_type(ATTRIBUTE(SIGN_TZO));
    auto sign_offset_dst_node
      = endpoint_node.child_by_type(ATTRIBUTE(DST_OFFSET_SIGN));
    auto minute_offset_dst_node
      = endpoint_node.child_by_type(ATTRIBUTE(DST_OFFSET_MINUTE));

    // Compute expected size for the set frame
    const uint8_t expected_frame_size = 5;

    // Initialize the frame for Schedule Entry Lock Time Offset Set command
    frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_TIME_OFFSET_SET_V2,
                                     frame,
                                     expected_frame_size);

    // Create a vector for the values to be shifted into the frame
    frame_generator.add_shifted_values(
      {{.left_shift       = 7,  // Sign TZO (1 bit)
        .node             = sign_tzo_node,
        .node_value_state = DESIRED_OR_REPORTED_ATTRIBUTE},
       {.left_shift       = 0,  // Hour TZO (7 bits)
        .node             = hour_tzo_node,
        .node_value_state = DESIRED_OR_REPORTED_ATTRIBUTE}});
    frame_generator.add_value(minute_tzo_node, DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_shifted_values(
      {{.left_shift       = 7,  // Sign Offset DST (1 bit)
        .node             = sign_offset_dst_node,
        .node_value_state = DESIRED_OR_REPORTED_ATTRIBUTE},
       {.left_shift       = 0,  // Minute Offset DST (7 bits)
        .node             = minute_offset_dst_node,
        .node_value_state = DESIRED_OR_REPORTED_ATTRIBUTE}});

    // Validate the constructed frame and set the frame length
    frame_generator.validate_frame(frame_length);

  } catch (const std::exception &e) {
    // Log any error that occurs during the frame generation process
    sl_log_error(LOG_TAG,
                 "Error while generating Schedule Entry Lock Set frame: %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_enable_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute enable_node(node);
    auto user_id_node = enable_node.parent();

    // Compute expected size for set frame
    const uint8_t expected_frame_size = 4;

    // Creating the frame
    frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_ENABLE_SET,
                                     frame,
                                     expected_frame_size);
    frame_generator.add_value(user_id_node, DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(enable_node, DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.validate_frame(frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_enable_all_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute value_node(node);

    // Compute expected size for set frame
    const uint8_t expected_frame_size = 3;

    // Creating the frame
    frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_ENABLE_ALL_SET,
                                     frame,
                                     expected_frame_size);
    frame_generator.add_value(value_node, DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.validate_frame(frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_week_day_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute weekday_schedule_set_action_node(node);
    auto user_id_node = weekday_schedule_set_action_node.first_parent(
      ATTRIBUTE(USER_IDENTIFIER));
    auto weekday_schedule_slot_id_node
      = weekday_schedule_set_action_node.parent();
    auto weekday_schedule_dayofweek_node
      = weekday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_DAY_OF_WEEK));
    auto weekday_schedule_starthour_node
      = weekday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_START_HOUR));
    auto weekday_schedule_startminute_node
      = weekday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_START_MINUTE));
    auto weekday_schedule_stophour_node
      = weekday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_HOUR));
    auto weekday_schedule_stopminute_node
      = weekday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_MINUTE));

    // Compute expected size for set frame
    const uint8_t expected_frame_size = 10;

    // Creating the frame
    frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_WEEK_DAY_SET,
                                     frame,
                                     expected_frame_size);
    frame_generator.add_value(weekday_schedule_set_action_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(user_id_node, DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(weekday_schedule_slot_id_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(weekday_schedule_dayofweek_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(weekday_schedule_starthour_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(weekday_schedule_startminute_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(weekday_schedule_stophour_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(weekday_schedule_stopminute_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.validate_frame(frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_year_day_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute yearday_schedule_set_action_node(node);
    auto user_id_node = yearday_schedule_set_action_node.first_parent(
      ATTRIBUTE(USER_IDENTIFIER));
    auto yearday_schedule_slot_id_node
      = yearday_schedule_set_action_node.parent();
    auto yearday_schedule_startyear_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_START_YEAR));
    auto yearday_schedule_startmonth_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MONTH));
    auto yearday_schedule_startday_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_START_DAY));
    auto yearday_schedule_starthour_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_START_HOUR));
    auto yearday_schedule_startminute_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MINUTE));
    auto yearday_schedule_stopyear_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_YEAR));
    auto yearday_schedule_stopmonth_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MONTH));
    auto yearday_schedule_stopday_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_DAY));
    auto yearday_schedule_stophour_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_HOUR));
    auto yearday_schedule_stopminute_node
      = yearday_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MINUTE));

    // Compute expected size for set frame
    const uint8_t expected_frame_size = 15;

    // Creating the frame
    frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_YEAR_DAY_SET,
                                     frame,
                                     expected_frame_size);
    frame_generator.add_value(yearday_schedule_set_action_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(user_id_node, DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_slot_id_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_startyear_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_startmonth_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_startday_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_starthour_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_startminute_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_stopyear_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_stopmonth_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_stopday_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_stophour_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(yearday_schedule_stopminute_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.validate_frame(frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_daily_repeating_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  try {
    attribute_store::attribute dailyrepeating_schedule_set_action_node(node);
    auto user_id_node = dailyrepeating_schedule_set_action_node.first_parent(
      ATTRIBUTE(USER_IDENTIFIER));
    auto dailyrepeating_schedule_slot_id_node
      = dailyrepeating_schedule_set_action_node.parent();
    auto dailyrepeating_weekday_node
      = dailyrepeating_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_WEEK_DAY));
    auto dailyrepeating_starthour_node
      = dailyrepeating_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_START_HOUR));
    auto dailyrepeating_startminute_node
      = dailyrepeating_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_START_MINUTE));
    auto dailyrepeating_durationhour_node
      = dailyrepeating_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_DURATION_HOUR));
    auto dailyrepeating_durationminute_node
      = dailyrepeating_schedule_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_DURATION_MINUTE));

    // Compute expected size for set frame
    const uint8_t expected_frame_size = 10;

    // Creating the frame
    frame_generator.initialize_frame(SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_SET_V3,
                                     frame,
                                     expected_frame_size);
    frame_generator.add_value(dailyrepeating_schedule_set_action_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(user_id_node, DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(dailyrepeating_schedule_slot_id_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(dailyrepeating_weekday_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(dailyrepeating_starthour_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(dailyrepeating_startminute_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(dailyrepeating_durationhour_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.add_value(dailyrepeating_durationminute_node,
                              DESIRED_OR_REPORTED_ATTRIBUTE);
    frame_generator.validate_frame(frame_length);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock Set frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Frame parsing functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t
  zwave_command_class_schedule_entry_lock_type_supported_report(
    const zwave_controller_connection_info_t *connection_info,
    const uint8_t *frame_data,
    uint16_t frame_length)
{
  //  Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));
  auto current_version = get_current_schedule_entry_lock_version(endpoint_node);

  sl_log_debug(LOG_TAG, "schedule_entry_lock Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = 5;

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for schedule_entry_lock Report frame");
      return SL_STATUS_FAIL;
    }

    attribute_store::attribute week_day_node
      = endpoint_node.child_by_type(ATTRIBUTE(SLOTS_WEEK_DAY));
    parser.read_byte(week_day_node);
    attribute_store::attribute year_day_node
      = endpoint_node.child_by_type(ATTRIBUTE(SLOTS_YEAR_DAY));
    parser.read_byte(year_day_node);

    if (current_version >= 3) {
      attribute_store::attribute number_of_slot_daily_repeating_node
        = endpoint_node.child_by_type(
          ATTRIBUTE(NUMBER_OF_SLOTS_DAILY_REPEATING));
      parser.read_byte(number_of_slot_daily_repeating_node);
    }

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing schedule_entry_lock Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_week_day_report(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  unid_t unid;
  dotdot_endpoint_id_t endpoint;

  //  Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG, "schedule_entry_lock Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = 9;

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for schedule_entry_lock Report frame");
      return SL_STATUS_FAIL;
    }

    attribute_store::attribute user_id_node
      = endpoint_node.child_by_type(ATTRIBUTE(USER_IDENTIFIER));
    parser.read_byte(user_id_node);
    attribute_store::attribute week_day_slot_id_node
      = user_id_node.child_by_type(ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID));
    parser.read_byte(week_day_slot_id_node);
    attribute_store::attribute day_of_week_node
      = week_day_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_DAY_OF_WEEK));
    parser.read_byte(day_of_week_node);
    attribute_store::attribute start_hour_node
      = week_day_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_START_HOUR));
    parser.read_byte(start_hour_node);
    attribute_store::attribute start_minute_node
      = week_day_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_START_MINUTE));
    parser.read_byte(start_minute_node);
    attribute_store::attribute stop_hour_node
      = week_day_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_HOUR));
    parser.read_byte(stop_hour_node);
    attribute_store::attribute stop_minute_node
      = week_day_slot_id_node.child_by_type(
        ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_MINUTE));
    parser.read_byte(stop_minute_node);
    attribute_store_network_helper_get_unid_from_node(endpoint_node, unid);
    endpoint = endpoint_node.reported<uint8_t>();
    zwave_command_class_publish_generated_week_day_report_command(
      unid,
      endpoint,
      endpoint_node);

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing schedule_entry_lock Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_year_day_report(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  unid_t unid;
  dotdot_endpoint_id_t endpoint;

  //  Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG, "schedule_entry_lock Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = 14;

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for schedule_entry_lock Report frame");
      return SL_STATUS_FAIL;
    }

    attribute_store::attribute user_id_node
      = endpoint_node.child_by_type(ATTRIBUTE(USER_IDENTIFIER));
    parser.read_byte(user_id_node);
    attribute_store::attribute year_day_slot_id_node
      = user_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID));
    parser.read_byte(year_day_slot_id_node);
    attribute_store::attribute start_year_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_YEAR));
    parser.read_byte(start_year_node);
    attribute_store::attribute start_month_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MONTH));
    parser.read_byte(start_month_node);
    attribute_store::attribute start_day_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_DAY));
    parser.read_byte(start_day_node);
    attribute_store::attribute start_hour_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_HOUR));
    parser.read_byte(start_hour_node);
    attribute_store::attribute start_minute_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MINUTE));
    parser.read_byte(start_minute_node);
    attribute_store::attribute stop_year_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_YEAR));
    parser.read_byte(stop_year_node);
    attribute_store::attribute stop_month_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MONTH));
    parser.read_byte(stop_month_node);
    attribute_store::attribute stop_day_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_DAY));
    parser.read_byte(stop_day_node);
    attribute_store::attribute stop_hour_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_HOUR));
    parser.read_byte(stop_hour_node);
    attribute_store::attribute stop_minute_node
      = year_day_slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MINUTE));
    parser.read_byte(stop_minute_node);
    attribute_store_network_helper_get_unid_from_node(endpoint_node, unid);
    endpoint = endpoint_node.reported<uint8_t>();
    zwave_command_class_publish_generated_year_day_report_command(
      unid,
      endpoint,
      endpoint_node);

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing schedule_entry_lock Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_schedule_entry_lock_time_offset_report(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  //  Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG, "schedule_entry_lock Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = 5;

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for schedule_entry_lock Report frame");
      return SL_STATUS_FAIL;
    }

    attribute_store::attribute sign_tzo_node
      = endpoint_node.child_by_type(ATTRIBUTE(SIGN_TZO));
    attribute_store::attribute hour_tzo_node
      = endpoint_node.child_by_type(ATTRIBUTE(HOUR_TZO));
    std::vector<zwave_frame_parser::bitmask_data> data_first_byte = {
    {.bitmask = SCHEDULE_ENTRY_LOCK_TIME_OFFSET_REPORT_LEVEL_SIGN_TZO_BIT_MASK_V2, .destination_node = sign_tzo_node},
    {.bitmask = SCHEDULE_ENTRY_LOCK_TIME_OFFSET_REPORT_LEVEL_HOUR_TZO_MASK_V2, .destination_node = hour_tzo_node}
    };
    parser.read_byte_with_bitmask(data_first_byte);
    attribute_store::attribute minute_tzo_node
      = endpoint_node.child_by_type(ATTRIBUTE(MINUTE_TZO));
    parser.read_byte(minute_tzo_node);
    attribute_store::attribute dst_offset_sign_node
      = endpoint_node.child_by_type(ATTRIBUTE(DST_OFFSET_SIGN));
    attribute_store::attribute dst_offset_minute_node
      = endpoint_node.child_by_type(ATTRIBUTE(DST_OFFSET_MINUTE));
    std::vector<zwave_frame_parser::bitmask_data> data_third_byte = {
    {.bitmask = SCHEDULE_ENTRY_LOCK_TIME_OFFSET_REPORT_LEVEL2_SIGN_OFFSET_DST_BIT_MASK_V2, .destination_node = dst_offset_sign_node},
    {.bitmask = SCHEDULE_ENTRY_LOCK_TIME_OFFSET_REPORT_LEVEL2_MINUTE_OFFSET_DST_MASK_V2, .destination_node = dst_offset_minute_node}
    };
    parser.read_byte_with_bitmask(data_third_byte);

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing schedule_entry_lock Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_daily_repeating_report(
    const zwave_controller_connection_info_t *connection_info,
    const uint8_t *frame_data,
    uint16_t frame_length)
{
  unid_t unid;
  dotdot_endpoint_id_t endpoint;

  //  Setup
  attribute_store::attribute endpoint_node(
    zwave_command_class_get_endpoint_node(connection_info));

  sl_log_debug(LOG_TAG, "schedule_entry_lock Report frame received");

  // Compute expected size for report frame
  const uint8_t expected_size = 9;

  // Parse the frame
  try {
    zwave_frame_parser parser(frame_data, frame_length);

    if (!parser.is_frame_size_valid(expected_size)) {
      sl_log_error(LOG_TAG,
                   "Invalid frame size for schedule_entry_lock Report frame");
      return SL_STATUS_FAIL;
    }

    attribute_store::attribute user_id_node
      = endpoint_node.child_by_type(ATTRIBUTE(USER_IDENTIFIER));
    parser.read_byte(user_id_node);
    attribute_store::attribute daily_repeating_slot_id_node
      = user_id_node.child_by_type(ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID));
    parser.read_byte(daily_repeating_slot_id_node);
    attribute_store::attribute week_day_node
      = daily_repeating_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_WEEK_DAY));
    parser.read_byte(week_day_node);
    attribute_store::attribute start_hour_node
      = daily_repeating_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_START_HOUR));
    parser.read_byte(start_hour_node);
    attribute_store::attribute start_minute_node
      = daily_repeating_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_START_MINUTE));
    parser.read_byte(start_minute_node);
    attribute_store::attribute duration_hour_node
      = daily_repeating_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_DURATION_HOUR));
    parser.read_byte(duration_hour_node);
    attribute_store::attribute duration_minute_node
      = daily_repeating_slot_id_node.child_by_type(
        ATTRIBUTE(DAILY_REPEATING_DURATION_MINUTE));
    parser.read_byte(duration_minute_node);
    attribute_store_network_helper_get_unid_from_node(endpoint_node, unid);
    endpoint = endpoint_node.reported<uint8_t>();
    zwave_command_class_publish_generated_daily_repeating_report_command(
      unid,
      endpoint,
      endpoint_node);

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while parsing schedule_entry_lock Report frame : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Incoming commands handler
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_schedule_entry_lock_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Frame too short, it should have not come here.
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }
  switch (frame_data[COMMAND_INDEX]) {
    case SCHEDULE_ENTRY_TYPE_SUPPORTED_REPORT:
      return zwave_command_class_schedule_entry_lock_type_supported_report(
        connection_info,
        frame_data,
        frame_length);
    case SCHEDULE_ENTRY_LOCK_WEEK_DAY_REPORT:
      return zwave_command_class_schedule_entry_lock_week_day_report(
        connection_info,
        frame_data,
        frame_length);
    case SCHEDULE_ENTRY_LOCK_YEAR_DAY_REPORT:
      return zwave_command_class_schedule_entry_lock_year_day_report(
        connection_info,
        frame_data,
        frame_length);
    case SCHEDULE_ENTRY_LOCK_TIME_OFFSET_REPORT_V2:
      return zwave_command_class_schedule_entry_lock_time_offset_report(
        connection_info,
        frame_data,
        frame_length);
    case SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_REPORT_V3:
      return zwave_command_class_schedule_entry_lock_daily_repeating_report(
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
static void zwave_command_class_schedule_entry_lock_on_version_attribute_update(
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

  // Now we know we have a schedule_entry_lock supporting endpoint.
  attribute_store::attribute endpoint_node
    = version_node.first_parent(ATTRIBUTE_ENDPOINT_ID);

  // Create the schedule_entry_lock attributes
  attribute_store::attribute user_id_node
    = endpoint_node.emplace_node(ATTRIBUTE(USER_IDENTIFIER));

  user_id_node.emplace_node(ATTRIBUTE(ENABLED));

  endpoint_node.emplace_node(ATTRIBUTE(ENABLE_ALL));

  endpoint_node.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));

  if (supporting_node_version >= 2) {
    endpoint_node.emplace_node(ATTRIBUTE(HOUR_TZO));
  }
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_schedule_entry_lock_init()
{
  // Attribute store callbacks
  attribute_store_register_callback_by_type(
    zwave_command_class_schedule_entry_lock_on_version_attribute_update,
    ATTRIBUTE(VERSION));

  // Attribute resolver rules
  attribute_resolver_register_rule(
    ATTRIBUTE(SLOTS_WEEK_DAY),
    NULL,
    &zwave_command_class_schedule_entry_lock_get);
  attribute_resolver_register_rule(
    ATTRIBUTE(HOUR_TZO),
    &zwave_command_class_schedule_entry_lock_time_offset_set,
    &zwave_command_class_schedule_entry_lock_time_offset_get);
  attribute_resolver_register_rule(
    ATTRIBUTE(ENABLED),
    &zwave_command_class_schedule_entry_lock_enable_set,
    NULL);
  attribute_resolver_register_rule(
    ATTRIBUTE(ENABLE_ALL),
    &zwave_command_class_schedule_entry_lock_enable_all_set,
    NULL);
  attribute_resolver_register_rule(
    ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION),
    &zwave_command_class_schedule_entry_lock_week_day_set,
    &zwave_command_class_schedule_entry_lock_week_day_get);
  attribute_resolver_register_rule(
    ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION),
    &zwave_command_class_schedule_entry_lock_year_day_set,
    &zwave_command_class_schedule_entry_lock_year_day_get);
  attribute_resolver_register_rule(
    ATTRIBUTE(DAILY_REPEATING_SET_ACTION),
    &zwave_command_class_schedule_entry_lock_daily_repeating_set,
    &zwave_command_class_schedule_entry_lock_daily_repeating_get);

  // The support side of things: Register our handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  handler.support_handler         = NULL;
  handler.control_handler
    = &zwave_command_class_schedule_entry_lock_control_handler;
  // Not supported, so this does not really matter
  handler.minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME;
  handler.manual_security_validation = false;
  handler.command_class              = COMMAND_CLASS_SCHEDULE_ENTRY_LOCK;
  handler.version                    = SCHEDULE_ENTRY_LOCK_VERSION_V3;
  handler.command_class_name         = "schedule_entry_lock";
  handler.comments                   = "";

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}
