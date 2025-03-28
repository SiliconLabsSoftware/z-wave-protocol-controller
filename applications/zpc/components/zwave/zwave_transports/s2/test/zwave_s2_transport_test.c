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
// Includes from the component being tested
#include "zwave_s2_transport.h"
#include "zwave_s2_internal.h"
#include "S2_external.h"  // To invoke S2_send_frame()
#include "zwave_s2_keystore_int.h"

// Mocks
#include "cmock.h"
#include "contiki_test_helper.h"
#include "process.h"
#include "S2_mock.h"
#include "zwapi_protocol_mem_mock.h"
#include "zwave_tx_mock.h"
#include "zwapi_protocol_controller_mock.h"
#include "zwave_network_management_mock.h"
#include "zwave_utils.h"

// ZPC Components
#include "zwave_controller_callbacks.h"
#include "s2_classcmd.h"
#include "ZW_classcmd.h"
#include "zwave_rx.h"
#include "attribute_store_fixt.h"
#include "datastore_fixt.h"
#include "zpc_attribute_store_type_registration.h"
#include "zpc_attribute_store_test_helper.h"
#include "zpc_attribute_store_network_helper.h"

// Unify components
#include "sl_status.h"
#include "sl_log.h"
#include "unify_dotdot_attribute_store.h"
#include "attribute_store_helper.h"

// Generic includes
#include <string.h>

CTR_DRBG_CTX s2_ctr_drbg;

static uint8_t my_status;
static zwapi_tx_report_t my_tx_info;
static void *my_user;
static int num_callbacks = 0;

static zwave_controller_connection_info_t my_connection_info;
static zwave_rx_receive_options_t my_rx_options;
static uint8_t my_frame_data[100];
static uint16_t my_frame_length;

extern const unify_dotdot_attribute_store_configuration_t zpc_configuration;

static void create_basic_network(void)
{
  const zwave_home_id_t home_id = 0xcafecafe;
  zwave_keyset_t granted_keys = 0xFF;

  unify_dotdot_attribute_store_set_configuration(&zpc_configuration);

  // Configure the UNID module to know our UNID.
  zwave_unid_set_home_id(home_id);

  // Make sure to start from scratch
  attribute_store_delete_node(attribute_store_get_root());
  // HomeID
  attribute_store_node_t home_id_node
    = attribute_store_add_node(ATTRIBUTE_HOME_ID, attribute_store_get_root());
  attribute_store_set_reported(home_id_node, &home_id, sizeof(zwave_home_id_t));

  attribute_store_node_t node_id_node;
  zwave_node_id_t node_id;

  node_id      = 1;  // ZPC
  node_id_node = attribute_store_add_node(ATTRIBUTE_NODE_ID, home_id_node);
  attribute_store_set_reported(node_id_node, &node_id, sizeof(node_id));

  node_id      = 3;  // Z-Wave node
  node_id_node = attribute_store_add_node(ATTRIBUTE_NODE_ID, home_id_node);
  attribute_store_set_reported(node_id_node, &node_id, sizeof(node_id));
  node_id_node = attribute_store_add_node(ATTRIBUTE_GRANTED_SECURITY_KEYS, node_id_node);
  zwave_set_node_granted_keys(node_id, &granted_keys);

  node_id      = 4;  // Z-Wave node
  node_id_node = attribute_store_add_node(ATTRIBUTE_NODE_ID, home_id_node);
  attribute_store_set_reported(node_id_node, &node_id, sizeof(node_id));
  node_id_node = attribute_store_add_node(ATTRIBUTE_GRANTED_SECURITY_KEYS, node_id_node);
  zwave_set_node_granted_keys(node_id, &granted_keys);
}

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp() {}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  return num_failures;
}

void setUp()
{
  num_callbacks = 0;
  contiki_test_helper_init();

  datastore_fixt_setup(":memory:");
  attribute_store_init();
  zpc_attribute_store_register_known_attribute_types();

  create_basic_network();
}

void tearDown()
{
  attribute_store_teardown();
  datastore_fixt_teardown();
}

// Used to catch the status codes
static void on_zwave_tx_send_data_complete(uint8_t status,
                                           const zwapi_tx_report_t *tx_info,
                                           void *user)
{
  my_status  = status;
  my_tx_info = *tx_info;
  my_user    = user;
  num_callbacks++;
}

