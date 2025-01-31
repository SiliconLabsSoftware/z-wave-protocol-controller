/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

// Generic includes
#include <string.h>

// Test includes
#include "unity.h"

// Interface includes
#include "attribute_store_defined_attribute_types.h"
#include "zwave_command_class_wake_up_types.h"
#include "ZW_classcmd.h"
#include "zwave_command_class_protocol.h"
#include "zwave_controller_utils.h"

// Includes from other components
#include "sl_log.h"
#include "zwave_controller_connection_info.h"
#include "zwave_tx_groups.h"

// Mock includes
#include "zwave_command_handler_mock.h"
#include "zwave_tx_scheme_selector_mock.h"
#include "attribute_store_mock.h"
#include "attribute_store_helper_mock.h"
#include "attribute_resolver_mock.h"
#include "zwapi_protocol_controller_mock.h"
#include "zwave_controller_keyset_mock.h"
#include "zwave_tx_mock.h"
#include "zwave_controller_callbacks_mock.h"

#define LOG_TAG "zwave_command_class_protocol_test"

static zwave_command_handler_t protocol_handler = {};

static sl_status_t zwave_command_handler_register_handler_stub(
  zwave_command_handler_t new_command_class_handler, int cmock_num_calls)
{
  protocol_handler = new_command_class_handler;

  TEST_ASSERT_EQUAL(ZWAVE_CONTROLLER_ENCAPSULATION_NONE,
                    protocol_handler.minimal_scheme);
  TEST_ASSERT_TRUE(ZWAVE_CMD_CLASS_PROTOCOL ==protocol_handler.command_class
                  || ZWAVE_CMD_CLASS_PROTOCOL_LR == protocol_handler.command_class);
  TEST_ASSERT_EQUAL(1, protocol_handler.version);
  TEST_ASSERT_NULL(protocol_handler.control_handler);
  TEST_ASSERT_NOT_NULL(protocol_handler.support_handler);
  TEST_ASSERT_FALSE(protocol_handler.manual_security_validation);

  return SL_STATUS_OK;
}

static sl_status_t zwave_controller_register_callback_stub(
  const zwave_controller_callbacks_t *callback, int cmock_num_calls)
{
  TEST_ASSERT_NOT_NULL(callback);
  TEST_ASSERT_NOT_NULL(callback->on_protocol_cc_encryption_request);

  return SL_STATUS_OK;
}

void suiteSetUp() {}

int suiteTearDown(int num_failures)
{
  return num_failures;
}

void setUp()
{
  // Handler registration
  zwave_command_handler_register_handler_Stub(
    &zwave_command_handler_register_handler_stub);
  
  zwave_controller_register_callbacks_Stub(
    &zwave_controller_register_callback_stub);

  zwave_command_class_protocol_init();
}

void tearDown() {}

void test_zwave_command_class_protocol_init()
{
  // Call the function
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwave_command_class_protocol_init());
}

void test_zwave_command_class_protocol_handler(void)
{
  uint8_t test_frame_data[5] = { 0};

  test_frame_data[0] = ZWAVE_CMD_CLASS_PROTOCOL;

  zwave_controller_connection_info_t connection = { 0 };
  connection.encapsulation = ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_AUTHENTICATED;
  connection.remote.node_id = 2;
  connection.local.node_id = 1;

  // Test with wrong command length
  TEST_ASSERT_EQUAL(
  SL_STATUS_NOT_SUPPORTED,
  protocol_handler.support_handler(&connection,
                                   test_frame_data,
                                   1)); // wrong length

  // Test with correct length
  zwave_controller_get_key_from_encapsulation_ExpectAndReturn(ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_AUTHENTICATED, ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);

  zwapi_transfer_protocol_cc_ExpectAndReturn(2,
                                             ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY,
                                             5,
                                             (uint8_t*) &test_frame_data,
                                             SL_STATUS_OK);

  TEST_ASSERT_EQUAL(
  SL_STATUS_OK,
  protocol_handler.support_handler(&connection,
                                   test_frame_data,
                                   sizeof(test_frame_data)));
}

