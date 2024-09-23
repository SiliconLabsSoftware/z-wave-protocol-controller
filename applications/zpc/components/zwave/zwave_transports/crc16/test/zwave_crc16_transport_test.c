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
#include "zwave_crc16_transport.h"

// Generic includes
#include <string.h>

// Test includes
#include "unity.h"

// Includes from other components
#include "ZW_classcmd.h"

// Mocks
#include "zwave_controller_transport_mock.h"
#include "zwave_controller_internal_mock.h"
#include "zwave_controller_crc16_mock.h"
#include "zwave_utils_mock.h"
#include "zwave_tx_mock.h"
#include "zwave_command_class_crc16_mock.h"

#define CRC_INITAL_VALUE 0x1D0Fu

// Static variables
static zwave_controller_transport_t crc_16_transport                    = {};
static zwave_controller_connection_info_t last_received_connection_info = {};
static zwave_tx_options_t last_received_tx_options                      = {};
static zwave_rx_receive_options_t last_received_rx_options              = {};
static uint8_t last_received_frame[ZWAVE_MAX_FRAME_SIZE]                = {};
static uint8_t last_received_frame_length                               = 0;
static on_zwave_tx_send_data_complete_t crc_16_on_send_complete         = NULL;
static void *last_received_user_pointer                                 = NULL;
static uint8_t send_data_callback_counter                               = 0;
static uint8_t my_user_variable                                         = 0x93;

sl_status_t zwave_controller_transport_register_stub(
  const zwave_controller_transport_t *transport, int cmock_num_calls)
{
  TEST_ASSERT_EQUAL(COMMAND_CLASS_CRC_16_ENCAP, transport->command_class);
  TEST_ASSERT_EQUAL(CRC_16_ENCAP_VERSION, transport->version);
  TEST_ASSERT_EQUAL(4, transport->priority);

  // Save the transport for our tests
  crc_16_transport = *transport;

  return SL_STATUS_OK;
}

void zwave_controller_on_frame_received_stub(
  const zwave_controller_connection_info_t *connection_info,
  const zwave_rx_receive_options_t *rx_options,
  const uint8_t *frame_data,
  uint16_t frame_length,
  int cmock_num_calls)
{
  // Save the received data for test verification
  last_received_connection_info = *connection_info;
  last_received_rx_options      = *rx_options;
  memcpy(last_received_frame, frame_data, frame_length);
  last_received_frame_length = frame_length;
}

sl_status_t zwave_tx_send_data_stub(
  const zwave_controller_connection_info_t *connection,
  uint16_t data_length,
  const uint8_t *data,
  const zwave_tx_options_t *tx_options,
  const on_zwave_tx_send_data_complete_t on_send_complete,
  void *user,
  zwave_tx_session_id_t *session,
  int cmock_num_calls)
{
  // Save the received data for test verification
  last_received_connection_info = *connection;
  last_received_tx_options      = *tx_options;
  crc_16_on_send_complete       = on_send_complete;
  last_received_user_pointer    = user;
  memcpy(last_received_frame, data, data_length);
  last_received_frame_length = data_length;

  TEST_ASSERT_EQUAL_PTR(NULL, session);
  return SL_STATUS_OK;
}

static void test_send_data_callback(uint8_t status,
                                    const zwapi_tx_report_t *tx_info,
                                    void *user)
{
  send_data_callback_counter += 1;
  TEST_ASSERT_EQUAL_PTR(&my_user_variable, user);
  TEST_ASSERT_EQUAL_PTR(NULL, tx_info);
}

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp() {}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  return num_failures;
}

/// Called before each and every test
void setUp()
{
  send_data_callback_counter = 0;
  crc_16_on_send_complete    = NULL;
  last_received_user_pointer = NULL;
}

// Keep this test first
void test_zwave_crc_16_transport_init()
{
  // Nothing to test here really. We intercept the registered transport.
  zwave_controller_transport_register_AddCallback(
    zwave_controller_transport_register_stub);

  zwave_controller_transport_register_ExpectAndReturn(NULL, SL_STATUS_OK);
  zwave_controller_transport_register_IgnoreArg_transport();
  // Call the function
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwave_crc16_transport_init());
}