void test_zwave_s2_send_data()
{
  zwave_controller_connection_info_t connection = {};

  uint8_t cmd_data[]            = "HelloWorld";
  zwave_tx_options_t tx_options = {};
  zwave_tx_session_id_t session;

  connection.encapsulation  = ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_ACCESS;
  connection.local.node_id  = 1;
  connection.remote.node_id = 2;

  s2_connection_t s2c = {};
  s2c.l_node          = 1;
  s2c.r_node          = 2;
  s2c.zw_tx_options   = 0;
  s2c.class_id        = 2;

  //get_protocol_ExpectAndReturn(s2c.r_node, PROTOCOL_ZWAVE);

  S2_send_data_singlecast_with_keyset_ExpectWithArrayAndReturn(
    0,
    0,
    &s2c,
    sizeof(s2_connection_t),
    UNKNOWN_KEYSET,
    cmd_data,
    sizeof(cmd_data),
    sizeof(cmd_data),
    1);

  //Test without verify delivery
  zwave_tx_get_number_of_responses_ExpectAndReturn(&session, 0);
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    zwave_s2_send_data(&connection,
                                       sizeof(cmd_data),
                                       cmd_data,
                                       &tx_options,
                                       on_zwave_tx_send_data_complete,
                                       (void *)0x42,
                                       &session));

  //Test with verify delivery
  zwave_tx_get_number_of_responses_ExpectAndReturn(&session, 1);
  s2c.tx_options = S2_TXOPTION_VERIFY_DELIVERY;
  S2_send_data_singlecast_with_keyset_ExpectWithArrayAndReturn(
    0,
    0,
    &s2c,
    sizeof(s2_connection_t),
    UNKNOWN_KEYSET,
    cmd_data,
    sizeof(cmd_data),
    sizeof(cmd_data),
    1);

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    zwave_s2_send_data(&connection,
                                       sizeof(cmd_data),
                                       cmd_data,
                                       &tx_options,
                                       on_zwave_tx_send_data_complete,
                                       (void *)0x42,
                                       &session));

  //Check the callback
  S2_send_done_event(0, S2_TRANSMIT_COMPLETE_VERIFIED);
  TEST_ASSERT_EQUAL_PTR((void *)0x42, my_user);
  TEST_ASSERT_EQUAL(1, num_callbacks);
  TEST_ASSERT_EQUAL(TRANSMIT_COMPLETE_VERIFIED, my_status);
}

void test_zwave_s2_send_protocol_data()
{
  zwave_controller_connection_info_t connection = {};

  uint8_t cmd_data[]            = "HelloWorld";
  zwave_tx_options_t tx_options = {};
  zwave_tx_session_id_t session;
  protocol_metadata_t metadata = {0};

  connection.encapsulation  = ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_ACCESS;
  connection.local.node_id  = 1;
  connection.remote.node_id = 2;

  tx_options.transport.is_protocol_frame = true;

  s2_connection_t s2c = {};
  s2c.l_node          = 1;
  s2c.r_node          = 2;
  s2c.zw_tx_options   = 0;
  s2c.class_id        = 2;
  s2c.tx_options      = S2_TXOPTION_VERIFY_DELIVERY;

  S2_send_data_singlecast_with_keyset_ExpectWithArrayAndReturn(
    0,
    0,
    &s2c,
    sizeof(s2_connection_t),
    UNKNOWN_KEYSET,
    cmd_data,
    sizeof(cmd_data),
    sizeof(cmd_data),
    1);

  zwave_tx_get_number_of_responses_ExpectAndReturn(&session, 0);
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    zwave_s2_send_data(&connection,
                                       sizeof(cmd_data),
                                       cmd_data,
                                       &tx_options,
                                       on_zwave_tx_send_data_complete,
                                       (void *)&metadata,
                                       &session));

  //Check the callback
  S2_send_done_event(0, S2_TRANSMIT_COMPLETE_VERIFIED);
  TEST_ASSERT_EQUAL_PTR((void *)&metadata, my_user);
  TEST_ASSERT_EQUAL(1, num_callbacks);
  TEST_ASSERT_EQUAL(TRANSMIT_COMPLETE_VERIFIED, my_status);
}

