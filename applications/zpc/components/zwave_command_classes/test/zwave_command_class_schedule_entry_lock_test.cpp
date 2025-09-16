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
// Base class
#include "zwave_command_class_schedule_entry_lock.h"
#include "zwave_command_classes_utils.h"
#include "unity.h"

// Generic includes
#include <string.h>

// Unify
#include "datastore.h"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
// Interface includes
#include "ZW_classcmd.h"

// ZPC includes
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store_type_registration.h"
#include "sl_log.h"

// Test helpers
#include "zwave_command_class_test_helper.hpp"

constexpr uint8_t SCHEDULE_ENTRY_LOCK_ENABLE      = 0x01;
constexpr uint8_t SCHEDULE_ENTRY_LOCK_DISABLE     = 0x00;
constexpr uint8_t SCHEDULE_ENTRY_LOCK_ERASE_SLOT  = 0;
constexpr uint8_t SCHEDULE_ENTRY_LOCK_MODIFY_SLOT = 1;

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK_##type

using namespace zwave_command_class_test_helper;

extern "C" {

// Mock helper
#include "dotdot_mqtt_mock.h"
#include "dotdot_mqtt_generated_commands_mock.h"
#include "zpc_attribute_store_network_helper_mock.h"

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp()
{
  datastore_init(":memory:");
  attribute_store_init();
  zpc_attribute_store_register_known_attribute_types();
}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  attribute_store_teardown();
  datastore_teardown();
  return num_failures;
}

// Tested command class handler
const zwave_struct_handler_args command_class_handler
  = {.command_class_id  = COMMAND_CLASS_SCHEDULE_ENTRY_LOCK,
     .supported_version = SCHEDULE_ENTRY_LOCK_VERSION_V3};
// Get Set function map
const resolver_function_map attribute_bindings
  = {{ATTRIBUTE(SLOTS_WEEK_DAY), {SCHEDULE_ENTRY_TYPE_SUPPORTED_GET, 0}},
     {ATTRIBUTE(HOUR_TZO),
      {SCHEDULE_ENTRY_LOCK_TIME_OFFSET_GET_V2,
       SCHEDULE_ENTRY_LOCK_TIME_OFFSET_SET_V2}},
     {ATTRIBUTE(ENABLED), {0, SCHEDULE_ENTRY_LOCK_ENABLE_SET}},
     {ATTRIBUTE(ENABLE_ALL), {0, SCHEDULE_ENTRY_LOCK_ENABLE_ALL_SET}},
     {ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION),
      {SCHEDULE_ENTRY_LOCK_WEEK_DAY_GET, SCHEDULE_ENTRY_LOCK_WEEK_DAY_SET}},
     {ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION),
      {SCHEDULE_ENTRY_LOCK_YEAR_DAY_GET, SCHEDULE_ENTRY_LOCK_YEAR_DAY_SET}},
     {ATTRIBUTE(DAILY_REPEATING_SET_ACTION),
      {SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_GET_V3,
       SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_SET_V3}}};

/// Called before each and every test
void setUp()
{
  zwave_setUp(command_class_handler,
              &zwave_command_class_schedule_entry_lock_init,
              attribute_bindings);
}

void test_switch_all_interview_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  // Verify that we have the correct node(s)
  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  helper_test_node_exists(ATTRIBUTE(SLOTS_WEEK_DAY));
  helper_test_node_exists(ATTRIBUTE(HOUR_TZO));
  helper_test_node_exists(ATTRIBUTE(ENABLED), user_id_node);
  helper_test_node_exists(ATTRIBUTE(ENABLE_ALL));
}

void test_schedule_entry_lock_get_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto slot_week_day_node = helper_test_and_get_node(ATTRIBUTE(SLOTS_WEEK_DAY));

  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_TYPE_SUPPORTED_GET,
                                       slot_week_day_node);
}

void test_schedule_entry_lock_time_offset_get_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto hour_tzo_node = helper_test_and_get_node(ATTRIBUTE(HOUR_TZO));

  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_TIME_OFFSET_GET_V2,
                                       hour_tzo_node);
}

void test_schedule_entry_lock_week_day_get_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID));
  auto set_action_node
    = slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION));

  uint8_t user_id = 0x01;
  uint8_t slot_id = 0x02;

  zwave_frame week_day_get;
  week_day_get.add(user_id);
  week_day_get.add(slot_id);

  user_id_node.set_reported(user_id);
  slot_id_node.set_reported(slot_id);

  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_WEEK_DAY_GET,
                                       set_action_node,
                                       week_day_get);
}