void test_decapsulation_happy_case()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.on_frame_received);

  zwave_controller_connection_info_t connection_info = {};
  zwave_rx_receive_options_t rx_options              = {};
  const uint8_t frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03, 0x04, 0x05};
  const uint8_t checksum_frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03};

  zwave_controller_on_frame_received_Stub(
    zwave_controller_on_frame_received_stub);

  zwave_controller_crc16_ExpectAndReturn(CRC_INITAL_VALUE,
                                         checksum_frame_data,
                                         5,
                                         0x0405);

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    crc_16_transport.on_frame_received(&connection_info,
                                                       &rx_options,
                                                       frame_data,
                                                       sizeof(frame_data)));

  // Verify that our decapsulation worked
  TEST_ASSERT_EQUAL(0x00, last_received_connection_info.remote.endpoint_id);
  TEST_ASSERT_EQUAL(0x00, last_received_connection_info.local.endpoint_id);
  TEST_ASSERT_EQUAL(false, last_received_connection_info.local.is_multicast);
  TEST_ASSERT_EQUAL(3, last_received_frame_length);
  const uint8_t expected_frame_data[] = {0x01, 0x02, 0x03};
  TEST_ASSERT_EQUAL_INT8_ARRAY(expected_frame_data,
                               last_received_frame,
                               last_received_frame_length);
}

void test_decapsulation_overflow()
{
  //zwave_command_class_crc_16_transport_init_verification();
  // Test with wrong command
  TEST_ASSERT_NOT_NULL(crc_16_transport.on_frame_received);

  zwave_controller_connection_info_t connection_info = {};
  zwave_rx_receive_options_t rx_options              = {};
  const uint8_t frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03, 0x04, 0x05};

  TEST_ASSERT_EQUAL(
    SL_STATUS_WOULD_OVERFLOW,
    crc_16_transport.on_frame_received(&connection_info,
                                       &rx_options,
                                       frame_data,
                                       ZWAVE_MAX_FRAME_SIZE
                                         + CRC_16_ENCAPSULATION_OVERHEAD + 1));
}

void test_decapsulation_too_short_frame()
{
  //zwave_command_class_crc_16_transport_init_verification();
  // Test with wrong command
  TEST_ASSERT_NOT_NULL(crc_16_transport.on_frame_received);

  zwave_controller_connection_info_t connection_info = {};
  zwave_rx_receive_options_t rx_options              = {};
  const uint8_t frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03, 0x04, 0x05};

  TEST_ASSERT_EQUAL(SL_STATUS_NOT_SUPPORTED,
                    crc_16_transport.on_frame_received(&connection_info,
                                                       &rx_options,
                                                       frame_data,
                                                       4));
}

void test_decapsulation_too_wrong_command()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.on_frame_received);

  zwave_controller_connection_info_t connection_info = {};
  zwave_rx_receive_options_t rx_options              = {};
  const uint8_t frame_data[] = {COMMAND_CLASS_CRC_16_ENCAP,
                                CRC_16_ENCAP + 1,
                                0x01,
                                0x02,
                                0x03,
                                0x04,
                                0x05};

  TEST_ASSERT_EQUAL(SL_STATUS_NOT_FOUND,
                    crc_16_transport.on_frame_received(&connection_info,
                                                       &rx_options,
                                                       frame_data,
                                                       sizeof(frame_data)));
}

void test_decapsulation_too_wrong_command_class()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.on_frame_received);

  zwave_controller_connection_info_t connection_info = {};
  zwave_rx_receive_options_t rx_options              = {};
  const uint8_t frame_data[] = {COMMAND_CLASS_CRC_16_ENCAP + 1,
                                CRC_16_ENCAP,
                                0x01,
                                0x02,
                                0x03,
                                0x04,
                                0x05};

  TEST_ASSERT_EQUAL(SL_STATUS_NOT_FOUND,
                    crc_16_transport.on_frame_received(&connection_info,
                                                       &rx_options,
                                                       frame_data,
                                                       sizeof(frame_data)));
}

