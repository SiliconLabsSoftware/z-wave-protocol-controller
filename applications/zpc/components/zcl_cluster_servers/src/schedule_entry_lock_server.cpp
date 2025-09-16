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

#include "attribute.hpp"

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_schedule_entry_lock";

///////////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////////
attribute_store::attribute get_user_id_node(const dotdot_unid_t &unid,
                                            dotdot_endpoint_id_t endpoint,
                                            uint8_t user_identifier)
{
  attribute_store::attribute base_id_node
    = attribute_store_network_helper_get_node_id_node(unid);
  attribute_store::attribute endpoint_node
    = base_id_node.child_by_type_and_value(ATTRIBUTE_ENDPOINT_ID, endpoint);
  auto user_id_node
    = endpoint_node.child_by_type_and_value(ATTRIBUTE(USER_IDENTIFIER),
                                            user_identifier);

  return user_id_node;
}

sl_status_t
  zwave_command_class_check_ep_support(const dotdot_unid_t unid,
                                       const dotdot_endpoint_id_t endpoint)
{
  attribute_store::attribute base_id_node
    = attribute_store_network_helper_get_node_id_node(unid);
  attribute_store::attribute endpoint_node
    = base_id_node.child_by_type_and_value<>(ATTRIBUTE_ENDPOINT_ID, endpoint);

  // If both attribute are not defined, we definitely doesn't support this Command Class
  if (!endpoint_node.child_by_type(ATTRIBUTE(SLOTS_WEEK_DAY)).is_valid()
      && !endpoint_node.child_by_type(ATTRIBUTE(HOUR_TZO)).is_valid()) {
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

// Helper function to set the desired values based on the attribute map.
static sl_status_t helper_set_desired_values(
  attribute_store::attribute base_node,
  const std::map<attribute_store_type_t, uint8_t> &attribute_map)
{
  try {
    for (const auto &[attribute, value]: attribute_map) {
      auto child_node = base_node.emplace_node(attribute);
      child_node.set_desired(value);
    }
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG, "Error while setting desired values : %s", e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// commands handling callbacks
//////////////////////////////////////////////////////////////////////////////

static sl_status_t zwave_command_class_schedule_entry_lock_enable_set_callback(
  dotdot_unid_t unid,
  dotdot_endpoint_id_t endpoint,
  uic_mqtt_dotdot_callback_call_type_t call_type,
  uint8_t user_identifier,
  uint8_t enabled)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }
  try {
    auto user_id_node = get_user_id_node(unid, endpoint, user_identifier);
    user_id_node.child_by_type(ATTRIBUTE(ENABLED)).set_desired(enabled);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock enable Set : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_enable_all_set_callback(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    uic_mqtt_dotdot_callback_call_type_t call_type,
    uint8_t enabled)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }
  try {
    attribute_store::attribute base_id_node
      = attribute_store_network_helper_get_node_id_node(unid);
    attribute_store::attribute endpoint_node
      = base_id_node.child_by_type_and_value(ATTRIBUTE_ENDPOINT_ID, endpoint);
    endpoint_node.child_by_type(ATTRIBUTE(ENABLE_ALL)).set_desired<>(enabled);
  } catch (const std::exception &e) {
    sl_log_error(
      LOG_TAG,
      "Error while generating schedule_entry_lock enable all Set : %s",
      e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_week_day_set_callback(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    uic_mqtt_dotdot_callback_call_type_t call_type,
    uint8_t set_action,
    uint8_t user_identifier,
    uint8_t schedule_slotid,
    uint8_t day_of_week,
    uint8_t start_hour,
    uint8_t start_minute,
    uint8_t stop_hour,
    uint8_t stop_minute)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }
  try {
    auto user_id_node = get_user_id_node(unid, endpoint, user_identifier);
    auto slot_id_node = user_id_node.child_by_type_and_value(
      ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID),
      schedule_slotid);

    const std::map<attribute_store_type_t, uint8_t> attribute_map
      = {{ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION), set_action},
         {ATTRIBUTE(WEEK_DAY_SCHEDULE_DAY_OF_WEEK), day_of_week},
         {ATTRIBUTE(WEEK_DAY_SCHEDULE_START_HOUR), start_hour},
         {ATTRIBUTE(WEEK_DAY_SCHEDULE_START_MINUTE), start_minute},
         {ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_HOUR), stop_hour},
         {ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_MINUTE), stop_minute}};
    return helper_set_desired_values(slot_id_node, attribute_map);

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock week day Set : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_week_day_get_callback(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    uic_mqtt_dotdot_callback_call_type_t call_type,
    uint8_t user_identifier,
    uint8_t schedule_slotid)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }

  try {
    auto user_id_node = get_user_id_node(unid, endpoint, user_identifier);
    auto slot_id_node = user_id_node.child_by_type_and_value(
      ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID),
      schedule_slotid);
    auto set_action_node
      = slot_id_node.child_by_type(ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION));
    set_action_node.clear_reported();

  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while retrieving schedule_entry_lock weekday get: %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_year_day_set_callback(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    uic_mqtt_dotdot_callback_call_type_t call_type,
    uint8_t set_action,
    uint8_t user_identifier,
    uint8_t schedule_slotid,
    uint8_t start_year,
    uint8_t start_month,
    uint8_t start_day,
    uint8_t start_hour,
    uint8_t start_minute,
    uint8_t stop_year,
    uint8_t stop_month,
    uint8_t stop_day,
    uint8_t stop_hour,
    uint8_t stop_minute)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }

  try {
    auto user_id_node = get_user_id_node(unid, endpoint, user_identifier);
    auto slot_id_node = user_id_node.child_by_type_and_value(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID),
      schedule_slotid);

    const std::map<attribute_store_type_t, uint8_t> attribute_map
      = {{ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION), set_action},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_START_YEAR), start_year},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MONTH), start_month},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_START_DAY), start_day},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_START_HOUR), start_hour},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MINUTE), start_minute},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_YEAR), stop_year},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MONTH), stop_month},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_DAY), stop_day},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_HOUR), stop_hour},
         {ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MINUTE), stop_minute}};

    return helper_set_desired_values(slot_id_node, attribute_map);
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock yearday set: %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_year_day_get_callback(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    uic_mqtt_dotdot_callback_call_type_t call_type,
    uint8_t user_identifier,
    uint8_t schedule_slotid)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }
  try {
    auto user_id_node = get_user_id_node(unid, endpoint, user_identifier);
    auto slot_id_node = user_id_node.child_by_type_and_value(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID),
      schedule_slotid);
    auto set_action_node
      = slot_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION));
    set_action_node.clear_reported();
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while generating schedule_entry_lock yearday get : %s",
                 e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_daily_repeating_set_callback(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    uic_mqtt_dotdot_callback_call_type_t call_type,
    uint8_t set_action,
    uint8_t user_identifier,
    uint8_t schedule_slotid,
    uint8_t week_day_bitmask,
    uint8_t start_hour,
    uint8_t start_minute,
    uint8_t duration_hour,
    uint8_t duration_minute)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }
  try {
    auto user_id_node = get_user_id_node(unid, endpoint, user_identifier);
    auto slot_id_node = user_id_node.child_by_type_and_value(
      ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID),
      schedule_slotid);

    const std::map<attribute_store_type_t, uint8_t> attribute_map
      = {{ATTRIBUTE(DAILY_REPEATING_SET_ACTION), set_action},
         {ATTRIBUTE(DAILY_REPEATING_WEEK_DAY), week_day_bitmask},
         {ATTRIBUTE(DAILY_REPEATING_START_HOUR), start_hour},
         {ATTRIBUTE(DAILY_REPEATING_START_MINUTE), start_minute},
         {ATTRIBUTE(DAILY_REPEATING_DURATION_HOUR), duration_hour},
         {ATTRIBUTE(DAILY_REPEATING_DURATION_MINUTE), duration_minute}};

    return helper_set_desired_values(slot_id_node, attribute_map);
  } catch (const std::exception &e) {
    sl_log_error(
      LOG_TAG,
      "Error while generating schedule_entry_lock daily repeating Set: %s",
      e.what());
    return SL_STATUS_FAIL;
  }
}