void test_schedule_entry_lock_year_day_get_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID));
  auto set_action_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION));

  uint8_t user_id = 0x01;
  uint8_t slot_id = 0x02;

  zwave_frame year_day_get;
  year_day_get.add(user_id);
  year_day_get.add(slot_id);

  user_id_node.set_reported(user_id);
  slot_id_node.set_reported(slot_id);

  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_YEAR_DAY_GET,
                                       set_action_node,
                                       year_day_get);
}

void test_schedule_entry_lock_daily_repeating_get_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID));
  auto set_action_node
    = slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SET_ACTION));

  uint8_t user_id = 0x01;
  uint8_t slot_id = 0x02;

  zwave_frame daily_repeating_get;
  daily_repeating_get.add(user_id);
  daily_repeating_get.add(slot_id);

  user_id_node.set_reported(user_id);
  slot_id_node.set_reported(slot_id);

  helper_test_get_set_frame_happy_case(
    SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_GET_V3,
    set_action_node,
    daily_repeating_get);
}

void test_schedule_entry_lock_enable_set_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto enable_node  = user_id_node.emplace_node(ATTRIBUTE(ENABLED));
  uint8_t enable    = 0x01;

  zwave_frame enable_set;
  enable_set.add(enable);
  enable_set.add(SCHEDULE_ENTRY_LOCK_ENABLE);

  // Test with desired value
  user_id_node.set_reported(enable);
  enable_node.set_desired(SCHEDULE_ENTRY_LOCK_ENABLE);

  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_ENABLE_SET,
                                       enable_node,
                                       enable_set);
}

void test_schedule_entry_lock_enable_all_set_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto enable_all_node = helper_test_and_get_node(ATTRIBUTE(ENABLE_ALL));

  // Test with desired value
  enable_all_node.set_desired(SCHEDULE_ENTRY_LOCK_ENABLE);
  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_ENABLE_ALL_SET,
                                       enable_all_node,
                                       {SCHEDULE_ENTRY_LOCK_ENABLE});
}

void test_schedule_entry_lock_week_day_set_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID));
  auto set_action_node
    = slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION));

  uint8_t set_action   = 0x01;
  uint8_t user_id      = 0x02;
  uint8_t slot_id      = 0x03;
  uint8_t day_of_week  = 0x04;
  uint8_t start_hour   = 0x05;
  uint8_t start_minute = 0x06;
  uint8_t stop_hour    = 0x07;
  uint8_t stop_minute  = 0x08;

  zwave_frame week_day_set;
  week_day_set.add(set_action);
  week_day_set.add(user_id);
  week_day_set.add(slot_id);
  week_day_set.add(day_of_week);
  week_day_set.add(start_hour);
  week_day_set.add(start_minute);
  week_day_set.add(stop_hour);
  week_day_set.add(stop_minute);

  set_action_node.set_reported(set_action);
  user_id_node.set_reported(user_id);
  slot_id_node.set_reported(slot_id);
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_DAY_OF_WEEK))
    .set_reported(day_of_week);
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_START_HOUR))
    .set_reported(start_hour);
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_START_MINUTE))
    .set_reported(start_minute);
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_HOUR))
    .set_reported(stop_hour);
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_MINUTE))
    .set_reported(stop_minute);

  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_WEEK_DAY_SET,
                                       set_action_node,
                                       week_day_set);
}