void test_encapsulation_happy_case()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);

  zwave_controller_connection_info_t connection_info = {};
  zwave_tx_options_t tx_options                      = {};
  tx_options.number_of_responses                     = 0; 
  const uint8_t frame_data[]                         = {0x01, 0x02, 0x03};
  connection_info.remote.endpoint_id                 = 2;
  connection_info.remote.node_id                     = 5;
  connection_info.local.endpoint_id                  = 1;
  connection_info.local.node_id                      = 0;
  connection_info.encapsulation           = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  zwave_tx_session_id_t parent_session_id = (void *)23;

  zwave_tx_send_data_Stub(zwave_tx_send_data_stub);

  zwave_get_endpoint_node_ExpectAndReturn(connection_info.remote.node_id,
                                          connection_info.remote.endpoint_id,
                                          0);
  zwave_command_class_crc16_is_supported_ExpectAndReturn(0, true);
  zwave_command_class_crc16_is_expecting_crc16_response_ExpectAndReturn(
    connection_info.remote.node_id,
    connection_info.remote.endpoint_id,
    true);
  zwave_command_class_crc16_clear_expect_crc16_response_Expect(
    connection_info.remote.node_id,
    connection_info.remote.endpoint_id);

  const uint8_t checksum_frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03};
  zwave_controller_crc16_ExpectAndReturn(CRC_INITAL_VALUE,
                                         checksum_frame_data,
                                         5,
                                         0x0405);

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    crc_16_transport.send_data(&connection_info,
                                               sizeof(frame_data),
                                               frame_data,
                                               &tx_options,
                                               test_send_data_callback,
                                               &my_user_variable,
                                               parent_session_id));

  // Verify that our decapsulation worked
  TEST_ASSERT_EQUAL(2, last_received_connection_info.remote.endpoint_id);
  TEST_ASSERT_EQUAL(1, last_received_connection_info.local.endpoint_id);
  TEST_ASSERT_TRUE(last_received_tx_options.transport.valid_parent_session_id);
  TEST_ASSERT_EQUAL_PTR(parent_session_id,
                        last_received_tx_options.transport.parent_session_id);

  const uint8_t expected_frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03, 0x04, 0x05};
  TEST_ASSERT_EQUAL(sizeof(expected_frame_data), last_received_frame_length);
  TEST_ASSERT_EQUAL_INT8_ARRAY(expected_frame_data,
                               last_received_frame,
                               last_received_frame_length);

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);

  TEST_ASSERT_NOT_NULL(crc_16_on_send_complete);
  crc_16_on_send_complete(TRANSMIT_COMPLETE_OK,
                          NULL,
                          last_received_user_pointer);

  TEST_ASSERT_EQUAL(1, send_data_callback_counter);
}

// Test if the number of responses is set to 1 we can 
// still send the frame
void test_encapsulation_happy_case_with_number_of_response_1()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);

  zwave_controller_connection_info_t connection_info = {};
  zwave_tx_options_t tx_options                      = {};
  tx_options.number_of_responses                     = 1; 
  const uint8_t frame_data[]                         = {0x01, 0x02, 0x03};
  connection_info.remote.endpoint_id                 = 2;
  connection_info.remote.node_id                     = 5;
  connection_info.local.endpoint_id                  = 1;
  connection_info.local.node_id                      = 0;
  connection_info.encapsulation           = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  zwave_tx_session_id_t parent_session_id = (void *)23;

  zwave_tx_send_data_Stub(zwave_tx_send_data_stub);

  zwave_get_endpoint_node_ExpectAndReturn(connection_info.remote.node_id,
                                          connection_info.remote.endpoint_id,
                                          0);
  zwave_command_class_crc16_is_supported_ExpectAndReturn(0, true);
  zwave_command_class_crc16_clear_expect_crc16_response_Expect(
    connection_info.remote.node_id,
    connection_info.remote.endpoint_id);

  const uint8_t checksum_frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03};
  zwave_controller_crc16_ExpectAndReturn(CRC_INITAL_VALUE,
                                         checksum_frame_data,
                                         5,
                                         0x0405);

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    crc_16_transport.send_data(&connection_info,
                                               sizeof(frame_data),
                                               frame_data,
                                               &tx_options,
                                               test_send_data_callback,
                                               &my_user_variable,
                                               parent_session_id));

  // Verify that our decapsulation worked
  TEST_ASSERT_EQUAL(2, last_received_connection_info.remote.endpoint_id);
  TEST_ASSERT_EQUAL(1, last_received_connection_info.local.endpoint_id);
  TEST_ASSERT_TRUE(last_received_tx_options.transport.valid_parent_session_id);
  TEST_ASSERT_EQUAL_PTR(parent_session_id,
                        last_received_tx_options.transport.parent_session_id);

  const uint8_t expected_frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03, 0x04, 0x05};
  TEST_ASSERT_EQUAL(sizeof(expected_frame_data), last_received_frame_length);
  TEST_ASSERT_EQUAL_INT8_ARRAY(expected_frame_data,
                               last_received_frame,
                               last_received_frame_length);

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);

  TEST_ASSERT_NOT_NULL(crc_16_on_send_complete);
  crc_16_on_send_complete(TRANSMIT_COMPLETE_OK,
                          NULL,
                          last_received_user_pointer);

  TEST_ASSERT_EQUAL(1, send_data_callback_counter);
}


