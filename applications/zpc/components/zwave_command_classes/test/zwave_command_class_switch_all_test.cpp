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
#include "zwave_command_class_switch_all.h"
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


constexpr uint8_t SWITCH_ALL_ON_VALUE = 1;
constexpr uint8_t SWITCH_ALL_OFF_VALUE = 0;

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_SWITCH_ALL_##type


using namespace zwave_command_class_test_helper;

extern "C" {

// Mock helper
#include "dotdot_mqtt_mock.h"
#include "zpc_attribute_store_network_helper_mock.h"

static uic_mqtt_dotdot_unify_switch_all_write_attributes_callback_t switch_all_mock_callback;
static void uic_mqtt_dotdot_unify_switch_all_write_attributes_callback_stub(
                const uic_mqtt_dotdot_unify_switch_all_write_attributes_callback_t callback, 
                int cmock_num_calls)
{
  switch_all_mock_callback = callback;
}

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
  = {.command_class_id  = COMMAND_CLASS_SWITCH_ALL,
     .supported_version = SWITCH_ALL_VERSION};
// Get Set function map
const resolver_function_map attribute_bindings = {
  {ATTRIBUTE(MODE), {SWITCH_ALL_GET, SWITCH_ALL_SET}},
  {ATTRIBUTE(ON_OFF), {0, SWITCH_ALL_ON}},
  {ATTRIBUTE(ON_OFF), {0, SWITCH_ALL_OFF}}
};

/// Called before each and every test
void setUp()
{
  uic_mqtt_dotdot_set_unify_switch_all_write_attributes_callback_Stub(uic_mqtt_dotdot_unify_switch_all_write_attributes_callback_stub);
  zwave_setUp(command_class_handler,
              &zwave_command_class_switch_all_init,
              attribute_bindings);
}

///////////////////////////////////////////////////////////////////////////////
// Test cases
///////////////////////////////////////////////////////////////////////////////
void test_switch_all_interview_happy_case()
{
  helper_set_version(SWITCH_ALL_VERSION);

  // Verify that we have the correct node(s)
  helper_test_node_exists(ATTRIBUTE(MODE));
  helper_test_node_exists(ATTRIBUTE(ON_OFF));
}

void test_switch_all_get_happy_case()
{
  helper_set_version(SWITCH_ALL_VERSION);

  auto mode_node = helper_test_and_get_node(ATTRIBUTE(MODE));

  helper_test_get_set_frame_happy_case(SWITCH_ALL_GET, mode_node);
}

void test_switch_all_set_happy_case()
{
  helper_set_version(SWITCH_ALL_VERSION);

  auto mode_node = helper_test_and_get_node(ATTRIBUTE(MODE));
  uint8_t mode = 0xFF;

  // Test with desired value
  mode_node.set_desired(mode);
  helper_test_get_set_frame_happy_case(SWITCH_ALL_SET,
                                        mode_node,
                                        {mode});
}

void test_switch_all_set_on_happy_case()
{
  helper_set_version(SWITCH_ALL_VERSION);

  auto mode_node = helper_test_and_get_node(ATTRIBUTE(MODE));
  auto on_off_node = helper_test_and_get_node(ATTRIBUTE(ON_OFF));
  uint8_t mode = 0xFF;

  // Test with desired value
  mode_node.set_reported(mode);
  on_off_node.set_desired(SWITCH_ALL_ON_VALUE);
  helper_test_get_set_frame_happy_case(SWITCH_ALL_ON,
                                        on_off_node);
}

void test_switch_all_set_on_fail_case()
{
  helper_set_version(SWITCH_ALL_VERSION);

  auto mode_node = helper_test_and_get_node(ATTRIBUTE(MODE));
  auto on_off_node = helper_test_and_get_node(ATTRIBUTE(ON_OFF));
  uint8_t mode = 0x00;

  // Test with desired value
  mode_node.set_reported(mode);
  on_off_node.set_desired(SWITCH_ALL_ON_VALUE);
  helper_test_get_set_fail_case(SWITCH_ALL_ON,
                                        SL_STATUS_OK,
                                        on_off_node);

  helper_test_get_set_fail_case(SWITCH_ALL_ON,
                                        SL_STATUS_INVALID_TYPE);

  mode = 0xFF;
  mode_node.set_reported(mode);
  on_off_node.set_desired<uint8_t>(0xFF);
  helper_test_get_set_fail_case(SWITCH_ALL_ON,
                                        SL_STATUS_INVALID_RANGE,
                                        on_off_node);

}

void test_switch_all_report_happy_case()
{
  helper_set_version(SWITCH_ALL_VERSION);
  auto mode_node = helper_test_and_get_node(ATTRIBUTE(MODE));
  uint8_t mode = 0;

  attribute_store_network_helper_get_endpoint_node_IgnoreAndReturn(cpp_endpoint_id_node);
  helper_test_report_frame(SWITCH_ALL_REPORT, {mode});
  TEST_ASSERT_EQUAL(mode, mode_node.reported<uint8_t>());

  mode = 0x02;
  helper_test_report_frame(SWITCH_ALL_REPORT, {mode});
  attribute_store_network_helper_get_endpoint_node_StopIgnore();
  TEST_ASSERT_EQUAL(mode, mode_node.reported<uint8_t>());
}

void test_switch_all_report_fail_case()
{
  helper_set_version(SWITCH_ALL_VERSION);
  uint8_t mode = 0;

  attribute_store_network_helper_get_endpoint_node_IgnoreAndReturn(cpp_endpoint_id_node);
  helper_test_report_frame(SWITCH_ALL_REPORT, {mode, mode}, SL_STATUS_FAIL);
  attribute_store_network_helper_get_endpoint_node_StopIgnore();
}

void test_switch_all_set_off_happy_case()
{
  helper_set_version(SWITCH_ALL_VERSION);

  auto mode_node = helper_test_and_get_node(ATTRIBUTE(MODE));
  auto on_off_node = helper_test_and_get_node(ATTRIBUTE(ON_OFF));
  uint8_t mode = 0xFF;

  // Test with desired value
  mode_node.set_reported(mode);
  on_off_node.set_desired(SWITCH_ALL_OFF_VALUE);
  helper_test_get_set_frame_happy_case(SWITCH_ALL_OFF,
                                        on_off_node);
}

void test_switch_all_set_off_fail_case()
{
  helper_set_version(SWITCH_ALL_VERSION);

  auto mode_node = helper_test_and_get_node(ATTRIBUTE(MODE));
  auto on_off_node = helper_test_and_get_node(ATTRIBUTE(ON_OFF));
  uint8_t mode = 0x0;

  // Test with desired value
  mode_node.set_reported(mode);
  on_off_node.set_desired(SWITCH_ALL_OFF_VALUE);
  helper_test_get_set_fail_case(SWITCH_ALL_OFF,
                                        SL_STATUS_OK,
                                        on_off_node);

  helper_test_get_set_fail_case(SWITCH_ALL_OFF,
                                        SL_STATUS_INVALID_TYPE);
}

void test_switch_all_write_command_handler()
{
  helper_set_version(SWITCH_ALL_VERSION);

  attribute_store::attribute home_node(home_id_node);

  auto unid_node_second = home_node.add_node(ATTRIBUTE_NODE_ID);
  unid_node_second.set_reported<std::string>("zw-CAFECAFE-0008");
  auto ep_node_second = unid_node_second.emplace_node<>(ATTRIBUTE_ENDPOINT_ID, endpoint_id);
  helper_set_version(SWITCH_ALL_VERSION, ep_node_second);

  attribute_store_network_helper_get_home_id_node_ExpectAndReturn("zw-CAFECAFE-0008", home_node);
  attribute_store_network_helper_get_node_id_node_ExpectAndReturn("zw-CAFECAFE-0008", unid_node_second);
  uic_mqtt_dotdot_unify_switch_all_state_t state = {0xFF, 1};
  uic_mqtt_dotdot_unify_switch_all_updated_state_t updates = {true, false};
  switch_all_mock_callback("zw-CAFECAFE-0008", endpoint_id, UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                            state, updates);
  auto dotdot_mode_second = helper_test_and_get_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SWITCH_ALL_MODE, ep_node_second);
  TEST_ASSERT_EQUAL(state.mode, dotdot_mode_second.desired<uint8_t>());

  attribute_store_network_helper_get_home_id_node_ExpectAndReturn("zw-CAFECAFE-0008", home_node);
  updates.mode = false;
  updates.on_off = true;
  switch_all_mock_callback("zw-CAFECAFE-0008", endpoint_id, UIC_MQTT_DOTDOT_CALLBACK_TYPE_NORMAL,
                            state, updates);
  auto dotdot_onoff_second = helper_test_and_get_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SWITCH_ALL_ON_OFF, ep_node_second);
  auto dotdot_onoff = helper_test_and_get_node(DOTDOT_ATTRIBUTE_ID_UNIFY_SWITCH_ALL_ON_OFF, cpp_endpoint_id_node);
  TEST_ASSERT_EQUAL(state.on_off, dotdot_onoff.desired<uint8_t>());
  TEST_ASSERT_EQUAL(state.on_off, dotdot_onoff_second.desired<uint8_t>());
}

} // extern "C"