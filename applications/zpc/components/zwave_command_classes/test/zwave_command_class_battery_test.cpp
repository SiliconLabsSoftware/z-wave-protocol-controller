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
#include "zwave_command_class_battery.h"
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

// Test helpers
#include "zwave_command_class_test_helper.hpp"

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_BATTERY_##type

using namespace zwave_command_class_test_helper;

extern "C" {

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
  = {.command_class_id  = COMMAND_CLASS_BATTERY,
     .supported_version = BATTERY_VERSION_V3};
// Get Set function map
const resolver_function_map attribute_bindings
  = {{ATTRIBUTE(BATTERY_LEVEL), {BATTERY_GET, 0}},
     {ATTRIBUTE(HEALTH_MAXIMUM_CAPACITY), {BATTERY_HEALTH_GET_V2, 0}}};

/// Called before each and every test
void setUp()
{
  zwave_setUp(command_class_handler,
              &zwave_command_class_battery_init,
              attribute_bindings);
}

///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

void helper_test_health_report_attributes(uint8_t expected_max_capacity,
                                          uint8_t expected_scale,
                                          uint8_t expected_precision,
                                          int32_t expected_temperature)
{
  helper_test_attribute_value(ATTRIBUTE(HEALTH_MAXIMUM_CAPACITY),
                              expected_max_capacity);
  helper_test_attribute_value(ATTRIBUTE(HEALTH_SCALE), expected_scale);
  helper_test_attribute_value(ATTRIBUTE(HEALTH_PRECISION), expected_precision);
  helper_test_attribute_value(ATTRIBUTE(HEALTH_BATTERY_TEMPERATURE),
                              expected_temperature);
}

///////////////////////////////////////////////////////////////////////////////
// Test cases
///////////////////////////////////////////////////////////////////////////////

void test_battery_interview_v1_happy_case()
{
  helper_set_version(1);

  helper_test_node_exists(ATTRIBUTE(BATTERY_LEVEL));
}

void test_battery_interview_v2_happy_case()
{
  helper_set_version(2);

  std::vector<attribute_store_type_t> tested_attributes
    = {ATTRIBUTE(BATTERY_LEVEL),
       // Added in v2
       ATTRIBUTE(CHARGING_STATUS),
       ATTRIBUTE(RECHARGEABLE),
       ATTRIBUTE(BACKUP_BATTERY),
       ATTRIBUTE(OVERHEATING),
       ATTRIBUTE(LOW_FLUID),
       ATTRIBUTE(REPLACE_RECHARGE),
       ATTRIBUTE(DISCONNECTED),
       ATTRIBUTE(HEALTH_MAXIMUM_CAPACITY),
       ATTRIBUTE(HEALTH_SCALE),
       ATTRIBUTE(HEALTH_PRECISION),
       ATTRIBUTE(HEALTH_BATTERY_TEMPERATURE)};

  for (const auto &attribute: tested_attributes) {
    helper_test_node_exists(attribute);
  }
}

void test_battery_interview_v3_happy_case()
{
  helper_set_version(3);

  std::vector<attribute_store_type_t> tested_attributes
    = {ATTRIBUTE(BATTERY_LEVEL),
       // Added in v2
       ATTRIBUTE(CHARGING_STATUS),
       ATTRIBUTE(RECHARGEABLE),
       ATTRIBUTE(BACKUP_BATTERY),
       ATTRIBUTE(OVERHEATING),
       ATTRIBUTE(LOW_FLUID),
       ATTRIBUTE(REPLACE_RECHARGE),
       ATTRIBUTE(DISCONNECTED),
       ATTRIBUTE(HEALTH_MAXIMUM_CAPACITY),
       ATTRIBUTE(HEALTH_SCALE),
       ATTRIBUTE(HEALTH_PRECISION),
       ATTRIBUTE(HEALTH_BATTERY_TEMPERATURE),
       // Added in v3
       ATTRIBUTE(LOW_TEMPERATURE)};

  for (const auto &attribute: tested_attributes) {
    helper_test_node_exists(attribute);
  }
}

void test_battery_health_report_temperature_size_1_happy_case()
{
  helper_set_version(2);

  uint8_t expected_battery_capacity = 0x12;
  int8_t expected_temperature       = 0xFF;

  helper_test_report_frame(
    BATTERY_HEALTH_REPORT_V2,
    {expected_battery_capacity, 0b01001001, static_cast<uint8_t>(0xFF)});

  helper_test_health_report_attributes(
    expected_battery_capacity,
    1,
    2,
    static_cast<int32_t>(expected_temperature));
}

void test_battery_health_report_temperature_size_2_happy_case()
{
  helper_set_version(3);
  uint8_t expected_battery_capacity = 0xFF;

  helper_test_report_frame(BATTERY_HEALTH_REPORT_V2,
                           {expected_battery_capacity, 0b00100010, 0x01, 0x2C});

  helper_test_health_report_attributes(expected_battery_capacity, 0, 1, 300);
}

void test_battery_health_report_temperature_size_4_happy_case()
{
  helper_set_version(3);
  uint8_t expected_battery_capacity = 0x60;

  helper_test_report_frame(
    BATTERY_HEALTH_REPORT_V2,
    {expected_battery_capacity, 0b10000100, 0xFF, 0xFE, 0x2B, 0x40});

  helper_test_health_report_attributes(expected_battery_capacity,
                                       0,
                                       4,
                                       -120000);
}

void helper_battery_report(uint8_t expected_battery_level,
                           uint8_t expected_charging_status  = 0,
                           uint8_t rechargeable              = 0,
                           uint8_t expected_backup_battery   = 0,
                           uint8_t expected_overheating      = 0,
                           uint8_t expected_low_fluid        = 0,
                           uint8_t expected_replace_recharge = 0,
                           uint8_t expected_disconnected     = 0,
                           uint8_t expected_low_temperature  = 0)
{
  zwave_cc_version_t version = helper_get_version();

  helper_test_attribute_value(ATTRIBUTE(BATTERY_LEVEL), expected_battery_level);

  std::map<attribute_store_type_t, uint8_t> expected_values;
  if (version >= 2) {
    expected_values = {{ATTRIBUTE(CHARGING_STATUS), expected_charging_status},
                       {ATTRIBUTE(RECHARGEABLE), rechargeable},
                       {ATTRIBUTE(BACKUP_BATTERY), expected_backup_battery},
                       {ATTRIBUTE(OVERHEATING), expected_overheating},
                       {ATTRIBUTE(LOW_FLUID), expected_low_fluid},
                       {ATTRIBUTE(REPLACE_RECHARGE), expected_replace_recharge},
                       {ATTRIBUTE(DISCONNECTED), expected_disconnected}};
  }

  if (version >= 3) {
    expected_values.insert(
      {ATTRIBUTE(LOW_TEMPERATURE), expected_low_temperature});
  }

  for (const auto &[attribute, expected_value]: expected_values) {
    helper_test_attribute_value(attribute, expected_value);
  }
}

void test_battery_report_version_1_happy_case()
{
  helper_set_version(1);

  uint8_t expected_battery_level = 0x12;
  helper_test_report_frame(BATTERY_REPORT, {expected_battery_level});

  helper_battery_report(expected_battery_level);
}

void test_battery_report_version_2_happy_case()
{
  helper_set_version(2);

  uint8_t expected_battery_level = 0xFF;
  helper_test_report_frame(BATTERY_REPORT,
                           {expected_battery_level, 0b11111111, 0b11111111});

  helper_battery_report(expected_battery_level, 3, 1, 1, 1, 1, 3, 1);

  helper_test_report_frame(BATTERY_REPORT,
                           {expected_battery_level, 0b10010110, 0});

  helper_battery_report(expected_battery_level, 2, 0, 1, 0, 1, 2, 0);
}

void test_battery_report_version_3_happy_case()
{
  helper_set_version(3);

  uint8_t expected_battery_level = 0x0C;
  helper_test_report_frame(BATTERY_REPORT,
                           {expected_battery_level, 0b01101001, 0b11111110});

  helper_battery_report(expected_battery_level, 1, 1, 0, 1, 0, 1, 0, 1);
}

void test_battery_get_happy_case()
{
  helper_test_get_set_frame_happy_case(BATTERY_GET);
}

void test_battery_health_get_happy_case()
{
  helper_test_get_set_frame_happy_case(BATTERY_HEALTH_GET_V2);
}

}  // extern "C"