void test_encapsulation_overflow()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);

  zwave_controller_connection_info_t connection_info = {};
  connection_info.remote.endpoint_id                 = 2;
  connection_info.local.endpoint_id                  = 1;
  connection_info.encapsulation = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  const uint8_t frame_data[]    = {0x03, 0x04, 0x05};

  TEST_ASSERT_EQUAL(
    SL_STATUS_WOULD_OVERFLOW,
    crc_16_transport.send_data(&connection_info,
                               CRC_16_ENCAPSULATED_COMMAND_MAXIMUM_SIZE + 1,
                               frame_data,
                               NULL,
                               NULL,
                               NULL,
                               NULL));

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);
}

void test_encapsulation_no_crc16_support()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);

  zwave_controller_connection_info_t connection_info = {};
  connection_info.remote.endpoint_id                 = 2;
  connection_info.local.endpoint_id                  = 1;
  connection_info.encapsulation = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  const uint8_t frame_data[]    = {0x03, 0x04, 0x05};

  zwave_get_endpoint_node_ExpectAndReturn(connection_info.remote.node_id,
                                          connection_info.remote.endpoint_id,
                                          15);
  zwave_command_class_crc16_is_supported_ExpectAndReturn(15, false);

  TEST_ASSERT_EQUAL(SL_STATUS_NOT_SUPPORTED,
                    crc_16_transport.send_data(&connection_info,
                                               sizeof(frame_data),
                                               frame_data,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL));

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);
}

void test_encapsulation_response_expected_no_crc16()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);

  zwave_controller_connection_info_t connection_info = {};
  connection_info.remote.endpoint_id                 = 2;
  connection_info.local.endpoint_id                  = 1;
  connection_info.encapsulation = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  const uint8_t frame_data[]    = {0x03, 0x04, 0x05};

  zwave_get_endpoint_node_ExpectAndReturn(connection_info.remote.node_id,
                                          connection_info.remote.endpoint_id,
                                          15);
  zwave_command_class_crc16_is_supported_ExpectAndReturn(15, true);
  zwave_command_class_crc16_is_expecting_crc16_response_ExpectAndReturn(
    connection_info.remote.node_id,
    connection_info.remote.endpoint_id,
    false);

  zwave_tx_options_t tx_options = {};
  tx_options.number_of_responses = 0;  
  TEST_ASSERT_EQUAL(SL_STATUS_NOT_SUPPORTED,
                    crc_16_transport.send_data(&connection_info,
                                               sizeof(frame_data),
                                               frame_data,
                                               &tx_options,
                                               NULL,
                                               NULL,
                                               NULL));

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);
}

void test_encapsulation_already_encapsulated()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);

  zwave_controller_connection_info_t connection_info = {};
  connection_info.remote.endpoint_id                 = 2;
  connection_info.local.endpoint_id                  = 1;
  connection_info.encapsulation = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  const uint8_t frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x03, 0x04, 0x05};

  TEST_ASSERT_EQUAL(SL_STATUS_NOT_SUPPORTED,
                    crc_16_transport.send_data(&connection_info,
                                               sizeof(frame_data),
                                               frame_data,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL));

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);
}

