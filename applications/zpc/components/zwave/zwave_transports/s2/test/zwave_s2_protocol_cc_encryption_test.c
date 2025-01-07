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
#include <string.h>

#include "contiki_test_helper.h"

#include "zwave_s2_protocol_cc_encryption.h"

#include "zwave_controller_connection_info.h"

#include "zwave_tx_mock.h"

#include "sl_log.h"
#include "sl_status.h"

#include "unity.h"

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp() {}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  return num_failures;
}

void setUp()
{
  contiki_test_helper_init();
}

void tearDown() {}

void test_zwave_s2_on_protocol_cc_encryption_request_happy_case()
{
  zwave_node_id_t destination_node_id = 2;
  uint8_t payload[]                   = {0x01, 0x02, 0x03};
  uint8_t payload_length              = sizeof(payload);
  uint8_t protocol_metadata[]         = {0xA1, 0xA2, 0xA3};
  uint8_t protocol_metadata_length    = sizeof(protocol_metadata);
  uint8_t use_supervision             = 1;
  uint8_t session_id                  = 1;
  zwave_controller_connection_info_t connection_info = {0};
  zwave_tx_options_t tx_options                      = {0};
  zwave_tx_session_id_t tx_session_id                = NULL;
  protocol_metadata_t metadata                       = {0};

  // Expected TX options
  tx_options.transport.is_protocol_frame = true;
  tx_options.number_of_responses         = 1;
  tx_options.discard_timeout_ms          = 5000;
  tx_options.qos_priority                = ZWAVE_TX_QOS_MAX_PRIORITY;

  // Expected connection info
  connection_info.encapsulation  = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  connection_info.local.node_id  = 0;
  connection_info.remote.node_id = 2;

  // Expected protocol metadata
  metadata.session_id  = session_id;
  metadata.data_length = protocol_metadata_length;
  memcpy(metadata.data, protocol_metadata, protocol_metadata_length);

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

  zwave_s2_on_protocol_cc_encryption_request(destination_node_id,
                                             payload_length,
                                             payload,
                                             protocol_metadata_length,
                                             protocol_metadata,
                                             use_supervision,
                                             session_id);
}
