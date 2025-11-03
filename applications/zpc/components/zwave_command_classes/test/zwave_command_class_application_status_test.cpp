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
#include "zwave_command_class_application_status.h"
#include "zwave_command_class_application_status_types.h"
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

// Includes from other Unify Components
#include "sl_log.h"
#include "attribute_store.h"

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_APPLICATION_STATUS_##type

// Log tag
constexpr char LOG_TAG[] = "zwave_command_class_application_status";

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
  = {.command_class_id  = COMMAND_CLASS_APPLICATION_STATUS,
     .supported_version = APPLICATION_STATUS_VERSION};
// Get Set function map
const resolver_function_map attribute_bindings = {};

/// Called before each and every test
void setUp()
{
  zwave_setUp(command_class_handler,
              &zwave_command_class_application_status_init,
              attribute_bindings);
}

///////////////////////////////////////////////////////////////////////////////
// Internal helpers
///////////////////////////////////////////////////////////////////////////////
attribute_store::attribute helper_get_busy_status_node()
{
  return helper_test_and_get_node(ATTRIBUTE(BUSY_STATUS));
}

attribute_store::attribute helper_get_wait_time_node()
{
  auto busy_status_node = helper_get_busy_status_node();
  return helper_test_and_get_node(ATTRIBUTE(WAIT_TIME), busy_status_node);
}

attribute_store::attribute helper_get_reject_status_node()
{
  return helper_test_and_get_node(ATTRIBUTE(REJECT_STATUS));
}

///////////////////////////////////////////////////////////////////////////////
// Test cases
///////////////////////////////////////////////////////////////////////////////
void test_application_status_interview_v1_happy_case()
{
  helper_set_version(1);

  // Verify that we do not create node when version update
  helper_test_node_does_not_exists(ATTRIBUTE(BUSY_STATUS));
  helper_test_node_does_not_exists(ATTRIBUTE(REJECT_STATUS));
}

void test_application_status_busy_command_happy_case()
{
  helper_set_version(1);

  uint8_t tested_status    = 0x01;
  uint8_t tested_wait_time = 0x05;

  helper_test_report_frame(APPLICATION_BUSY, {tested_status, tested_wait_time});

  auto busy_status_node = helper_get_busy_status_node();
  sl_log_debug(LOG_TAG, "busy_status_node: %d", busy_status_node);
  auto wait_time_node = helper_get_wait_time_node();
  sl_log_debug(LOG_TAG, "wait_time_node: %d", wait_time_node);

  // Verify that we create necessary nodes
  helper_test_node_exists(ATTRIBUTE(BUSY_STATUS));
  helper_test_node_exists(ATTRIBUTE(WAIT_TIME), busy_status_node);

  // Verify that the application busy is updated
  TEST_ASSERT_EQUAL_MESSAGE(
    tested_status,
    busy_status_node.reported<application_busy_status>(),
    "Application Status Busy isn't updated after report");
  // Verify that the wait time is updated
  TEST_ASSERT_EQUAL_MESSAGE(
    tested_wait_time,
    wait_time_node.reported<application_busy_wait_time>(),
    "Wait time isn't updated after report");
}

void test_application_status_reject_command_happy_case()
{
  helper_set_version(1);

  uint8_t tested_status = 0x01;

  helper_test_report_frame(APPLICATION_REJECTED_REQUEST, {tested_status});

  auto reject_status_node = helper_get_reject_status_node();

  // Verify that the application busy is updated
  TEST_ASSERT_EQUAL_MESSAGE(
    tested_status,
    reject_status_node.reported<application_reject_request>(),
    "Application Status Reject isn't updated after report");
}

void test_application_status_reject_command_invalid_status()
{
  helper_set_version(1);

  uint8_t tested_status = 0x02;

  helper_test_report_frame(APPLICATION_REJECTED_REQUEST,
                           {tested_status},
                           SL_STATUS_FAIL);
}

}  // extern "C"