static sl_status_t
  zwave_command_class_schedule_entry_lock_daily_repeating_get_callback(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    uic_mqtt_dotdot_callback_call_type_t call_type,
    uint8_t user_identifier,
    uint8_t schedule_slotid)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }

  try {
    attribute_store::attribute base_id_node
      = attribute_store_network_helper_get_node_id_node(unid);
    attribute_store::attribute endpoint_node
      = base_id_node.child_by_type_and_value(ATTRIBUTE_ENDPOINT_ID, endpoint);
    auto user_id_node
      = endpoint_node.child_by_type_and_value(ATTRIBUTE(USER_IDENTIFIER),
                                              user_identifier);
    auto slot_id_node = user_id_node.child_by_type_and_value(
      ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID),
      schedule_slotid);
    auto set_action_node
      = slot_id_node.child_by_type(ATTRIBUTE(DAILY_REPEATING_SET_ACTION));
    set_action_node.clear_reported();
  } catch (const std::exception &e) {
    sl_log_error(
      LOG_TAG,
      "Error while generating schedule_entry_lock daily repeating Set : %s",
      e.what());
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

sl_status_t zwave_command_class_schedule_entry_lock_write_attributes_callback(
  const dotdot_unid_t unid,
  const dotdot_endpoint_id_t endpoint,
  uic_mqtt_dotdot_callback_call_type_t call_type,
  uic_mqtt_dotdot_unify_schedule_entry_lock_state_t lock_state,
  uic_mqtt_dotdot_unify_schedule_entry_lock_updated_state_t updated_lock_state)
{
  if (call_type == UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK) {
    return zwave_command_class_check_ep_support(unid, endpoint);
  }

  std::map<attribute_store_node_t, uint8_t> attributes_to_update;

  if (updated_lock_state.slots_week_day) {
    attributes_to_update
      [DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_SLOTS_WEEK_DAY]
      = lock_state.slots_week_day;
  }

  if (updated_lock_state.slots_year_day) {
    attributes_to_update
      [DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_SLOTS_YEAR_DAY]
      = lock_state.slots_year_day;
  }

  if (updated_lock_state.signtzo) {
    attributes_to_update[DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_SIGNTZO]
      = lock_state.signtzo;
  }

  if (updated_lock_state.hourtzo) {
    attributes_to_update[DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_HOURTZO]
      = lock_state.hourtzo;
  }

  if (updated_lock_state.minutetzo) {
    attributes_to_update
      [DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_MINUTETZO]
      = lock_state.minutetzo;
  }

  if (updated_lock_state.dst_offset_sign) {
    attributes_to_update
      [DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_DST_OFFSET_SIGN]
      = lock_state.dst_offset_sign;
  }

  if (updated_lock_state.dst_offset_minute) {
    attributes_to_update
      [DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_DST_OFFSET_MINUTE]
      = lock_state.dst_offset_minute;
  }

  if (updated_lock_state.number_of_slots_daily_repeating) {
    attributes_to_update
      [DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_NUMBER_OF_SLOTS_DAILY_REPEATING]
      = lock_state.number_of_slots_daily_repeating;
  }

  attribute_store::attribute base_id_node
    = attribute_store_network_helper_get_node_id_node(unid);
  attribute_store::attribute endpoint_node
    = base_id_node.child_by_type_and_value(ATTRIBUTE_ENDPOINT_ID, endpoint);

  for (const auto &[attribute_id, value]: attributes_to_update) {
    attribute_store::attribute attribute_node
      = endpoint_node.emplace_node(attribute_id);
    attribute_node.set_desired<>(value);
    sl_log_debug(LOG_TAG,
                 "write callback set desired attribute %u %u",
                 attribute_id,
                 attribute_node);
  }

  return SL_STATUS_OK;
}

sl_status_t zwave_command_class_publish_generated_week_day_report_command(
  dotdot_unid_t unid,
  dotdot_endpoint_id_t endpoint,
  attribute_store_node_t ep_node)
{
  attribute_store::attribute endpoint_node(ep_node);

  uic_mqtt_dotdot_unify_schedule_entry_lock_command_week_day_report_fields_t
    fields;
  attribute_store::attribute user_id_node
    = endpoint_node.child_by_type(ATTRIBUTE(USER_IDENTIFIER));
  attribute_store::attribute week_day_slot_id_node
    = user_id_node.child_by_type(ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID));
  attribute_store::attribute day_of_week_node
    = week_day_slot_id_node.child_by_type(
      ATTRIBUTE(WEEK_DAY_SCHEDULE_DAY_OF_WEEK));
  attribute_store::attribute start_hour_node
    = week_day_slot_id_node.child_by_type(
      ATTRIBUTE(WEEK_DAY_SCHEDULE_START_HOUR));
  attribute_store::attribute start_minute_node
    = week_day_slot_id_node.child_by_type(
      ATTRIBUTE(WEEK_DAY_SCHEDULE_START_MINUTE));
  attribute_store::attribute stop_hour_node
    = week_day_slot_id_node.child_by_type(
      ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_HOUR));
  attribute_store::attribute stop_minute_node
    = week_day_slot_id_node.child_by_type(
      ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_MINUTE));
  fields.user_identifier = user_id_node.reported<uint8_t>();
  fields.schedule_slotid = week_day_slot_id_node.reported<uint8_t>();
  fields.day_of_week     = day_of_week_node.reported<uint8_t>();
  fields.start_hour      = start_hour_node.reported<uint8_t>();
  fields.start_minute    = start_minute_node.reported<uint8_t>();
  fields.stop_hour       = stop_hour_node.reported<uint8_t>();
  fields.stop_minute     = stop_minute_node.reported<uint8_t>();
  uic_mqtt_dotdot_unify_schedule_entry_lock_publish_generated_week_day_report_command(
    unid,
    endpoint,
    &fields);
  return SL_STATUS_OK;
}

