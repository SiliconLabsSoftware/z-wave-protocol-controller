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
#include "zwave_command_class_crc16.h"
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
#include "zap-types.h"

// ZPC includes
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store_type_registration.h"

// Test helpers
#include "zwave_command_class_test_helper.hpp"


// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_CRC16_##type


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
  = {.command_class_id    = COMMAND_CLASS_CRC_16_ENCAP,
     .supported_version   = CRC_16_ENCAP_VERSION,
     .scheme              = ZWAVE_CONTROLLER_ENCAPSULATION_NONE,
     .has_control_handler = false,
     .has_support_handler = true};

/// Called before each and every test
void setUp()
{
  zwave_setUp(command_class_handler,
              &zwave_command_class_crc16_init);
}

void helper_set_network_status(NodeStateNetworkStatus status)
{
  cpp_endpoint_id_node.parent()
    .emplace_node(DOTDOT_ATTRIBUTE_ID_STATE_NETWORK_STATUS)
    .set_reported(status);
}

void test_attribute_initialized_happy_case()
{
  helper_set_version(1);

  // Set the network status to online
  helper_set_network_status(ZCL_NODE_STATE_NETWORK_STATUS_ONLINE_FUNCTIONAL);

  // Test that the attribute is initialized
  auto disable_crc16_node = helper_test_and_get_node(ATTRIBUTE(DISABLE_CRC16));
  TEST_ASSERT_EQUAL_MESSAGE(0x00,
                            disable_crc16_node.reported<uint8_t>(),
                            "DISABLE_CRC16 flag should be set to off");

  TEST_ASSERT_EQUAL_MESSAGE(
    true,
    zwave_command_class_crc16_is_supported(endpoint_id_node),
    "CRC16 should be marked as supported");

  // Set disabled to true
    disable_crc16_node.set_reported<uint8_t>(0x01);

  TEST_ASSERT_EQUAL_MESSAGE(
    false,
    zwave_command_class_crc16_is_supported(endpoint_id_node),
    "CRC16 should be marked as supported");
}

void test_zwave_command_class_crc16_is_supported_interview()
{
  helper_set_version(1);

  // Set the network status to online
  helper_set_network_status(ZCL_NODE_STATE_NETWORK_STATUS_ONLINE_INTERVIEWING);

  // Test that the attribute is initialized
  auto disable_crc16_node = helper_test_and_get_node(ATTRIBUTE(DISABLE_CRC16));
  TEST_ASSERT_EQUAL_MESSAGE(0x00,
                            disable_crc16_node.reported<uint8_t>(),
                            "DISABLE_CRC16 flag should be set to off");

  TEST_ASSERT_EQUAL_MESSAGE(
    false,
    zwave_command_class_crc16_is_supported(endpoint_id_node),
    "CRC16 should be marked as NOT supported since we are interviewing");

  helper_set_network_status(ZCL_NODE_STATE_NETWORK_STATUS_ONLINE_FUNCTIONAL);

  TEST_ASSERT_EQUAL_MESSAGE(
    true,
    zwave_command_class_crc16_is_supported(endpoint_id_node),
    "CRC16 should be marked as supported since we are done with the interview");
}

void test_zwave_command_class_crc16_expect_crc16_response_once_happy_case()
{
  TEST_ASSERT_EQUAL_MESSAGE(
    false,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id, endpoint_id),
    "Current endpoint should not be marked as expecting a response");

  zwave_command_class_crc16_set_expect_crc16_response(node_id, endpoint_id);

  TEST_ASSERT_EQUAL_MESSAGE(
    true,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id, endpoint_id),
    "Current endpoint should be marked as expecting a response");

  zwave_command_class_crc16_clear_expect_crc16_response(node_id, endpoint_id);

  TEST_ASSERT_EQUAL_MESSAGE(
    false,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id, endpoint_id),
    "Current endpoint should not be marked as expecting a response since it "
    "should be cleared");
}

void test_zwave_command_class_crc16_expect_crc16_response_stacked_happy_case()
{
  constexpr int MAX_RESPONSES = 12;
  // test if we can set the flag multiple times
  for (int i = 0; i < MAX_RESPONSES; i++) {
    zwave_command_class_crc16_set_expect_crc16_response(node_id, endpoint_id);
  }
  // test if we can set the flag to another response
  zwave_command_class_crc16_set_expect_crc16_response(node_id, endpoint_id + 1);

  TEST_ASSERT_EQUAL_MESSAGE(
    true,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id, endpoint_id),
    "Current endpoint should be marked as expecting a response");

  // Clear 1 response
  zwave_command_class_crc16_clear_expect_crc16_response(node_id, endpoint_id);

  TEST_ASSERT_EQUAL_MESSAGE(
    true,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id, endpoint_id),
    "Current endpoint should be marked as expecting a response since it "
    "still have responses to send");

  // Test if we can clear all the responses
  // Also test if we can clear more than the number of responses
  for (int i = 0; i < MAX_RESPONSES; i++) {
    zwave_command_class_crc16_clear_expect_crc16_response(node_id, endpoint_id);
  }

  TEST_ASSERT_EQUAL_MESSAGE(
    false,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id, endpoint_id),
    "Current endpoint should NOT be marked as expecting a response since it "
    "should be all cleared");

  // Test other frame
  TEST_ASSERT_EQUAL_MESSAGE(
    true,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id,
                                                          endpoint_id + 1),
    "Other endpoint should be marked as expecting a response since it "
    "still have responses to send");

  zwave_command_class_crc16_clear_expect_crc16_response(node_id,
                                                        endpoint_id + 1);

  TEST_ASSERT_EQUAL_MESSAGE(
    false,
    zwave_command_class_crc16_is_expecting_crc16_response(node_id,
                                                          endpoint_id + 1),
    "Other endpoint should NOT be marked as expecting a response since it "
    "should be cleared");
}

} // extern "C"