void test_schedule_entry_lock_year_day_set_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID));
  auto set_action_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION));

  uint8_t set_action   = 0x01;
  uint8_t user_id      = 0x02;
  uint8_t slot_id      = 0x03;
  uint8_t start_year   = 0x04;
  uint8_t start_month  = 0x05;
  uint8_t start_day    = 0x06;
  uint8_t start_hour   = 0x07;
  uint8_t start_minute = 0x08;
  uint8_t stop_year    = 0x09;
  uint8_t stop_month   = 0x0a;
  uint8_t stop_day     = 0x0b;
  uint8_t stop_hour    = 0x0c;
  uint8_t stop_minute  = 0x0d;

  zwave_frame year_day_set;
  year_day_set.add(set_action);
  year_day_set.add(user_id);
  year_day_set.add(slot_id);
  year_day_set.add(start_year);
  year_day_set.add(start_month);
  year_day_set.add(start_day);
  year_day_set.add(start_hour);
  year_day_set.add(start_minute);
  year_day_set.add(stop_year);
  year_day_set.add(stop_month);
  year_day_set.add(stop_day);
  year_day_set.add(stop_hour);
  year_day_set.add(stop_minute);

  set_action_node.set_reported(set_action);
  user_id_node.set_reported(user_id);
  slot_id_node.set_reported(slot_id);

  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_YEAR))
    .set_reported(start_year);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MONTH))
    .set_reported(start_month);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_DAY))
    .set_reported(start_day);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_HOUR))
    .set_reported(start_hour);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MINUTE))
    .set_reported(start_minute);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_YEAR))
    .set_reported(stop_year);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MONTH))
    .set_reported(stop_month);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_DAY))
    .set_reported(stop_day);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_HOUR))
    .set_reported(stop_hour);
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MINUTE))
    .set_reported(stop_minute);

  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_YEAR_DAY_SET,
                                       set_action_node,
                                       year_day_set);
}

void test_schedule_entry_lock_daily_repeating_set_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID));
  auto set_action_node
    = slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SET_ACTION));

  uint8_t set_action      = 0x01;
  uint8_t user_id         = 0x02;
  uint8_t slot_id         = 0x03;
  uint8_t week_day        = 0x04;
  uint8_t start_hour      = 0x05;
  uint8_t start_minute    = 0x06;
  uint8_t duration_hour   = 0x07;
  uint8_t duration_minute = 0x08;

  zwave_frame daily_repeating_set;
  daily_repeating_set.add(set_action);
  daily_repeating_set.add(user_id);
  daily_repeating_set.add(slot_id);
  daily_repeating_set.add(week_day);
  daily_repeating_set.add(start_hour);
  daily_repeating_set.add(start_minute);
  daily_repeating_set.add(duration_hour);
  daily_repeating_set.add(duration_minute);

  set_action_node.set_reported(set_action);
  user_id_node.set_reported(user_id);
  slot_id_node.set_reported(slot_id);

  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_WEEK_DAY))
    .set_reported(week_day);
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_START_HOUR))
    .set_reported(start_hour);
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_START_MINUTE))
    .set_reported(start_minute);
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_DURATION_HOUR))
    .set_reported(duration_hour);
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_DURATION_MINUTE))
    .set_reported(duration_minute);

  helper_test_get_set_frame_happy_case(
    SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_SET_V3,
    set_action_node,
    daily_repeating_set);
}

void test_schedule_entry_lock_time_offset_set_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto hour_tzo_node = helper_test_and_get_node(ATTRIBUTE(HOUR_TZO));
  auto endpoint_node = hour_tzo_node.parent();

  // Set reported values for TZO and DST fields as per Z-Wave specs
  hour_tzo_node.set_reported<uint8_t>(15);  // Hour TZO (7 bits)

  endpoint_node.emplace_node(ATTRIBUTE(SIGN_TZO))
    .set_reported<uint8_t>(1);  // Sign TZO (1 bit, 0 for positive)
  endpoint_node.emplace_node(ATTRIBUTE(MINUTE_TZO))
    .set_reported<uint8_t>(0x00);  // Minute TZO (7 bits)

  endpoint_node.emplace_node(ATTRIBUTE(DST_OFFSET_SIGN))
    .set_reported<uint8_t>(1);  // DST Offset Sign (1 bit, 0 for positive)
  endpoint_node.emplace_node(ATTRIBUTE(DST_OFFSET_MINUTE))
    .set_reported<uint8_t>(15);  // DST Offset Minute (7 bits)

  // Byte 1: Sign TZO (1 bit) + Hour TZO (7 bits)
  // Byte 2: Minute TZO (7 bits)
  // Byte 3: Sign Offset DST (1 bit) + Minute Offset DST (7 bits)
  helper_test_get_set_frame_happy_case(SCHEDULE_ENTRY_LOCK_TIME_OFFSET_SET_V2,
                                       hour_tzo_node,
                                       {0b10001111, 0x00, 0b10001111});
}