sl_status_t zwave_command_class_publish_generated_year_day_report_command(
  dotdot_unid_t unid,
  dotdot_endpoint_id_t endpoint,
  attribute_store_node_t ep_node)
{
  attribute_store::attribute endpoint_node(ep_node);

  uic_mqtt_dotdot_unify_schedule_entry_lock_command_year_day_report_fields_t
    fields;
  attribute_store::attribute user_id_node
    = endpoint_node.child_by_type(ATTRIBUTE(USER_IDENTIFIER));
  attribute_store::attribute year_day_slot_id_node
    = user_id_node.child_by_type(ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID));
  attribute_store::attribute start_year_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_START_YEAR));
  attribute_store::attribute start_month_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MONTH));
  attribute_store::attribute start_day_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_START_DAY));
  attribute_store::attribute start_hour_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_START_HOUR));
  attribute_store::attribute start_minute_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MINUTE));
  attribute_store::attribute stop_year_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_YEAR));
  attribute_store::attribute stop_month_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MONTH));
  attribute_store::attribute stop_day_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_DAY));
  attribute_store::attribute stop_hour_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_HOUR));
  attribute_store::attribute stop_minute_node
    = year_day_slot_id_node.child_by_type(
      ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MINUTE));
  fields.user_identifier = user_id_node.reported<uint8_t>();
  fields.schedule_slotid = year_day_slot_id_node.reported<uint8_t>();
  fields.start_year      = start_year_node.reported<uint8_t>();
  fields.start_month     = start_month_node.reported<uint8_t>();
  fields.start_day       = start_day_node.reported<uint8_t>();
  fields.start_hour      = start_hour_node.reported<uint8_t>();
  fields.start_minute    = start_minute_node.reported<uint8_t>();
  fields.stop_year       = stop_year_node.reported<uint8_t>();
  fields.stop_month      = stop_month_node.reported<uint8_t>();
  fields.stop_day        = stop_day_node.reported<uint8_t>();
  fields.stop_hour       = stop_hour_node.reported<uint8_t>();
  fields.stop_minute     = stop_minute_node.reported<uint8_t>();

  uic_mqtt_dotdot_unify_schedule_entry_lock_publish_generated_year_day_report_command(
    unid,
    endpoint,
    &fields);
  return SL_STATUS_OK;
}