void test_zwave_s2_send_data_nonce_report_sos()
{
  uint8_t nonce_report[]
    = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NONCE_REPORT, 0x23, 0x01, 0x00};
  const s2_connection_t s2_connection = {.l_node = 3, .r_node = 5};

  // Expected zwave_tx_send_data parameters
  zwave_controller_connection_info_t expected_info = {};
  zwave_tx_options_t expected_options              = {};

  expected_info.encapsulation  = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  expected_info.local.node_id  = s2_connection.l_node;
  expected_info.remote.node_id = s2_connection.r_node;

  expected_options.qos_priority
    = ZWAVE_TX_QOS_RECOMMENDED_TIMING_CRITICAL_PRIORITY;
  expected_options.number_of_responses = 1;
  // SOS/MOS flags triggers an ignore of the incoming frames back-off
  expected_options.transport.ignore_incoming_frames_back_off = true;

  zwave_tx_send_data_ExpectAndReturn(&expected_info,
                                     sizeof(nonce_report),
                                     nonce_report,
                                     &expected_options,
                                     NULL,
                                     0,
                                     0,
                                     SL_STATUS_OK);
  zwave_tx_send_data_IgnoreArg_on_send_complete();

  // Send from S2, will trigger a call to zwave_tx_send_data
  TEST_ASSERT_EQUAL(
    1,
    S2_send_frame(NULL, &s2_connection, nonce_report, sizeof(nonce_report)));
}

static sl_status_t zwapi_memory_get_buffer_stub(uint16_t offset,
                                                uint8_t *buf,
                                                uint8_t length,
                                                int ncalls)
{
  static nvm_config_t nvm
    = {.magic           = NVM_MAGIC,
       .security_netkey = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
       .assigned_keys   = 0x87,
       .security2_key   = {{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
                           {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
                           {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}},
       .ecdh_priv_key
       = {0xab, 0xe,  0xab, 0xe,  0xab, 0xe,  0xab, 0xe,  0xab, 0xe,  0xab,
          0xe,  0xab, 0xe,  0xab, 0xe,  0xab, 0xe,  0xab, 0xe,  0xab, 0xe,
          0xab, 0xe,  0xab, 0xe,  0xab, 0xe,  0xab, 0xe,  0xab, 0xe}};
  const uint8_t *ptr = (const uint8_t *)&nvm;
  memcpy(buf, ptr + offset, length);
  return SL_STATUS_OK;
}

void test_zwave_s2_set_secure_nif()
{
  uint8_t nif[] = "HelloWorld";
  uint8_t *test_nif;
  uint8_t test_nif_length;

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwave_s2_set_secure_nif(nif, sizeof(nif)));

  zwapi_memory_get_buffer_Stub(zwapi_memory_get_buffer_stub);
  S2_get_commands_supported(1,
                            2,
                            (const uint8_t **)&test_nif,
                            &test_nif_length);
  TEST_ASSERT_EQUAL(sizeof(nif), test_nif_length);
  TEST_ASSERT_EQUAL_CHAR_ARRAY(nif, test_nif, test_nif_length);
}

void test_zwave_s2_on_frame_received()
{
  zwave_controller_connection_info_t connection = {};

  uint8_t cmd_data[]                    = "\x9fHelloWorld";
  zwave_rx_receive_options_t rx_options = {};
  rx_options.rssi                       = 0x31;
  connection.encapsulation              = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  connection.local.node_id              = 1;
  connection.remote.node_id             = 2;

  s2_connection_t s2c = {};
  s2c.l_node          = 1;
  s2c.r_node          = 2;
  s2c.zw_rx_RSSIval   = 0x31;

  S2_application_command_handler_ExpectWithArray(0,
                                                 0,
                                                 &s2c,
                                                 sizeof(s2_connection_t),
                                                 cmd_data,
                                                 sizeof(cmd_data),
                                                 sizeof(cmd_data));
  zwave_s2_on_frame_received(&connection,
                             &rx_options,
                             cmd_data,
                             sizeof(cmd_data));
}

void my_on_frame_received(
  const zwave_controller_connection_info_t *connection_info,
  const zwave_rx_receive_options_t *rx_options,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  my_connection_info = *connection_info;
  my_rx_options      = *rx_options;
  memcpy(my_frame_data, frame_data, frame_length);
  my_frame_length = frame_length;
}

void test_S2_msg_received_event()
{
  static zwave_controller_callbacks_t cb
    = {.on_application_frame_received = my_on_frame_received};
  uint8_t cmd_data[] = "HelloWorld";

  s2_connection_t s2c = {};
  s2c.l_node          = 1;
  s2c.r_node          = 2;
  s2c.class_id        = 2;
  s2c.zw_rx_RSSIval   = 0x31;

  zwave_controller_register_callbacks(&cb);

  S2_msg_received_event(0, &s2c, cmd_data, sizeof(cmd_data));

  TEST_ASSERT_EQUAL(1, my_connection_info.local.node_id);
  TEST_ASSERT_EQUAL(2, my_connection_info.remote.node_id);
  TEST_ASSERT_EQUAL(ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_ACCESS,
                    my_connection_info.encapsulation);
  TEST_ASSERT_EQUAL(sizeof(cmd_data), my_frame_length);
  TEST_ASSERT_EQUAL_CHAR_ARRAY(cmd_data, my_frame_data, sizeof(cmd_data));
}

void test_zwave_s2_abort_send_data()
{
  //Setup S2 frame transmission
  zwave_controller_connection_info_t connection = {};
  uint8_t cmd_data[]                            = "HelloWorld";
  zwave_tx_options_t tx_options                 = {};
  zwave_tx_session_id_t session                 = (void *)23;
  connection.encapsulation  = ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_ACCESS;
  connection.local.node_id  = 1;
  connection.remote.node_id = 2;
  s2_connection_t s2c       = {};
  s2c.l_node                = 1;
  s2c.r_node                = 2;
  s2c.zw_tx_options         = 0;
  s2c.class_id              = 2;

  // Aborting before triggering a tranmission
  TEST_ASSERT_EQUAL(SL_STATUS_NOT_FOUND, zwave_s2_abort_send_data(session));

  //get_protocol_ExpectAndReturn(s2c.r_node, PROTOCOL_ZWAVE);
  S2_send_data_singlecast_with_keyset_ExpectWithArrayAndReturn(
    0,
    0,
    &s2c,
    sizeof(s2_connection_t),
    UNKNOWN_KEYSET,
    cmd_data,
    sizeof(cmd_data),
    sizeof(cmd_data),
    1);

  //Test without verify delivery
  zwave_tx_get_number_of_responses_ExpectAndReturn(session, 0);
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    zwave_s2_send_data(&connection,
                                       sizeof(cmd_data),
                                       cmd_data,
                                       &tx_options,
                                       on_zwave_tx_send_data_complete,
                                       (void *)0x42,
                                       session));

  S2_send_frame_done_notify_Ignore();
  //Abort the tranmission
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zwave_s2_abort_send_data(session));
}

