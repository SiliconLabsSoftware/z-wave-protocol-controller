/******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "schedule_entry_lock_server.h"

#include "unity.h"

// Unify components
#include "datastore.h"
#include "attribute_store_fixt.h"
#include "attribute_store_helper.h"
#include "unify_dotdot_defined_attribute_types.h"
#include "zpc_attribute_store_network_helper.h"

// ZPC Components
#include "zwave_unid.h"
#include "zwave_command_class_schedule_entry_lock.h"
#include "zwave_command_class_test_helper.hpp"

// Test helpers
#include "zpc_attribute_store_test_helper.h"

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK_##type
using namespace zwave_command_class_test_helper;

extern "C" {

// Mock helper
#include "dotdot_mqtt_mock.h"
#include "dotdot_mqtt_generated_commands_mock.h"
#include "zpc_attribute_store_network_helper_mock.h"
#include "attribute_store_defined_attribute_types.h"
#include "unify_dotdot_attribute_store.h"

// private variables

static uic_mqtt_dotdot_unify_schedule_entry_lock_enable_set_callback_t enable_set = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_enable_all_set_callback_t enable_all_set = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_set_callback_t week_day_set = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_get_callback_t week_day_get = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_set_callback_t year_day_set = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_get_callback_t year_day_get = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_set_callback_t daily_repeating_set = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_get_callback_t daily_repeating_get = NULL;
static uic_mqtt_dotdot_unify_schedule_entry_lock_write_attributes_callback_t write_attributes_callback = NULL;

void uic_mqtt_dotdot_unify_schedule_entry_lock_enable_set_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_enable_set_callback_t callback,int cmock_num_calls){
  enable_set = callback;
}

void uic_mqtt_dotdot_unify_schedule_entry_lock_enable_all_set_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_enable_all_set_callback_t callback,int cmock_num_calls){
  enable_all_set = callback;
}

void uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_set_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_set_callback_t callback,int cmock_num_calls)
{
  week_day_set = callback;
}

void uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_get_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_get_callback_t callback,int cmock_num_calls)
{
  week_day_get = callback;
}

void uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_set_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_set_callback_t callback,int cmock_num_calls)
{
  year_day_set = callback;
}

void uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_get_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_get_callback_t callback,int cmock_num_calls)
{
  year_day_get = callback;
}

void uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_set_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_set_callback_t callback,int cmock_num_calls){
  daily_repeating_set = callback;
}

void uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_get_callback_set_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_get_callback_t callback,int cmock_num_calls){
  daily_repeating_get = callback;
}

void uic_mqtt_dotdot_set_unify_schedule_entry_lock_write_attributes_callback_stub(const uic_mqtt_dotdot_unify_schedule_entry_lock_write_attributes_callback_t callback, int cmock_num_calls){
  write_attributes_callback = callback;
}


/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp()
{
  datastore_init(":memory:");
  attribute_store_init();
}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  attribute_store_teardown();
  datastore_teardown();
  return num_failures;
}

/// Called before each and every test
void setUp()
{
  zpc_attribute_store_test_helper_create_network();

  uic_mqtt_dotdot_unify_schedule_entry_lock_enable_set_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_enable_set_callback_set_stub);

  uic_mqtt_dotdot_unify_schedule_entry_lock_enable_all_set_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_enable_all_set_callback_set_stub);

  uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_set_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_set_callback_set_stub);

  uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_get_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_week_day_get_callback_set_stub);

  uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_set_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_set_callback_set_stub);

  uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_get_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_year_day_get_callback_set_stub);

  uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_set_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_set_callback_set_stub);

  uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_get_callback_set_Stub(
    &uic_mqtt_dotdot_unify_schedule_entry_lock_daily_repeating_get_callback_set_stub);

  uic_mqtt_dotdot_set_unify_schedule_entry_lock_write_attributes_callback_Stub(
    &uic_mqtt_dotdot_set_unify_schedule_entry_lock_write_attributes_callback_stub);

  // Call init
  TEST_ASSERT_EQUAL(SL_STATUS_OK, schedule_entry_lock_cluster_server_init());
}

/// Called after each and every test
void tearDown()
{
  attribute_store_delete_node(attribute_store_get_root());
}

void test_schedule_entry_lock_enable_set()
{
  TEST_ASSERT_NOT_NULL(enable_set);
  uint8_t user_id = 3;
  uint8_t enabled = 1;

  attribute_store::attribute ep_node_id(endpoint_id_node);

  auto user_id_node = ep_node_id.emplace_node(ATTRIBUTE(USER_IDENTIFIER));
  user_id_node.emplace_node(ATTRIBUTE(ENABLED));
  user_id_node.set_reported(user_id);
  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));

  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    enable_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         user_id,
                         enabled));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    enable_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         user_id,
                         enabled));
}

void test_schedule_entry_lock_enable_all_set(){
  TEST_ASSERT_NOT_NULL(enable_all_set);
  uint8_t enabled = 1;
  attribute_store::attribute ep_node_id(endpoint_id_node);
  ep_node_id.emplace_node(ATTRIBUTE(ENABLE_ALL));
  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));


  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    enable_all_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         enabled));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    enable_all_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         enabled));

}

void test_schedule_entry_lock_week_day_set(){
  TEST_ASSERT_NOT_NULL(week_day_set);
  uint8_t set_action = 1;
  uint8_t user_identifier = 1;
  uint8_t schedule_slotid = 1;
  uint8_t day_of_week = 2;
  uint8_t start_hour = 2;
  uint8_t start_minute = 3;
  uint8_t stop_hour = 3;
  uint8_t stop_minute = 4;

  attribute_store::attribute ep_node_id(endpoint_id_node);
  auto user_id_node = ep_node_id.emplace_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node = user_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID));
  user_id_node.set_reported(user_identifier);
  slot_id_node.set_reported(schedule_slotid);
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION));
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_DAY_OF_WEEK));
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_START_HOUR));
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_START_MINUTE));
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_HOUR));
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_STOP_MINUTE));

  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));

  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    week_day_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         set_action,user_identifier,schedule_slotid,day_of_week,start_hour,start_minute,stop_hour,stop_minute));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    week_day_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         set_action,user_identifier,schedule_slotid,day_of_week,start_hour,start_minute,stop_hour,stop_minute));
}

void test_schedule_entry_lock_week_day_get(){
  TEST_ASSERT_NOT_NULL(week_day_get);
  uint8_t user_identifier = 1;
  uint8_t schedule_slotid = 1;

  attribute_store::attribute ep_node_id(endpoint_id_node);
  auto user_id_node = ep_node_id.emplace_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node = user_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SLOT_ID));
  user_id_node.set_reported(user_identifier);
  slot_id_node.set_reported(schedule_slotid);
  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));
  slot_id_node.emplace_node(ATTRIBUTE(WEEK_DAY_SCHEDULE_SET_ACTION));

  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    week_day_get(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         user_identifier,schedule_slotid));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    week_day_get(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         user_identifier,schedule_slotid));
}

void test_schedule_entry_lock_year_day_set(){
  TEST_ASSERT_NOT_NULL(year_day_set);
  uint8_t set_action = 1;
  uint8_t user_identifier = 1;
  uint8_t schedule_slotid = 1;
  uint8_t start_year = 2;
  uint8_t start_day = 2;
  uint8_t start_month = 2;
  uint8_t start_hour = 2;
  uint8_t start_minute = 3;
  uint8_t stop_year = 3;
  uint8_t stop_month = 3;
  uint8_t stop_hour = 4;
  uint8_t stop_day = 4;
  uint8_t stop_minute = 4;

  attribute_store::attribute ep_node_id(endpoint_id_node);
  auto user_id_node = ep_node_id.emplace_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node = user_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID));
  user_id_node.set_reported(user_identifier);
  slot_id_node.set_reported(schedule_slotid);
  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_YEAR));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MONTH));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_DAY));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_HOUR));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_START_MINUTE));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_YEAR));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MONTH));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_DAY));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_HOUR));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_STOP_MINUTE));

  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    year_day_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         set_action,user_identifier,schedule_slotid,start_year,start_day,start_hour,start_month,start_minute,stop_year,stop_month,stop_day,stop_hour,stop_minute));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    year_day_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         set_action,user_identifier,schedule_slotid,start_year,start_day,start_hour,start_month,start_minute,stop_year,stop_month,stop_day,stop_hour,stop_minute));

}

void test_schedule_entry_lock_year_day_get(){
  TEST_ASSERT_NOT_NULL(year_day_get);
  uint8_t user_identifier = 1;
  uint8_t schedule_slotid = 1;

  attribute_store::attribute ep_node_id(endpoint_id_node);
  auto user_id_node = ep_node_id.emplace_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node = user_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SLOT_ID));
  user_id_node.set_reported(user_identifier);
  slot_id_node.set_reported(schedule_slotid);
  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));
  slot_id_node.emplace_node(ATTRIBUTE(YEAR_DAY_SCHEDULE_SET_ACTION));

  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    year_day_get(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         user_identifier,schedule_slotid));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    year_day_get(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         user_identifier,schedule_slotid));

}


void test_schedule_entry_lock_daily_repeating_set(){
  TEST_ASSERT_NOT_NULL(daily_repeating_set);
  uint8_t set_action = 1;
  uint8_t user_identifier = 1;
  uint8_t schedule_slotid = 1;
  uint8_t week_day_bitmask = 2;
  uint8_t start_hour = 2;
  uint8_t start_minute = 3;
  uint8_t duration_hour = 3;
  uint8_t duration_minute = 4;

  attribute_store::attribute ep_node_id(endpoint_id_node);
  auto user_id_node = ep_node_id.emplace_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node = user_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID));
  user_id_node.set_reported(user_identifier);
  slot_id_node.set_reported(schedule_slotid);
  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SET_ACTION));
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_WEEK_DAY));
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_START_HOUR));
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_START_MINUTE));
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_DURATION_HOUR));
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_DURATION_MINUTE));

  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    daily_repeating_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         set_action,user_identifier,schedule_slotid,week_day_bitmask,start_hour,start_minute,duration_hour,duration_minute));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    daily_repeating_set(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         set_action,user_identifier,schedule_slotid,week_day_bitmask,start_hour,start_minute,duration_hour,duration_minute));
}

void test_schedule_entry_lock_daily_repeating_get(){
  TEST_ASSERT_NOT_NULL(daily_repeating_get);
  uint8_t user_identifier = 1;
  uint8_t schedule_slotid = 1;

  attribute_store::attribute ep_node_id(endpoint_id_node);
  auto user_id_node = ep_node_id.emplace_node(ATTRIBUTE(USER_IDENTIFIER));
  auto slot_id_node = user_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SCHEDULE_SLOT_ID));
  user_id_node.set_reported(user_identifier);
  slot_id_node.set_reported(schedule_slotid);
  ep_node_id.emplace_node(ATTRIBUTE(SLOTS_WEEK_DAY));
  ep_node_id.emplace_node(ATTRIBUTE(HOUR_TZO));
  slot_id_node.emplace_node(ATTRIBUTE(DAILY_REPEATING_SET_ACTION));

  attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

  // Support check
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    daily_repeating_get(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_SUPPORT_CHECK,
                         user_identifier,schedule_slotid));

  // Test callback
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    daily_repeating_get(supporting_node_unid,
                         endpoint_id,
                         UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                         user_identifier,schedule_slotid));
}

void test_schedule_entry_lock_write_attributes_callback()
{
    TEST_ASSERT_NOT_NULL(write_attributes_callback);

    attribute_store_network_helper_get_node_id_node_IgnoreAndReturn(node_id_node);

    // Define the lock state with sample values
    uic_mqtt_dotdot_unify_schedule_entry_lock_state_t lock_state = {};
    lock_state.slots_week_day = 1;
    lock_state.slots_year_day = 2;
    lock_state.signtzo = 3;
    lock_state.hourtzo = 4;
    lock_state.minutetzo = 5;
    lock_state.dst_offset_sign = 6;
    lock_state.dst_offset_minute = 7;
    lock_state.number_of_slots_daily_repeating = 8;

    // Define the updated state to indicate which attributes are updated
    uic_mqtt_dotdot_unify_schedule_entry_lock_updated_state_t updated_lock_state = {};
    updated_lock_state.slots_week_day = true;
    updated_lock_state.slots_year_day = true;
    updated_lock_state.signtzo = true;
    updated_lock_state.hourtzo = true;
    updated_lock_state.minutetzo = true;
    updated_lock_state.dst_offset_sign = true;
    updated_lock_state.dst_offset_minute = true;
    updated_lock_state.number_of_slots_daily_repeating = true;

    // Mock attribute nodes for testing each field
    attribute_store::attribute ep_node_id(endpoint_id_node);
    auto slots_week_day_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_SLOTS_WEEK_DAY);
    auto slots_year_day_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_SLOTS_YEAR_DAY);
    auto signtzo_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_SIGNTZO);
    auto hourtzo_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_HOURTZO);
    auto minutetzo_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_MINUTETZO);
    auto dst_offset_sign_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_DST_OFFSET_SIGN);
    auto dst_offset_minute_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_DST_OFFSET_MINUTE);
    auto number_of_slots_daily_repeating_node = ep_node_id.emplace_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SCHEDULE_ENTRY_LOCK_NUMBER_OF_SLOTS_DAILY_REPEATING);

    uic_mqtt_dotdot_set_unify_schedule_entry_lock_write_attributes_callback_Ignore();
    // Run the callback
    write_attributes_callback(
        supporting_node_unid,
        endpoint_id,
        UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
        lock_state,
        updated_lock_state
    );

    // Test the desired values set by the callback
    TEST_ASSERT_EQUAL_UINT8(lock_state.slots_week_day, slots_week_day_node.desired<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(lock_state.slots_year_day, slots_year_day_node.desired<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(lock_state.signtzo, signtzo_node.desired<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(lock_state.hourtzo, hourtzo_node.desired<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(lock_state.minutetzo, minutetzo_node.desired<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(lock_state.dst_offset_sign, dst_offset_sign_node.desired<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(lock_state.dst_offset_minute, dst_offset_minute_node.desired<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(lock_state.number_of_slots_daily_repeating, number_of_slots_daily_repeating_node.desired<uint8_t>());
}

} // extern "C"
