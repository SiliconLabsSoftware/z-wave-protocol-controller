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

#include <string.h>

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
  zwave_node_id_t node_id       = 2;
  uint8_t response_buffer[]     = {0x04 /* length = len(payload) + 3 */,
                                   0x01 /* type: response */,
                                   FUNC_ID_ZW_ENABLE_NODE_NLS /* cmd */,
                                   0x01 /* payload */};
  uint8_t response_length       = 4;
  uint8_t payload_buffer[]      = {0x02};
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
  zwave_node_id_t node_id       = 2;
  uint8_t nls_supported         = 1;
  uint8_t nls_enabled           = 1;
  uint8_t response_buffer[]     = {0x04 /* length = len(payload) + 3 */,
                                   0x01 /* type: response */,
                                   FUNC_ID_ZW_GET_NODE_NLS_STATE /* cmd */,
                                   nls_supported,
                                   nls_enabled /* payload */};
  uint8_t response_length       = 5;
  uint8_t payload_buffer[]      = {0x02};
  uint8_t payload_buffer_length = 1;
  uint8_t node_nls_state        = 99;
  uint8_t node_nls_support      = 99;

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

  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    zwapi_get_node_nls(node_id, &node_nls_state, &node_nls_support));
  TEST_ASSERT_EQUAL(nls_enabled, node_nls_state);
}

void test_zwapi_get_nls_nodes_frame_0_reduced(void)
{
  // clang-format off
  uint8_t response_buffer[3 + 6] = {0x00};        /* frame 0 */
  memset(response_buffer, 0, sizeof(response_buffer));
  response_buffer[0] = 0x09;                      /* length = len(payload) + 3 */
  response_buffer[1] = 0x01;                      /* type: response */
  response_buffer[2] = FUNC_ID_ZW_GET_NLS_NODES;  /* cmd */
  response_buffer[3] = 0x00;                      /* more nodes flag: no */
  response_buffer[4] = 0x00;                      /* start offset */
  response_buffer[5] = 0x03;                      /* list length */
  response_buffer[6] = 0xF2;                      /* nodes 2, 5, 6, 7, 8 */
  response_buffer[7] = 0x01;                      /* nodes 9 */
  response_buffer[8] = 0x80;                      /* nodes 24 */
  uint8_t response_buffer_length = sizeof(response_buffer);
  uint8_t request_buffer[]       = {0x00 /* start offset */};
  uint8_t request_buffer_length  = sizeof(request_buffer);
  uint8_t expected_list_length   = response_buffer[5];
  // clang-format on

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_GET_NLS_NODES,
    request_buffer,
    request_buffer_length,
    response_buffer,
    &response_buffer_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer,
    response_buffer_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_buffer_length);

  uint16_t list_length = 0;
  zwave_nodemask_t node_list;
  memset(&node_list, 0, sizeof(zwave_nodemask_t));
  // verify API call
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_get_nls_nodes(&list_length, node_list));
  // verify list length
  TEST_ASSERT_EQUAL(expected_list_length, list_length);
  // verify NLS enabled nodes
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(2, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(5, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(6, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(7, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(8, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(9, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(24, node_list));
}

void test_zwapi_get_nls_nodes_frame_0_full(void)
{
  // clang-format off
  uint8_t response_buffer[128 + 6] = {0x00};            /* frame 0 */
  memset(response_buffer, 0, sizeof(response_buffer));
  response_buffer[0]       = 0x86;                      /* length = len(payload) + 3 */
  response_buffer[1]       = 0x01;                      /* type: response */
  response_buffer[2]       = FUNC_ID_ZW_GET_NLS_NODES;  /* cmd */
  response_buffer[3]       = 0x00;                      /* more nodes flag: no */
  response_buffer[4]       = 0x00;                      /* start offset */
  response_buffer[5]       = 0x80;                      /* list length */
  response_buffer[125 + 6] = 0x01;                      /* nodes 1024 */
  response_buffer[127 + 6] = 0x80;                      /* nodes 1047 (last node ID of the frame 0) */
  uint8_t response_buffer_length = sizeof(response_buffer);
  uint8_t request_buffer[]       = {0x00 /* start offset */};
  uint8_t request_buffer_length  = sizeof(request_buffer);
  uint8_t expected_list_length   = response_buffer[5];
  // clang-format on

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_GET_NLS_NODES,
    request_buffer,
    request_buffer_length,
    response_buffer,
    &response_buffer_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer,
    response_buffer_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_buffer_length);

  uint16_t list_length = 0;
  zwave_nodemask_t node_list;
  memset(&node_list, 0, sizeof(zwave_nodemask_t));
  // verify API call
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_get_nls_nodes(&list_length, node_list));
  // verify list length
  TEST_ASSERT_EQUAL(expected_list_length, list_length);
  // verify NLS enabled nodes
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1024, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1047, node_list));
}