void test_zwave_s2_nls_node_list_get_test_request_0_node_exists_last_node()
{
  zwave_nodemask_t node_list = {0};
  uint16_t node_list_length  = 0;
  int8_t status;

  // Variables for test input
  node_t src_node;
  bool request;

  // Variables to be filled by the function being tested
  bool is_last_node;
  uint16_t node_id;
  uint8_t granted_keys;
  bool nls_state;

  // Set expected values
  node_list[0]     = 0x08;  // Node 4 is NLS enabled
  node_list_length = 1;

  zwapi_get_nls_nodes_ExpectAndReturn(NULL, NULL, SL_STATUS_OK);
  zwapi_get_nls_nodes_IgnoreArg_list_length();
  zwapi_get_nls_nodes_IgnoreArg_node_list();
  zwapi_get_nls_nodes_ReturnThruPtr_list_length(&node_list_length);
  zwapi_get_nls_nodes_ReturnArrayThruPtr_node_list(node_list,
                                                   sizeof(node_list));

  src_node = 1;
  request  = false;  // Request 1st node
  status   = S2_get_nls_node_list(src_node,
                                request,
                                &is_last_node,
                                &node_id,
                                &granted_keys,
                                &nls_state);

  TEST_ASSERT_EQUAL(0, status);
  TEST_ASSERT_EQUAL(true, is_last_node);
  TEST_ASSERT_EQUAL(4, node_id);
  TEST_ASSERT_EQUAL(0xFF, granted_keys);
  TEST_ASSERT_EQUAL(true, nls_state);
}