sl_status_t
  zwave_command_class_publish_generated_daily_repeating_report_command(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    attribute_store_node_t ep_node)
{
  attribute_store::attribute endpoint_node(ep_node);

  uic_mqtt_dotdot_unify_schedule_entry_lock_command_daily_repeating_report_fields_t
    fields;
  attribute_store::attribute user_id_node
    = endpoint_node.child_by_type(ATTRIBUTE(USER_IDENTIFIER));
  attribute_store::attribute daily_repeating_slot_id_node
    = user_id_node.child_by_type(ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID));
  attribute_store::attribute week_day_node
    = daily_repeating_slot_id_node.child_by_type(
      ATTRIBUTE(DAILY_REPEATING_WEEK_DAY));
  attribute_store::attribute start_hour_node
    = daily_repeating_slot_id_node.child_by_type(
      ATTRIBUTE(DAILY_REPEATING_START_HOUR));
  attribute_store::attribute start_minute_node
    = daily_repeating_slot_id_node.child_by_type(
      ATTRIBUTE(DAILY_REPEATING_START_MINUTE));
  attribute_store::attribute duartion_hour_node
    = daily_repeating_slot_id_node.child_by_type(
      ATTRIBUTE(DAILY_REPEATING_DURATION_HOUR));
  attribute_store::attribute duration_minute_node
    = daily_repeating_slot_id_node.child_by_type(
      ATTRIBUTE(DAILY_REPEATING_DURATION_MINUTE));
  fields.user_identifier  = user_id_node.reported<uint8_t>();
  fields.schedule_slotid  = daily_repeating_slot_id_node.reported<uint8_t>();
  fields.week_day_bitmask = week_day_node.reported<uint8_t>();
  fields.start_hour       = start_hour_node.reported<uint8_t>();
  fields.start_minute     = start_minute_node.reported<uint8_t>();
  fields.duration_hour    = duartion_hour_node.reported<uint8_t>();
  fields.duration_minute  = duration_minute_node.reported<uint8_t>();
  uic_mqtt_dotdot_unify_schedule_entry_lock_publish_generated_daily_repeating_report_command(
    unid,
    endpoint,
    &fields);
  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  Init and teardown functions.