void test_zwapi_get_nls_nodes_frame_0_full_frame_1_reduced(void)
{
  // clang-format off
  uint8_t response_buffer1[128 + 6] = {0x00};           /* frame 0 */
  uint8_t response_buffer2[29 + 6]  = {0x00};           /* frame 1 */
  memset(response_buffer1, 0, sizeof(response_buffer1));
  memset(response_buffer2, 0, sizeof(response_buffer2));
  response_buffer1[0]       = 0x86;                     /* length = len(payload) + 3 */
  response_buffer1[1]       = 0x01;                     /* type: response */
  response_buffer1[2]       = FUNC_ID_ZW_GET_NLS_NODES; /* cmd */
  response_buffer1[3]       = 0x80;                     /* more nodes flag: yes */
  response_buffer1[4]       = 0x00;                     /* start offset */
  response_buffer1[5]       = 0x80;                     /* list length */
  response_buffer1[29 + 6]  = 0x01;                     /* nodes 256 (first LR node ID) */
  response_buffer1[127 + 6] = 0x80;                     /* nodes 1047 (last node ID of the frame 0) */
  response_buffer2[0]       = 0x22;                     /* length = len(payload) + 6 */
  response_buffer2[1]       = 0x01;                     /* type: response */
  response_buffer2[2]       = FUNC_ID_ZW_GET_NLS_NODES; /* cmd */
  response_buffer2[3]       = 0x00;                     /* more nodes flag: no */
  response_buffer2[4]       = 0x01;                     /* start offset */
  response_buffer2[5]       = 0x1C;                     /* list length */
  response_buffer2[0 + 6]   = 0x01;                     /* nodes 1048 (first node ID of the frame 1) */
  response_buffer2[27 + 6]  = 0x80;                     /* nodes 1271 */
  uint8_t response_buffer1_length = sizeof(response_buffer1);
  uint8_t response_buffer2_length = sizeof(response_buffer2);
  uint8_t request_buffer1[]       = {0x00 /* start offset */};
  uint8_t request_buffer1_length  = sizeof(request_buffer1);
  uint8_t request_buffer2[]       = {0x01 /* start offset */};
  uint8_t request_buffer2_length  = sizeof(request_buffer2);
  uint8_t expected_list_length    = response_buffer1[5] + response_buffer2[5];
  // clang-format on

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_GET_NLS_NODES,
    request_buffer1,
    request_buffer1_length,
    response_buffer1,
    &response_buffer1_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer1,
    response_buffer1_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_buffer1_length);

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_GET_NLS_NODES,
    request_buffer2,
    request_buffer2_length,
    response_buffer2,
    &response_buffer2_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer2,
    response_buffer2_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_buffer2_length);

  uint16_t list_length = 0;
  zwave_nodemask_t node_list;
  memset(&node_list, 0, sizeof(zwave_nodemask_t));
  // verify API call
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_get_nls_nodes(&list_length, node_list));
  // verify list length
  TEST_ASSERT_EQUAL(expected_list_length, list_length);
  // verify NLS enabled nodes
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(256, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1047, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1048, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1271, node_list));
}