void test_zwave_s2_on_protocol_cc_encryption_request_happy_case()
{
  zwave_node_id_t destination_node_id = 2;
  uint8_t payload[]                   = {0x01, 0x02, 0x03};
  uint8_t payload_length              = sizeof(payload) / sizeof(payload[0]);
  uint8_t protocol_metadata[]         = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7};
  uint8_t protocol_metadata_length    = sizeof(protocol_metadata) / sizeof(protocol_metadata[0]);
  uint8_t use_supervision             = 0;
  uint8_t session_id                  = 1;
  zwave_controller_connection_info_t connection_info = {0};
  zwave_tx_options_t tx_options                      = {0};
  zwave_tx_session_id_t tx_session_id                = NULL;
  protocol_metadata_t metadata                       = {0};

  // Expected TX options
  tx_options.transport.is_protocol_frame = true;

  // Expected protocol metadata
  metadata.session_id  = session_id;
  metadata.data_length = protocol_metadata_length;
  memcpy(metadata.data, protocol_metadata, protocol_metadata_length);

  zwave_tx_scheme_get_node_connection_info_Expect(destination_node_id, 0, NULL);
  zwave_tx_scheme_get_node_connection_info_IgnoreArg_connection_info();
  zwave_tx_scheme_get_node_tx_options_Expect(ZWAVE_TX_QOS_MAX_PRIORITY, 1, 5000, NULL);
  zwave_tx_scheme_get_node_tx_options_IgnoreArg_tx_options();

  zwave_tx_send_data_ExpectWithArrayAndReturn(&connection_info,
                                              sizeof(connection_info),
                                              payload_length,
                                              payload,
                                              sizeof(payload),
                                              &tx_options,
                                              sizeof(tx_options),
                                              NULL,
                                              (void *)&metadata,
                                              sizeof(protocol_metadata_t),
                                              &tx_session_id,
                                              sizeof(zwave_tx_session_id_t),
                                              SL_STATUS_OK);
  zwave_tx_send_data_IgnoreArg_on_send_complete();

  zwave_on_protocol_cc_encryption_request(destination_node_id,
                                          payload_length,
                                          payload,
                                          protocol_metadata_length,
                                          protocol_metadata,
                                          use_supervision,
                                          session_id);

  // Test with supervision
  use_supervision = 1;

  zwave_tx_scheme_get_node_connection_info_Expect(destination_node_id, 0, NULL);
  zwave_tx_scheme_get_node_connection_info_IgnoreArg_connection_info();
  zwave_tx_scheme_get_node_tx_options_Expect(ZWAVE_TX_QOS_MAX_PRIORITY, 1, 5000, NULL);
  zwave_tx_scheme_get_node_tx_options_IgnoreArg_tx_options();

  uint16_t supervision_frame_size = 4 + payload_length; // sizeof(frame) - SUPERVISION_ENCAPSULATED_COMMAND_MAXIMUM_SIZE
  tx_options.number_of_responses += 1; // supervision get expects 1 frame in response

  zwave_tx_send_data_ExpectWithArrayAndReturn(&connection_info,
                                              sizeof(connection_info),
                                              supervision_frame_size,
                                              payload,
                                              sizeof(payload),
                                              &tx_options,
                                              sizeof(tx_options),
                                              NULL,
                                              (void *)&metadata,
                                              sizeof(protocol_metadata_t),
                                              &tx_session_id,
                                              sizeof(zwave_tx_session_id_t),
                                              SL_STATUS_OK);
  zwave_tx_send_data_IgnoreArg_on_send_complete();
  zwave_tx_send_data_IgnoreArg_data();
  zwave_tx_send_data_IgnoreArg_user();

  zwave_on_protocol_cc_encryption_request(destination_node_id,
                                          payload_length,
                                          payload,
                                          protocol_metadata_length,
                                          protocol_metadata,
                                          use_supervision,
                                          session_id);
}

