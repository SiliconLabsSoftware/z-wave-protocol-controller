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
#include "unity.h"
#include "zwapi_protocol_controller.h"
#include "zwapi_func_ids.h"

// Session mock:
#include "zwapi_session_mock.h"
#include "zwapi_init_mock.h"
#include "zwapi_utils.h"

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp() {}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  return num_failures;
}

/// Called before each and every test
void setUp() {}

void test_zwapi_enable_node_nls(void)
{
  zwave_node_id_t node_id = 2;
  uint8_t response_buffer[]     = {0x04 /* length = len(payload) + 3 */,
                                   0x01 /* type: response */,
                                   FUNC_ID_ZW_ENABLE_NODE_NLS /* cmd */,
                                   0x01 /* payload */};
  uint8_t response_length       = 4;
  uint8_t payload_buffer[] = {0x02};
  uint8_t payload_buffer_length = 1;

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_ENABLE_NODE_NLS,
    payload_buffer,
    payload_buffer_length,
    response_buffer,
    &response_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer,
    response_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_length);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_enable_node_nls(node_id));
}

void test_zwapi_get_node_nls(void)
{
  zwave_node_id_t node_id = 2;
  uint8_t nls_enabled = 1;
  uint8_t response_buffer[]     = {0x04 /* length = len(payload) + 3 */,
                                   0x01 /* type: response */,
                                   FUNC_ID_ZW_GET_NODE_NLS_STATE /* cmd */,
                                   nls_enabled /* payload */};
  uint8_t response_length       = 4;
  uint8_t payload_buffer[] = {0x02};
  uint8_t payload_buffer_length = 1;
  uint8_t node_nls_state        = 99;

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_GET_NODE_NLS_STATE,
    payload_buffer,
    payload_buffer_length,
    response_buffer,
    &response_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer,
    response_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_length);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_get_node_nls(node_id, &node_nls_state));
  TEST_ASSERT_EQUAL(nls_enabled, node_nls_state);
}

void test_zwapi_transfer_protocol_cc(void)
{
  zwave_node_id_t node_id = 2;
  uint8_t decryption_key = 3;
  uint8_t tpcc_payload[] = {0xAA, 0xBB};
  uint8_t tpcc_payload_length   = 2;
  uint8_t response_buffer[]     = {0x04 /* length = len(payload) + 3 */,
                                   0x01 /* type: response */,
                                   FUNC_ID_ZW_TRANSFER_PROTOCOL_CC /* cmd */,
                                   0x01 /* payload */};
  uint8_t response_length       = 4;
  uint8_t payload_buffer[] = {0x02, 0x03, 0x02, 0xAA, 0xBB};
  uint8_t payload_buffer_length = 5;

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_TRANSFER_PROTOCOL_CC,
    payload_buffer,
    payload_buffer_length,
    response_buffer,
    &response_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer,
    response_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_length);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_transfer_protocol_cc(node_id, decryption_key, tpcc_payload_length, tpcc_payload));
}

void test_zwapi_request_protocol_cc_encryption_callback(void)
{
  zwave_node_id_t node_id = 2;
  zwapi_tx_report_t tx_report = {0};
  uint8_t session_id = 1;

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_request_protocol_cc_encryption_callback(node_id, &tx_report, session_id));
}