void test_zwapi_get_nls_nodes_frame_0_full_frame_1_full(void)
{
  // clang-format off
  uint8_t response_buffer1[128 + 6] = {0x00};           /* frame 0 */
  uint8_t response_buffer2[29 + 6]  = {0x00};           /* frame 1 */
  memset(response_buffer1, 0, sizeof(response_buffer1));
  memset(response_buffer2, 0, sizeof(response_buffer2));
  response_buffer1[0]       = 0x86;                     /* length = len(payload) + 3 */
  response_buffer1[1]       = 0x01;                     /* type: response */
  response_buffer1[2]       = FUNC_ID_ZW_GET_NLS_NODES; /* cmd */
  response_buffer1[3]       = 0x80;                     /* more nodes flag: yes */
  response_buffer1[4]       = 0x00;                     /* start offset */
  response_buffer1[5]       = 0x80;                     /* list length */
  response_buffer1[29 + 6]  = 0x01;                     /* nodes 256 (first LR node ID) */
  response_buffer1[127 + 6] = 0x80;                     /* nodes 1047 (last node ID of the frame 0) */
  response_buffer2[0]       = 0x23;                     /* length = len(payload) + 6 */
  response_buffer2[1]       = 0x01;                     /* type: response */
  response_buffer2[2]       = FUNC_ID_ZW_GET_NLS_NODES; /* cmd */
  response_buffer2[3]       = 0x00;                     /* more nodes flag: no */
  response_buffer2[4]       = 0x01;                     /* start offset */
  response_buffer2[5]       = 0x1D;                     /* list length */
  response_buffer2[0 + 6]   = 0x01;                     /* nodes 1048 (first node ID of the frame 1) */
  response_buffer2[28 + 6]  = 0x80;                     /* nodes 1279 (last LR node ID) */
  uint8_t response_buffer1_length = sizeof(response_buffer1);
  uint8_t response_buffer2_length = sizeof(response_buffer2);
  uint8_t request_buffer1[]       = {0x00 /* start offset */};
  uint8_t request_buffer1_length  = sizeof(request_buffer1);
  uint8_t request_buffer2[]       = {0x01 /* start offset */};
  uint8_t request_buffer2_length  = sizeof(request_buffer2);
  uint8_t expected_list_length    = response_buffer1[5] + response_buffer2[5];
  // clang-format on

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_GET_NLS_NODES,
    request_buffer1,
    request_buffer1_length,
    response_buffer1,
    &response_buffer1_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer1,
    response_buffer1_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_buffer1_length);

  zwapi_session_send_frame_with_response_ExpectAndReturn(
    FUNC_ID_ZW_GET_NLS_NODES,
    request_buffer2,
    request_buffer2_length,
    response_buffer2,
    &response_buffer2_length,
    SL_STATUS_OK);
  zwapi_session_send_frame_with_response_IgnoreArg_response_buf();
  zwapi_session_send_frame_with_response_IgnoreArg_response_len();
  zwapi_session_send_frame_with_response_ReturnMemThruPtr_response_buf(
    response_buffer2,
    response_buffer2_length);
  zwapi_session_send_frame_with_response_ReturnThruPtr_response_len(
    &response_buffer2_length);

  uint16_t list_length = 0;
  zwave_nodemask_t node_list;
  memset(&node_list, 0, sizeof(zwave_nodemask_t));
  // verify API call
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwapi_get_nls_nodes(&list_length, node_list));
  // verify list length
  TEST_ASSERT_EQUAL(expected_list_length, list_length);
  // verify NLS enabled nodes
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(256, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1047, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1048, node_list));
  TEST_ASSERT_EQUAL(1, ZW_IS_NODE_IN_MASK(1279, node_list));
}

void test_zwapi_transfer_protocol_cc(void)
{
  zwave_node_id_t node_id       = 2;
  uint8_t decryption_key        = 3;
  uint8_t tpcc_payload[]        = {0xAA, 0xBB};
  uint8_t tpcc_payload_length   = 2;
  uint8_t response_buffer[]     = {0x04 /* length = len(payload) + 3 */,
                                   0x01 /* type: response */,
                                   FUNC_ID_ZW_TRANSFER_PROTOCOL_CC /* cmd */,
                                   0x01 /* payload */};
  uint8_t response_length       = 4;
  uint8_t payload_buffer[]      = {0x02, 0x03, 0x02, 0xAA, 0xBB};
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

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    zwapi_transfer_protocol_cc(node_id,
                                               decryption_key,
                                               tpcc_payload_length,
                                               tpcc_payload));
}

void test_zwapi_request_protocol_cc_encryption_callback(void)
{
  zwave_node_id_t node_id     = 2;
  zwapi_tx_report_t tx_report = {0};
  uint8_t session_id          = 1;

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    zwapi_request_protocol_cc_encryption_callback(node_id,
                                                                  &tx_report,
                                                                  session_id));
}
