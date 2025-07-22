/******************************************************************************
* # License
* <b>Copyright 2021  Silicon Laboratories Inc. www.silabs.com</b>
******************************************************************************
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
*****************************************************************************/

#include "zwave_controller_crc16.h"

// Generic includes
#include <stdbool.h>
#include <string.h>

// Includes from this component
#include "zwave_crc16_transport.h"
#include "zwave_command_class_crc16.h"

// Includes from other components
#include "ZW_classcmd.h"
#include "zwave_command_class_indices.h"
#include "zwave_controller_connection_info.h"
#include "zwave_controller_transport.h"
#include "zwave_controller_internal.h"
#include "zwave_rx.h"
#include "zwave_tx.h"
#include "zwave_utils.h"

// Unify includes
#include "sl_log.h"

#define LOG_TAG "zwave_crc16_transport"

#define CRC_INITAL_VALUE 0x1D0Fu

// Send data state
typedef struct send_data_state {
  // User Callback to invoken when transmission is completed
  on_zwave_tx_send_data_complete_t on_send_data_complete;
  // User pointer to use for the invoking the on_send_data_complete function
  void *user;
  // Are we idle or currently transmitting.
  bool transmission_ongoing;
  // Save the Parent Tx session ID to be able to abort
  zwave_tx_session_id_t parent_session_id;
} send_data_state_t;
static send_data_state_t state;

///////////////////////////////////////////////////////////////////////////////
// Private helper functions
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes our array of callback settings by setting
 *        everything to NULL
 */
static void reset_send_data_settings()
{
  state.transmission_ongoing  = false;
  state.on_send_data_complete = NULL;
  state.user                  = NULL;
  state.parent_session_id     = NULL;
}

static void zwave_crc16_transport_start_transmission(
  on_zwave_tx_send_data_complete_t callback,
  void *user,
  zwave_tx_session_id_t parent_session_id)
{
  state.transmission_ongoing  = true;
  state.on_send_data_complete = callback;
  state.user                  = user;
  state.parent_session_id     = parent_session_id;
}

///////////////////////////////////////////////////////////////////////////////
// Z-Wave Controller transport functions
///////////////////////////////////////////////////////////////////////////////
/**
 * @defgroup crc16_transport Multi Channel Transport
 * @ingroup crc16_command_class
 * @brief CRC 16 encapsulation and decapsulation module
 *
 * This module allows to send and receive Multi Channel encapsulated
 * frames.
 *
 * @{
 */
/**
 * @brief Callback function registered to Z-Wave TX \ref zwave_tx_send_data
 *
 * It helps tracking when a frame was fully transmitted and will invoke the
 * callback of the component that called the \ref
 * zwave_command_class_crc16_send_data function
 *
 * @param status  Indicates how the transmission operation was completed.
 *                Refer for \ref zwapi_transmit_complete_codes for details.
 * @param tx_info zwapi_tx_report_t reported by the @ref zwave_api. It
 *                contains transmission details, refer to \ref zwapi_tx_report_t.
 * @param user    User pointer provided in \ref zwave_command_class_crc16_send_data()
 */
static void on_crc16_send_complete(uint8_t status,
                                   const zwapi_tx_report_t *tx_info,
                                   void *user)
{
  (void)user;
  // Call the registered callback directly and tell them we are happy with the
  // transmission of our Multi Channel encapsulated frame. No retry or
  // additional frames needed
  if (state.transmission_ongoing == false) {
    sl_log_warning(LOG_TAG,
                   "Send data complete callback while no transmission "
                   "is ongoing. Ignoring.");
    return;
  }

  // Give the caller a callback, if they wanted one
  if (state.on_send_data_complete != NULL) {
    state.on_send_data_complete(status, tx_info, state.user);
  }

  reset_send_data_settings();
}

/**
 * @brief Encapsulates with CRC16 
 * @param connection       Connection object describing the source and
 *                         destination.
 * @param data_length      Length of the frame to send
 * @param data             Points to the payload to send
 * @param tx_options       Transmit options to use.
 * @param on_send_data_complete  Callback function that will be called when
 *                                  the send operation has completed
 * @param user             User pointer passed in argument of the on_send_complete
 *                         callback function
 * @param parent_session_id Value of the frame in the TX Queue that is the parent
 *                          of this frame. Frames MUST have a valid parent
 *
 * @returns
 * - SL_STATUS_OK The transmission request has been accepted and callback will be
 *                    triggered when the operation is completed.
 * - SL_STATUS_NOT_SUPPORTED   If no endpoint encapsulation is to be applied
 * - SL_STATUS_FAIL           If the transmission cannot be done at the moment.
 */
