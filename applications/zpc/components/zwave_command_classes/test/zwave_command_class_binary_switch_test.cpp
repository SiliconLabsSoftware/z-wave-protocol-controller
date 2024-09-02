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
#include "zwave_command_class_binary_switch.h"
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
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_BINARY_SWITCH_##type

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
  = {.command_class_id  = COMMAND_CLASS_SWITCH_BINARY,
     .supported_version = 2,
     .scheme            = ZWAVE_CONTROLLER_ENCAPSULATION_NONE};
// Get Set function map
const resolver_function_map attribute_bindings = {
  {ATTRIBUTE(VALUE), {SWITCH_BINARY_GET, SWITCH_BINARY_SET}},
};

/// Called before each and every test
void setUp()
{
  zwave_setUp(command_class_handler,
              &zwave_command_class_binary_switch_init,
              attribute_bindings);
}

///////////////////////////////////////////////////////////////////////////////
// Internal helpers
///////////////////////////////////////////////////////////////////////////////
attribute_store::attribute helper_get_value_node()
{
  auto state_node = helper_test_and_get_node(ATTRIBUTE(STATE));

  return helper_test_and_get_node(ATTRIBUTE(VALUE), state_node);
}

attribute_store::attribute helper_get_duration_node()
{
  auto state_node = helper_test_and_get_node(ATTRIBUTE(STATE));

  return helper_test_and_get_node(ATTRIBUTE(DURATION), state_node);
}

///////////////////////////////////////////////////////////////////////////////
// Test cases
///////////////////////////////////////////////////////////////////////////////
void test_binary_switch_interview_v1_happy_case()
{
  helper_set_version(1);

  auto state_node = helper_test_and_get_node(ATTRIBUTE(STATE));

  // Verify that we have the correct node(s)
  helper_test_node_exists(ATTRIBUTE(VALUE), state_node);
  helper_test_node_does_not_exists(ATTRIBUTE(DURATION), state_node);
}

void test_binary_switch_interview_v2_happy_case()
{
  helper_set_version(2);

  auto state_node = helper_test_and_get_node(ATTRIBUTE(STATE));

  // Verify that we have the correct node(s)
  helper_test_node_exists(ATTRIBUTE(VALUE), state_node);
  helper_test_node_exists(ATTRIBUTE(DURATION), state_node);
}

void test_binary_switch_get_happy_case()
{
  helper_test_get_set_frame_happy_case(SWITCH_BINARY_GET);
}

void test_binary_switch_set_version_1_happy_case()
{
  helper_set_version(1);

  uint8_t tested_value = 0xFF;
  auto value_node      = helper_get_value_node();

  // Test with reported value
  value_node.set_reported(tested_value);
  helper_test_get_set_frame_happy_case(SWITCH_BINARY_SET,
                                       value_node,
                                       {tested_value});

  // Test with desired value
  tested_value = 0x00;
  value_node.set_desired(tested_value);
  helper_test_get_set_frame_happy_case(SWITCH_BINARY_SET,
                                       value_node,
                                       {tested_value});
}

void test_binary_switch_set_version_2_happy_case()
{
  helper_set_version(2);

  auto value_node    = helper_get_value_node();
  auto duration_node = helper_get_duration_node();

  uint8_t tested_value    = 0xFF;
  uint8_t tested_duration = 0x55;

  // Set value node
  value_node.set_reported(tested_value);
  // Set duration
  duration_node.set_reported(tested_duration + 1);
  // See if we take the duration from the desired value
  duration_node.set_desired(tested_duration);

  helper_test_get_set_frame_happy_case(SWITCH_BINARY_SET,
                                       value_node,
                                       {tested_value, tested_duration});
}

void test_binary_switch_report_version_1_happy_case()
{
  helper_set_version(1);

  uint8_t tested_value = 0xFF;
  auto value_node      = helper_get_value_node();

  helper_test_report_frame(SWITCH_BINARY_REPORT, {tested_value});

  // Verify that the value is updated
  TEST_ASSERT_EQUAL_MESSAGE(tested_value,
                            value_node.reported<uint8_t>(),
                            "Value isn't updated after report");
}

void test_binary_switch_report_version_2_happy_case()
{
  helper_set_version(2);

  uint8_t tested_value    = 0xFF;
  uint8_t tested_duration = 0x55;

  auto value_node    = helper_get_value_node();
  auto duration_node = helper_get_duration_node();

  helper_test_report_frame(SWITCH_BINARY_REPORT,
                           {tested_value, tested_value, tested_duration});

  // Verify that the value is updated
  TEST_ASSERT_EQUAL_MESSAGE(tested_value,
                            value_node.reported<uint8_t>(),
                            "Value isn't updated after report");

  // Verify that the duration is updated
  TEST_ASSERT_EQUAL_MESSAGE(tested_duration,
                            duration_node.reported<uint8_t>(),
                            "Duration isn't updated after report");
}

void test_binary_switch_report_invalid_size()
{
  helper_set_version(1);

  helper_test_report_frame(SWITCH_BINARY_REPORT, {0x12, 0x12}, SL_STATUS_FAIL);
}

void test_binary_switch_report_no_node_state()
{
  helper_test_report_frame(SWITCH_BINARY_REPORT, {0x12}, SL_STATUS_FAIL);
}

void test_binary_switch_report_invalid_value_v1() {
  helper_set_version(1);

  auto value_node = helper_get_value_node();

  helper_test_report_frame(SWITCH_BINARY_REPORT, {0xFC}, SL_STATUS_FAIL);

  // Verify that the value is not updated
  TEST_ASSERT_FALSE_MESSAGE(value_node.reported_exists(),
                            "Value should not be updated (reported) after invalid report");
  TEST_ASSERT_FALSE_MESSAGE(value_node.desired_exists(),
                            "Value should not be updated (desired) after invalid report");
}

void test_binary_switch_report_invalid_value_v2() {
  helper_set_version(2);

  auto value_node = helper_get_value_node();

  // Invalid destination value
  helper_test_report_frame(SWITCH_BINARY_REPORT,
                           {0x00, 0xFE, 0x05},
                           SL_STATUS_FAIL);

  // Verify that the value is not updated
  TEST_ASSERT_FALSE_MESSAGE(value_node.reported_exists(),
                            "Value should not be updated (reported) after invalid report");
  TEST_ASSERT_FALSE_MESSAGE(value_node.desired_exists(),
                            "Value should not be updated (desired) after invalid report");
}

void test_binary_switch_report_adjusted_value_v1() {
  helper_set_version(1);

  auto value_node = helper_get_value_node();

  // Invalid destination value
  helper_test_report_frame(SWITCH_BINARY_REPORT,
                           {0x12});

  // Verify that the value is not updated
   // Verify that the duration is updated
  TEST_ASSERT_EQUAL_MESSAGE(0xFF,
                            value_node.reported<uint8_t>(),
                            "Value should be adjusted to 0xFF after report");
}

void test_binary_switch_report_adjusted_value_v2()
{
  helper_set_version(2);

  auto value_node    = helper_get_value_node();
  auto duration_node = helper_get_duration_node();

  helper_test_report_frame(SWITCH_BINARY_REPORT,
                           {0xFF, 0x12, 0xFF});

  // Verify that the value is updated
  TEST_ASSERT_EQUAL_MESSAGE(0xFF,
                            value_node.reported<uint8_t>(),
                            "Value should be adjusted to 0xFF after report");

  // Verify that the duration is updated
  TEST_ASSERT_EQUAL_MESSAGE(0x00,
                            duration_node.reported<uint8_t>(),
                            "Duration value should be adjusted");
}

}  // extern "C"