//////////////////////////////////////////////////////////////////////////////
sl_status_t schedule_entry_lock_cluster_server_init(void)
{
  sl_log_debug(LOG_TAG, "Schedule entry lock cluster server initialization");

  uic_mqtt_dotdot_unify_schedule_entry_lock_enable_set_callback_set(
    zwave_command_class_schedule_entry_lock_enable_set_callback);

  uic_mqtt_dotdot_unify_schedule_entry_lock_enable_all_set_callback_set(
    zwave_command_class_schedule_entry_lock_enable_all_set_callback);

  uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_set_callback_set(
    zwave_command_class_schedule_entry_lock_week_day_set_callback);

  uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_get_callback_set(
    zwave_command_class_schedule_entry_lock_week_day_get_callback);

  uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_set_callback_set(
    zwave_command_class_schedule_entry_lock_year_day_set_callback);

  uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_get_callback_set(
    zwave_command_class_schedule_entry_lock_year_day_get_callback);

  uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_set_callback_set(
    zwave_command_class_schedule_entry_lock_daily_repeating_set_callback);

  uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_get_callback_set(
    zwave_command_class_schedule_entry_lock_daily_repeating_get_callback);

  uic_mqtt_dotdot_set_unify_schedule_entry_lock_write_attributes_callback(
    zwave_command_class_schedule_entry_lock_write_attributes_callback);

  return SL_STATUS_OK;
}