static sl_status_t zwave_command_class_crc16_send_data(
  const zwave_controller_connection_info_t *connection,
  uint16_t data_length,
  const uint8_t *data,
  const zwave_tx_options_t *tx_options,
  const on_zwave_tx_send_data_complete_t on_send_data_complete,
  void *user,
  zwave_tx_session_id_t parent_session_id)
{
  // Check if the frame is already CRC16 encapsulated
  if ((data_length >= 1)
      && (data[COMMAND_CLASS_INDEX] == COMMAND_CLASS_CRC_16_ENCAP)) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  // Is the frame too big for us?
  if (data_length > CRC_16_ENCAPSULATED_COMMAND_MAXIMUM_SIZE) {
    sl_log_critical(LOG_TAG, "Frame is too big for CRC16 encapsulation");
    return SL_STATUS_WOULD_OVERFLOW;
  }

  // CC:0056.01.00.21.003  : The CRC-16 Encapsulation Command Class MUST NOT be encapsulated by any other Command Class.
  if (connection->encapsulation != ZWAVE_CONTROLLER_ENCAPSULATION_NONE) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  if (state.transmission_ongoing) {
    sl_log_critical(LOG_TAG,
                    "Transmission is ongoing, cannot send another frame");
    return SL_STATUS_BUSY;
  }

  // Retrieve the Attribute Store node for the endpoint:
  attribute_store_node_t endpoint_node
    = zwave_get_endpoint_node(connection->remote.node_id,
                              connection->remote.endpoint_id);
  // Check if end node support CRC_16
  if (!zwave_command_class_crc16_is_supported(endpoint_node)) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  // If we don't expect any response we check if the sender wants us to use CRC16
  // or not
  if (tx_options != NULL && tx_options->number_of_responses == 0
      && !zwave_command_class_crc16_is_expecting_crc16_response(
        connection->remote.node_id,
        connection->remote.endpoint_id)) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  zwave_crc16_encapsulation_frame_t frame = {0};
  uint8_t *raw_frame                      = (uint8_t *)&frame;
  uint16_t frame_length                   = 0;

  frame.command_class = COMMAND_CLASS_CRC_16_ENCAP;
  frame.command       = CRC_16_ENCAP;
  memcpy(frame.encapsulated_command, data, data_length);

  frame_length = data_length + CRC_16_ENCAPSULATION_HEADER;
  // Compute the CRC16 of the frame data with the header
  uint16_t crc16
    = zwave_controller_crc16(CRC_INITAL_VALUE, raw_frame, frame_length);

  raw_frame[frame_length++] = (crc16 >> 8) & 0xFF;
  raw_frame[frame_length++] = crc16 & 0xFF;

  // New frame will be a child of original frame
  zwave_tx_options_t multi_channel_tx_options;

  if (tx_options) {
    multi_channel_tx_options = *tx_options;
  }

  multi_channel_tx_options.transport.parent_session_id = parent_session_id;
  multi_channel_tx_options.transport.valid_parent_session_id = true;

  sl_status_t transmit_status = zwave_tx_send_data(connection,
                                                   frame_length,
                                                   (const uint8_t *)raw_frame,
                                                   &multi_channel_tx_options,
                                                   &on_crc16_send_complete,
                                                   NULL,
                                                   NULL);

  if (transmit_status != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  // Clear flag now that we have sent the frame
  zwave_command_class_crc16_clear_expect_crc16_response(
    connection->remote.node_id,
    connection->remote.endpoint_id);

  zwave_crc16_transport_start_transmission(on_send_data_complete,
                                           user,
                                           parent_session_id);

  return SL_STATUS_OK;
}

/**
 * @brief Decapsulate Multi Channel encapsulation and inject the frame back to
 *        the Z-Wave Controller
 *
 * The provided payload will be Multi Channel decapsulated and the endpoint
 * data will be copied in
 * connection->remote.endpoint_id and connection->local.endpoint_id and passed
 * to the \ref zwave_controller_on_frame_received() function.
 *
 * Bit addressing can be used by setting the endpoint value directly (1 bit
 * bit addressing and 7 bits endpoint identifier)
 *
 * @param connection_info  Connection object describing the source and
 *                         destination.
 * @param rx_options       Connection object describing the source and
 *                         destination.
 * @param frame_data       Length of the frame to send
 * @param frame_length     Points to the payload to send
 *
 * @returns
 * - SL_STATUS_NOT_SUPPORTED  if the frame data is not CRC16 encapsulated
 *                            properly to generate a decapsulated frame
 * - SL_STATUS_NOT_FOUND      if the frame is not encapsulated but should not be
 *                            discarded.
 * - SL_STATUS_WOULD_OVERFLOW If the decapsulated frame is too large to fit in
 *                            our local buffer
 * - SL_STATUS_OK             If the frame was decapsulated and should be discarded
 *                            because its decapsulated version has been passed
 *                            to the Z-Wave Controller.
 * - SL_STATUS_FAIl           if the frame should be discarded
 */
static sl_status_t zwave_command_class_crc16_decapsulate(
  const zwave_controller_connection_info_t *connection_info,
  const zwave_rx_receive_options_t *rx_options,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX
      || frame_data[COMMAND_CLASS_INDEX] != COMMAND_CLASS_CRC_16_ENCAP
      || frame_data[COMMAND_INDEX] != CRC_16_ENCAP) {
    // SL_STATUS_NOT_FOUND will pass the frame to the upper layer,
    // CC:0056.01.00.21.006 b. If the request is sent non-encapsulated, the response MUST be sent non-encapsulated
    return SL_STATUS_NOT_FOUND;
  }

  if (frame_length <= CRC_16_ENCAPSULATION_OVERHEAD) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  uint8_t decapsulated_frame[ZWAVE_MAX_FRAME_SIZE] = {0};
  uint16_t decapsulated_frame_length               = 0;

  if (frame_length - CRC_16_ENCAPSULATION_OVERHEAD
      > sizeof(decapsulated_frame)) {
    return SL_STATUS_WOULD_OVERFLOW;
  }
  decapsulated_frame_length = frame_length - CRC_16_ENCAPSULATION_OVERHEAD;
  memcpy(decapsulated_frame,
         &frame_data[CRC_16_ENCAPSULATION_HEADER],
         decapsulated_frame_length);

  uint16_t expected_crc16
    = (frame_data[frame_length - 2] << 8) | frame_data[frame_length - 1];

  // CC:0056.01.01.11.005:  The checksum data MUST be built by taking all bytes starting from the CRC16 Command Class
  //                        identifier (COMMAND_CLASS_CRC_16_ENCAP) until the last byte of the Data field.
  // This means we have ton compute the CRC16 of the frame data with the header, but without the CRC16 footer
  uint16_t computed_crc16
    = zwave_controller_crc16(CRC_INITAL_VALUE,
                             frame_data,
                             frame_length - CRC_16_ENCAPSULATION_FOOTER);

  if (expected_crc16 != computed_crc16) {
    sl_log_warning(LOG_TAG,
                   "CRC16 check failed, discarding frame : %d",
                   computed_crc16);
    return SL_STATUS_FAIL;
  }

  // zwave_command_class_crc16_set_expect_crc16_response(
  //   connection_info->remote.node_id,
  //   connection_info->remote.endpoint_id);

  zwave_controller_on_frame_received(connection_info,
                                     rx_options,
                                     decapsulated_frame,
                                     decapsulated_frame_length);

  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_crc16_abort_send_data(zwave_tx_session_id_t session_id)
{
  if (state.transmission_ongoing == true
      && state.parent_session_id == session_id) {
    sl_log_debug(LOG_TAG, "Aborting CRC16 session for frame id=%p", session_id);
    on_crc16_send_complete(TRANSMIT_COMPLETE_FAIL, NULL, NULL);
    return SL_STATUS_OK;
  }

  return SL_STATUS_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////////
// Shared functions within the component
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_crc16_transport_init()
{
  reset_send_data_settings();

  // Register our transport to the Z-Wave Controller Transport
  zwave_controller_transport_t transport = {0};
  transport.priority                     = 4;
  transport.command_class                = COMMAND_CLASS_CRC_16_ENCAP;
  transport.version                      = CRC_16_ENCAP_VERSION;
  transport.on_frame_received = &zwave_command_class_crc16_decapsulate;
  transport.send_data         = &zwave_command_class_crc16_send_data;
  transport.abort_send_data   = &zwave_command_class_crc16_abort_send_data;

  return zwave_controller_transport_register(&transport);
}
