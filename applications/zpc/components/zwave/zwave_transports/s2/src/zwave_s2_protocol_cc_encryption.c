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
#include "zwave_controller_transport.h"
#include "zwave_controller_transport_internal.h"
#include "sl_log.h"
#include "zwave_controller.h"
#include "zwave_s2_internal.h"
#include "zwave_s2_transport.h"
#include "zwave_s2_protocol_cc_encryption.h"
#include "zwave_tx_scheme_selector.h"

#define LOG_TAG "zwave_s2_protocol_cc_encryption"

protocol_metadata_t metadata = {0};

static void on_send_protocol_data_callback_received(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
{
  protocol_metadata_t *metadata = (protocol_metadata_t *)user;

  if (status == TRANSMIT_COMPLETE_FAIL || status == TRANSMIT_COMPLETE_VERIFIED) {
    zwave_controller_request_protocol_cc_encryption_callback(status, tx_info, metadata->session_id);
  } else {
    sl_log_debug(LOG_TAG, "Send Protocol Data callback, status: %d", status);
  }
}

void zwave_s2_on_protocol_cc_encryption_request(
  const zwave_node_id_t destination_node_id,
  const uint8_t payload_length,
  const uint8_t *const payload,
  const uint8_t protocol_metadata_length,
  const uint8_t *const protocol_metadata,
  const uint8_t use_supervision,
  const uint8_t session_id)
{
  zwave_controller_connection_info_t connection_info  = {0};
  zwave_tx_options_t tx_options                       = {0};
  uint8_t number_of_expected_responses                = 1;
  uint32_t discard_timeout_ms                         = 5000;
  sl_status_t ret                                     = SL_STATUS_OK;
  zwave_tx_session_id_t tx_session_id                 = NULL;

  zwave_tx_scheme_get_node_connection_info(destination_node_id, 0, &connection_info);
  zwave_tx_scheme_get_node_tx_options(
    ZWAVE_TX_QOS_MAX_PRIORITY,
    number_of_expected_responses,
    discard_timeout_ms,
    &tx_options);

  // Other TX options are set in the transport layer in `S2_send_frame`
  tx_options.transport.is_protocol_frame = true;

  metadata.session_id  = session_id;
  metadata.data_length = protocol_metadata_length;
  memcpy(metadata.data, protocol_metadata, protocol_metadata_length);

  // Following command will trigger a "Send Protocol Data" command and the callback
  // provided to this function will be triggered when the callback data frame of
  // the "Send Protocol Data" command is received.
  ret = zwave_tx_send_data(&connection_info,
                           payload_length,
                           payload,
                           &tx_options,
                           on_send_protocol_data_callback_received,
                           (void *)&metadata,
                           &tx_session_id);
  if (ret != SL_STATUS_OK) {
    sl_log_error(LOG_TAG, "Unable to send S2 data. Error code %d.", ret);
  }
}
