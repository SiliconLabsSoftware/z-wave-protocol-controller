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

// Attribute store helpers
#include "attribute_store_defined_attribute_types.h"
#include "attribute_store_helper.h"

// Includes from other components
#include "sl_log.h"
#include "attribute_store.h"
#include "attribute_resolver.h"
#include "zwave_unid.h"
#include "ZW_classcmd.h"
#include "zwave_tx.h"
#include "zwave_controller_keyset.h"
#include "zwave_controller_utils.h"
#include "zwave_utils.h"
#include "zwave_command_handler.h"
#include "zwave_command_class_indices.h"
#include "zwapi_protocol_controller.h"
#include "zwave_command_class_protocol.h"
#include "zwave_command_class_supervision.h"
#include "zwave_tx_scheme_selector.h"

// Generic includes
#include "assert.h"
#include "string.h"

// Log tag
#define LOG_TAG "zwave_command_class_protocol"

protocol_metadata_t metadata = {0};

static zwave_controller_callbacks_t zwave_command_class_protocol_callbacks = {
  .on_protocol_cc_encryption_request = zwave_on_protocol_cc_encryption_request
};

static void on_send_protocol_data_callback_received(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
{
  protocol_metadata_t *metadata = (protocol_metadata_t *)user;

  if (status == TRANSMIT_COMPLETE_FAIL || status == TRANSMIT_COMPLETE_VERIFIED) {
    zwave_controller_request_protocol_cc_encryption_callback(status, tx_info, metadata->session_id);
  } else {
    sl_log_debug(LOG_TAG, "Send Protocol Data callback, status: %d", status);
  }
}

void zwave_on_protocol_cc_encryption_request(
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
  // sl_status_t ret                                     = SL_STATUS_OK;
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

  if (use_supervision)
  {
    zwave_command_class_supervision_send_data(
      &connection_info,
      payload_length,
      payload,
      &tx_options,
      &on_send_protocol_data_callback_received,
      (void *)&metadata,
      &tx_session_id);
  } else {
    zwave_tx_send_data(
      &connection_info,
      payload_length,
      payload,
      &tx_options,
      &on_send_protocol_data_callback_received,
      (void *)&metadata,
      &tx_session_id);
  }
}

sl_status_t zwave_command_class_protocol_support_handler(
  const zwave_controller_connection_info_t *connection,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  // Frame too short, it should have not come here.
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  sl_log_info(LOG_TAG, "Protocol command received from NodeID %d:%d",
               connection->remote.node_id, connection->remote.endpoint_id);

  sl_status_t status = 
  zwapi_transfer_protocol_cc(
    connection->remote.node_id,
    zwave_controller_get_key_from_encapsulation(connection->encapsulation),
    frame_length,
    frame_data);

  switch (status) {
    case SL_STATUS_OK:
      sl_log_info(LOG_TAG,
                   "Command from NodeID %d:%d was handled successfully.",
                   connection->remote.node_id,
                   connection->remote.endpoint_id);
      break;

    case SL_STATUS_FAIL:
      sl_log_warning(LOG_TAG,
                   "Command from NodeID %d:%d had an error during handling. "
                   "Not all parameters were accepted",
                   connection->remote.node_id,
                   connection->remote.endpoint_id);
      break;

    case SL_STATUS_BUSY:
      // This should not happen, or if it happens, we should be able to return
      // an application busy message or similar.
      sl_log_warning(LOG_TAG,
                     "Frame handler is busy and could not handle frame from "
                     "NodeID %d:%d correctly.",
                     connection->remote.node_id,
                     connection->remote.endpoint_id);
      break;

    case SL_STATUS_NOT_SUPPORTED:
      sl_log_warning(
        LOG_TAG,
        "Command from NodeID %d:%d got rejected because it is not supported. "
        "It was possibly also rejected due to security filtering",
        connection->remote.node_id,
        connection->remote.endpoint_id);
      break;

    default:
      sl_log_warning(
        LOG_TAG,
        "Command from NodeID %d:%d had an unexpected return status: 0x%04X\n",
        connection->remote.node_id,
        connection->remote.endpoint_id,
        status);
      break;
  }
  return status;
}

sl_status_t zwave_command_class_protocol_init()
{
  zwave_command_handler_t handler_protocol = { 0 };
  handler_protocol.support_handler = &zwave_command_class_protocol_support_handler;
  handler_protocol.control_handler = NULL;
  handler_protocol.minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  handler_protocol.command_class = ZWAVE_CMD_CLASS_PROTOCOL;
  handler_protocol.version = 1;
  handler_protocol.command_class_name = "Protocol";
  handler_protocol.manual_security_validation = false;

  if(SL_STATUS_OK != zwave_controller_register_callbacks(&zwave_command_class_protocol_callbacks))
  {
    sl_log_error(LOG_TAG, "Failed to register callbacks for Protocol Command Class");
    return SL_STATUS_FAIL;
  }

  if(SL_STATUS_OK != zwave_command_handler_register_handler(handler_protocol))
  {
    sl_log_error(LOG_TAG, "Failed to register Protocol Command Class");
    return SL_STATUS_FAIL;
  }

  zwave_command_handler_t handler_protocol_lr = { 0 };
  handler_protocol_lr.support_handler = &zwave_command_class_protocol_support_handler;
  handler_protocol_lr.control_handler = NULL;
  handler_protocol_lr.minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  handler_protocol_lr.command_class = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  handler_protocol_lr.version = 1;
  handler_protocol_lr.command_class_name = "Protocol LR";
  handler_protocol_lr.manual_security_validation = false;

  if (SL_STATUS_OK != zwave_command_handler_register_handler(handler_protocol_lr))
  {
    sl_log_error(LOG_TAG, "Failed to register Protocol Command Class");
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}