void test_schedule_entry_lock_type_supported_report_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);
  auto slot_week_day_node = helper_test_and_get_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  auto endpoint_node      = slot_week_day_node.parent();
  auto slot_year_day_node
    = endpoint_node.emplace_node(ATTRIBUTE(SLOTS_YEAR_DAY));
  auto slot_daily_repeating_node
    = endpoint_node.emplace_node(ATTRIBUTE(NUMBER_OF_SLOTS_DAILY_REPEATING));
  uint8_t slot_week_day        = 0x01;
  uint8_t slot_year_day        = 0x02;
  uint8_t slot_daily_repeating = 0x03;

  zwave_frame type_supported_report;
  type_supported_report.add(slot_week_day);
  type_supported_report.add(slot_year_day);
  type_supported_report.add(slot_daily_repeating);

  attribute_store_network_helper_get_endpoint_node_IgnoreAndReturn(
    cpp_endpoint_id_node);
  helper_test_report_frame(SCHEDULE_ENTRY_TYPE_SUPPORTED_REPORT,
                           type_supported_report);
  TEST_ASSERT_EQUAL(slot_week_day, slot_week_day_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(slot_year_day, slot_year_day_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(slot_daily_repeating,
                    slot_daily_repeating_node.reported<uint8_t>());
}

void test_schedule_entry_lock_time_offset_report_report_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);

  auto hour_tzo_node   = helper_test_and_get_node(ATTRIBUTE(HOUR_TZO));
  auto endpoint_node   = hour_tzo_node.parent();
  auto sign_tzo_node   = endpoint_node.emplace_node(ATTRIBUTE(SIGN_TZO));
  auto minute_tzo_node = endpoint_node.emplace_node(ATTRIBUTE(MINUTE_TZO));
  auto dst_offset_sign_node
    = endpoint_node.emplace_node(ATTRIBUTE(DST_OFFSET_SIGN));
  auto dst_offset_minute_node
    = endpoint_node.emplace_node(ATTRIBUTE(DST_OFFSET_MINUTE));

  attribute_store_network_helper_get_endpoint_node_IgnoreAndReturn(
    cpp_endpoint_id_node);

  helper_test_report_frame(SCHEDULE_ENTRY_LOCK_TIME_OFFSET_REPORT_V2,
                           {0b10001111, 0x00, 0b10001111});
  TEST_ASSERT_EQUAL(1, sign_tzo_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(15, hour_tzo_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(0x00, minute_tzo_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(1, dst_offset_sign_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(15, dst_offset_minute_node.reported<uint8_t>());
}

void test_schedule_entry_lock_week_day_report_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);
  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID));
  auto day_of_week_node
    = slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_DAY_OF_WEEK));
  auto start_hour_node
    = slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_START_HOUR));
  auto start_minute_node
    = slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_START_MINUTE));
  auto stop_hour_node
    = slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_HOUR));
  auto stop_minute_node
    = slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_MINUTE));

  uint8_t user_id      = 0x01;
  uint8_t slot_id      = 0x02;
  uint8_t day_of_week  = 0x03;
  uint8_t start_hour   = 0x04;
  uint8_t start_minute = 0x05;
  uint8_t stop_hour    = 0x06;
  uint8_t stop_minute  = 0x07;

  zwave_frame week_day_set;
  week_day_set.add(user_id);
  week_day_set.add(slot_id);
  week_day_set.add(day_of_week);
  week_day_set.add(start_hour);
  week_day_set.add(start_minute);
  week_day_set.add(stop_hour);
  week_day_set.add(stop_minute);

  attribute_store_network_helper_get_endpoint_node_IgnoreAndReturn(
    cpp_endpoint_id_node);
  attribute_store_network_helper_get_unid_from_node_IgnoreAndReturn(
    cpp_endpoint_id_node);
  uic_mqtt_dotdot_unify_schedule_entry_lock_publish_generated_week_day_report_command_Ignore();

  helper_test_report_frame(SCHEDULE_ENTRY_LOCK_WEEK_DAY_REPORT, week_day_set);
  TEST_ASSERT_EQUAL(user_id, user_id_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(slot_id, slot_id_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(day_of_week, day_of_week_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_hour, start_hour_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_minute, start_minute_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(stop_hour, stop_hour_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(stop_minute, stop_minute_node.reported<uint8_t>());
}

void test_schedule_entry_lock_year_day_report_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);
  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID));
  auto start_year_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_YEAR));
  auto start_month_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MONTH));
  auto start_day_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_DAY));
  auto start_hour_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_HOUR));
  auto start_minute_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MINUTE));
  auto stop_year_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_YEAR));
  auto stop_month_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MONTH));
  auto stop_day_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_DAY));
  auto stop_hour_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_HOUR));
  auto stop_minute_node
    = slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MINUTE));

  uint8_t user_id      = 0x01;
  uint8_t slot_id      = 0x02;
  uint8_t start_year   = 0x03;
  uint8_t start_month  = 0x04;
  uint8_t start_day    = 0x05;
  uint8_t start_hour   = 0x06;
  uint8_t start_minute = 0x07;
  uint8_t stop_year    = 0x08;
  uint8_t stop_month   = 0x09;
  uint8_t stop_day     = 0x0a;
  uint8_t stop_hour    = 0x0b;
  uint8_t stop_minute  = 0x0c;

  zwave_frame year_day_set;
  year_day_set.add(user_id);
  year_day_set.add(slot_id);
  year_day_set.add(start_year);
  year_day_set.add(start_month);
  year_day_set.add(start_day);
  year_day_set.add(start_hour);
  year_day_set.add(start_minute);
  year_day_set.add(stop_year);
  year_day_set.add(stop_month);
  year_day_set.add(stop_day);
  year_day_set.add(stop_hour);
  year_day_set.add(stop_minute);

  attribute_store_network_helper_get_endpoint_node_IgnoreAndReturn(
    cpp_endpoint_id_node);
  attribute_store_network_helper_get_unid_from_node_IgnoreAndReturn(
    cpp_endpoint_id_node);
  uic_mqtt_dotdot_unify_schedule_entry_lock_publish_generated_year_day_report_command_Ignore();
  helper_test_report_frame(SCHEDULE_ENTRY_LOCK_YEAR_DAY_REPORT, year_day_set);
  TEST_ASSERT_EQUAL(user_id, user_id_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(slot_id, slot_id_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_year, start_year_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_month, start_month_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_day, start_day_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_hour, start_hour_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_minute, start_minute_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(stop_year, stop_year_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(stop_month, stop_month_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(stop_day, stop_day_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(stop_hour, stop_hour_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(stop_minute, stop_minute_node.reported<uint8_t>());
}

void test_schedule_entry_lock_daily_repeating_report_happy_case()
{
  helper_set_version(SCHEDULE_ENTRY_LOCK_VERSION_V3);
  auto user_id_node = helper_test_and_get_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node
    = user_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID));
  auto week_day_node
    = slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_WEEK_DAY));
  auto start_hour_node
    = slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_START_HOUR));
  auto start_minute_node
    = slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_START_MINUTE));
  auto duration_hour_node
    = slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_DURATION_HOUR));
  auto duration_minute_node
    = slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_DURATION_MINUTE));

  uint8_t user_id         = 0x01;
  uint8_t slot_id         = 0x02;
  uint8_t week_day        = 0x03;
  uint8_t start_hour      = 0x04;
  uint8_t start_minute    = 0x05;
  uint8_t duration_hour   = 0x06;
  uint8_t duration_minute = 0x07;

  zwave_frame daily_repeating_set;
  daily_repeating_set.add(user_id);
  daily_repeating_set.add(slot_id);
  daily_repeating_set.add(week_day);
  daily_repeating_set.add(start_hour);
  daily_repeating_set.add(start_minute);
  daily_repeating_set.add(duration_hour);
  daily_repeating_set.add(duration_minute);

  attribute_store_network_helper_get_endpoint_node_IgnoreAndReturn(
    cpp_endpoint_id_node);
  attribute_store_network_helper_get_unid_from_node_IgnoreAndReturn(
    cpp_endpoint_id_node);
  uic_mqtt_dotdot_unify_schedule_entry_lock_publish_generated_daily_repeating_report_command_Ignore();

  helper_test_report_frame(SCHEDULE_ENTRY_LOCK_DAILY_REPEATING_REPORT_V3,
                           daily_repeating_set);
  TEST_ASSERT_EQUAL(user_id, user_id_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(slot_id, slot_id_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(week_day, week_day_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_hour, start_hour_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(start_minute, start_minute_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(duration_hour, duration_hour_node.reported<uint8_t>());
  TEST_ASSERT_EQUAL(duration_minute, duration_minute_node.reported<uint8_t>());
}

}  // extern "C"