void test_zwave_s2_nls_node_list_get_test_request_0_node_exists_not_last_node()
{
  zwave_nodemask_t node_list = {0};
  uint16_t node_list_length  = 0;
  int8_t status;

  // Variables for test input
  node_t src_node;
  bool request;

  // Variables to be filled by the function being tested
  bool is_last_node;
  uint16_t node_id;
  uint8_t granted_keys;
  bool nls_state;

  // Set expected values
  node_list[0]     = 0x0C;  // Node 3 and 4 are NLS enabled
  node_list_length = 1;

  zwapi_get_nls_nodes_ExpectAndReturn(NULL, NULL, SL_STATUS_OK);
  zwapi_get_nls_nodes_IgnoreArg_list_length();
  zwapi_get_nls_nodes_IgnoreArg_node_list();
  zwapi_get_nls_nodes_ReturnThruPtr_list_length(&node_list_length);
  zwapi_get_nls_nodes_ReturnArrayThruPtr_node_list(node_list,
                                                   sizeof(node_list));

  src_node = 1;
  request  = false;  // Request 1st node
  status   = S2_get_nls_node_list(src_node,
                                request,
                                &is_last_node,
                                &node_id,
                                &granted_keys,
                                &nls_state);

  TEST_ASSERT_EQUAL(0, status);
  TEST_ASSERT_EQUAL(false, is_last_node);
  TEST_ASSERT_EQUAL(3, node_id);
  TEST_ASSERT_EQUAL(0xFF, granted_keys);
  TEST_ASSERT_EQUAL(true, nls_state);

  src_node = 1;
  request  = true;  // Request 2nd node
  status   = S2_get_nls_node_list(src_node,
                                request,
                                &is_last_node,
                                &node_id,
                                &granted_keys,
                                &nls_state);

  TEST_ASSERT_EQUAL(0, status);
  TEST_ASSERT_EQUAL(true, is_last_node);
  TEST_ASSERT_EQUAL(4, node_id);
  TEST_ASSERT_EQUAL(0xFF, granted_keys);
  TEST_ASSERT_EQUAL(true, nls_state);

  src_node = 1;
  request  = true;  // Request a non-existant node
  status   = S2_get_nls_node_list(src_node,
                                request,
                                &is_last_node,
                                &node_id,
                                &granted_keys,
                                &nls_state);

  TEST_ASSERT_EQUAL(-1, status);
  TEST_ASSERT_EQUAL(false, nls_state);
}

void test_zwave_s2_nls_node_list_get_test_request_0_node_does_not_exist()
{
  zwave_nodemask_t node_list = {0};
  uint16_t node_list_length  = 0;
  int8_t status;

  // Variables for test input
  node_t src_node;
  bool request;

  // Variables to be filled by the function being tested
  bool is_last_node;
  uint16_t node_id;
  uint8_t granted_keys;
  bool nls_state;

  // Set expected values
  node_list[0]     = 0x00;
  node_list_length = 1;

  zwapi_get_nls_nodes_ExpectAndReturn(NULL, NULL, SL_STATUS_OK);
  zwapi_get_nls_nodes_IgnoreArg_list_length();
  zwapi_get_nls_nodes_IgnoreArg_node_list();
  zwapi_get_nls_nodes_ReturnThruPtr_list_length(&node_list_length);
  zwapi_get_nls_nodes_ReturnArrayThruPtr_node_list(node_list,
                                                   sizeof(node_list));

  src_node = 1;
  request  = false;  // Request 1st node
  status   = S2_get_nls_node_list(src_node,
                                request,
                                &is_last_node,
                                &node_id,
                                &granted_keys,
                                &nls_state);

  TEST_ASSERT_EQUAL(-1, status);
  TEST_ASSERT_EQUAL(false, nls_state);
}

void test_zwave_s2_nls_node_list_report_test_happy_case()
{
  int8_t status;

  // Variables for test input
  node_t src_node;
  uint16_t node_id;
  uint8_t granted_keys;
  bool nls_state;

  src_node = 1;
  node_id = 3;
  granted_keys = 0xFF;
  nls_state = true;

  zwapi_enable_node_nls_ExpectAndReturn(node_id, SL_STATUS_OK);

  status = S2_notify_nls_node_list_report(src_node, node_id, granted_keys, nls_state);

  TEST_ASSERT_EQUAL(0, status);
}

void test_zwave_s2_nls_node_list_report_test_node_does_not_exist()
{
  int8_t status;

  // Variables for test input
  node_t src_node;
  uint16_t node_id;
  uint8_t granted_keys;
  bool nls_state;

  src_node = 1;
  node_id = 6; // Non existant node
  granted_keys = 0xFF;
  nls_state = true;

  zwapi_enable_node_nls_ExpectAndReturn(node_id, SL_STATUS_OK);

  status = S2_notify_nls_node_list_report(src_node, node_id, granted_keys, nls_state);

  TEST_ASSERT_EQUAL(-1, status);
}

void test_zwave_s2_nls_node_list_report_test_nls_state_not_set()
{
  int8_t status;

  // Variables for test input
  node_t src_node;
  uint16_t node_id;
  uint8_t granted_keys;
  bool nls_state;

  src_node = 1;
  node_id = 3; // Non existant node
  granted_keys = 0xFF;
  nls_state = false;

  status = S2_notify_nls_node_list_report(src_node, node_id, granted_keys, nls_state);

  TEST_ASSERT_EQUAL(-1, status);
}