void test_encapsulation_other_than_none()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);

  zwave_controller_connection_info_t connection_info = {};
  connection_info.remote.endpoint_id                 = 2;
  connection_info.local.endpoint_id                  = 1;
  const uint8_t frame_data[]                         = {0x03, 0x04, 0x05};
  const uint8_t tested_encapsulations[]
    = {ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_0,
       ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_ACCESS,
       ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_AUTHENTICATED,
       ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_UNAUTHENTICATED,
       ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME};

  for (size_t i = 0; i < sizeof(tested_encapsulations); i++) {
    connection_info.encapsulation
      = (zwave_controller_encapsulation_scheme_t)tested_encapsulations[i];
    TEST_ASSERT_EQUAL(SL_STATUS_NOT_SUPPORTED,
                      crc_16_transport.send_data(&connection_info,
                                                 sizeof(frame_data),
                                                 frame_data,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL));
  }

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);
}

void test_abort_send_data()
{
  TEST_ASSERT_NOT_NULL(crc_16_transport.send_data);
  TEST_ASSERT_NOT_NULL(crc_16_transport.abort_send_data);

  zwave_controller_connection_info_t connection_info = {};
  zwave_tx_options_t tx_options                      = {};
  const uint8_t frame_data[]                         = {0x01, 0x02, 0x03};
  zwave_tx_session_id_t parent_session_id            = (void *)23;
  connection_info.encapsulation = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;

  // Try to abort now, nothing will happen
  TEST_ASSERT_EQUAL(SL_STATUS_NOT_FOUND,
                    crc_16_transport.abort_send_data(parent_session_id));

  zwave_tx_send_data_Stub(zwave_tx_send_data_stub);

  zwave_get_endpoint_node_ExpectAndReturn(connection_info.remote.node_id,
                                          connection_info.remote.endpoint_id,
                                          0);
  zwave_command_class_crc16_is_supported_ExpectAndReturn(0, true);
  zwave_command_class_crc16_is_expecting_crc16_response_ExpectAndReturn(
    connection_info.remote.node_id,
    connection_info.remote.endpoint_id,
    true);
  zwave_command_class_crc16_clear_expect_crc16_response_Expect(
    connection_info.remote.node_id,
    connection_info.remote.endpoint_id);

  const uint8_t checksum_frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03};
  zwave_controller_crc16_ExpectAndReturn(CRC_INITAL_VALUE,
                                         checksum_frame_data,
                                         5,
                                         0x0405);

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    crc_16_transport.send_data(&connection_info,
                                               sizeof(frame_data),
                                               frame_data,
                                               &tx_options,
                                               test_send_data_callback,
                                               &my_user_variable,
                                               parent_session_id));

  // Verify that our decapsulation worked
  TEST_ASSERT_EQUAL(0, last_received_connection_info.remote.endpoint_id);
  TEST_ASSERT_EQUAL(0, last_received_connection_info.local.endpoint_id);
  TEST_ASSERT_TRUE(last_received_tx_options.transport.valid_parent_session_id);
  TEST_ASSERT_EQUAL_PTR(parent_session_id,
                        last_received_tx_options.transport.parent_session_id);
  const uint8_t expected_frame_data[]
    = {COMMAND_CLASS_CRC_16_ENCAP, CRC_16_ENCAP, 0x01, 0x02, 0x03, 0x04, 0x05};
  TEST_ASSERT_EQUAL(sizeof(expected_frame_data), last_received_frame_length);
  TEST_ASSERT_EQUAL_INT8_ARRAY(expected_frame_data,
                               last_received_frame,
                               last_received_frame_length);

  TEST_ASSERT_EQUAL(0, send_data_callback_counter);

  // Try to abort now, it will trigger a callback
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    crc_16_transport.abort_send_data(parent_session_id));

  TEST_ASSERT_EQUAL(1, send_data_callback_counter);
}
