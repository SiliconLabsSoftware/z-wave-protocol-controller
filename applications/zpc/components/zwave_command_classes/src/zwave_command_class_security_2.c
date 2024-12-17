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
// Includes from this component
#include "zwave_command_class_security_2.h"
#include "zwave_command_class_indices.h"
#include "zwave_command_classes_utils.h"
#include "zwave_command_class_granted_keys_resolver.h"

// Generic includes
#include <assert.h>

// ZPC Includes
#include "zwave_controller_connection_info.h"
#include "zwave_controller_utils.h"
#include "zwave_rx.h"
#include "zwave_command_handler.h"
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store.h"
#include "zpc_attribute_store_network_helper.h"
#include "ZW_classcmd.h"
#include "zwave_tx_scheme_selector.h"

// Unify includes
#include "sl_log.h"
#include "attribute_store.h"
#include "attribute_store_helper.h"
#include "attribute_resolver.h"

// Log tag
#define LOG_TAG                                "zwave_command_class_security_2"
#define SECURE_SUPPORTED_COMMAND_CLASSES_INDEX 2

static zwave_node_id_t last_node_id = 0;

///////////////////////////////////////////////////////////////////////////////
// Command Handler functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_security_2_commands_supported_report(
  const zwave_controller_connection_info_t *connection,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  attribute_store_node_t endpoint_node
    = zwave_command_class_get_endpoint_node(connection);

  // We just received a report, it means that the key/protocol
  // combination that we are trying are working.
  zwave_command_class_mark_key_protocol_as_supported(
    attribute_store_get_first_parent_with_type(endpoint_node,
                                               ATTRIBUTE_NODE_ID),
    connection->encapsulation);

  // Get the Secure NIF atribute node under the endpoint
  attribute_store_node_t secure_nif_node
    = attribute_store_get_first_child_by_type(endpoint_node,
                                              ATTRIBUTE_ZWAVE_SECURE_NIF);
  if (frame_length <= SECURE_SUPPORTED_COMMAND_CLASSES_INDEX) {
    // Empty payload, if security scheme is equal to or higher than the ZPC
    // highest security scheme then delete the attribute
    // ATTRIBUTE_ZWAVE_SECURE_NIF. This is e.g. the case with CTT v3.
    if (is_using_zpc_highest_security_class(connection)) {
      attribute_store_delete_node(secure_nif_node);
      sl_log_debug(LOG_TAG,
                   "Received empty S2 Commands Supported Report with an equal "
                   "or higher security scheme than the ZPC, deleting the "
                   "Secure NIF from the Attribute Store");
      return SL_STATUS_OK;
    } else {
      sl_log_debug(LOG_TAG,
                   "Received empty S2 Commands Supported Report with "
                   "security scheme lower than highest ZPC security scheme");
      return SL_STATUS_OK;
    }
  } else {
    // Note that Securely Supported CC list will not be larger than 255
    uint8_t supported_cc_len
      = frame_length - SECURE_SUPPORTED_COMMAND_CLASSES_INDEX;

    // Accept the capabilities only if it is received at the highest granted key
    if (connection->encapsulation
        != zwave_tx_scheme_get_node_highest_security_class(
          connection->remote.node_id)) {
      // Here it could be a downgrade attack, where we receive a non-secure
      // S2 Command Supported Report. Do not accept the contents!
      sl_log_warning(LOG_TAG,
                     "Received S2 Commands Supported Report with "
                     "content on a 'non-secure' level. Discarding.");
      return SL_STATUS_OK;
    }

    attribute_store_set_child_reported(
      endpoint_node,
      ATTRIBUTE_ZWAVE_SECURE_NIF,
      &frame_data[SECURE_SUPPORTED_COMMAND_CLASSES_INDEX],
      supported_cc_len);
  }

  // We are done parsing the security 2 commands supported report frame
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_security_2_support_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

static sl_status_t zwave_command_class_security_2_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case SECURITY_2_COMMANDS_SUPPORTED_REPORT:
      return zwave_command_class_security_2_commands_supported_report(
        connection_info,
        frame_data,
        frame_length);
    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

static void on_nls_state_get_v2_send_complete(uint8_t status,
                                              const zwapi_tx_report_t *tx_info,
                                              void *user)
{
  // This callback is just for debugging purposes
  zwave_node_id_t node_id = *((zwave_node_id_t *)user);
  (void)tx_info;

  sl_log_debug(LOG_TAG, "%s, status: %d, node_id: %d", __func__, status, node_id);
}

static void on_nls_state_set_v2_send_complete(uint8_t status,
                                              const zwapi_tx_report_t *tx_info,
                                              void *user)
{
  // Standard frame constructing mechanism is not used here, as we want to
  // respond to the ACK reception of the NLS State Set command with a NLS State Get

  zwave_node_id_t node_id = *((zwave_node_id_t *)user);
  sl_status_t send_status = SL_STATUS_OK;

  sl_log_debug(LOG_TAG, "%s, status: %d, node_id: %d", __func__, status, node_id);

  switch (status) {
    case TRANSMIT_COMPLETE_VERIFIED: 
    case TRANSMIT_COMPLETE_OK:
      {
        ZW_NLS_STATE_GET_V2_FRAME frame = {0};
        frame.cmdClass    = COMMAND_CLASS_SECURITY_2;
        frame.cmd         = NLS_STATE_GET_V2;

        zwave_controller_connection_info_t connection_info  = {};
        zwave_tx_options_t tx_options                       = {};
        uint8_t number_of_expected_responses                = 1;
        uint32_t discard_timeout_ms                         = 5 * CLOCK_CONF_SECOND;

        zwave_tx_scheme_get_node_connection_info(node_id, 0, &connection_info);
        zwave_tx_scheme_get_node_tx_options(
          ZWAVE_TX_QOS_MAX_PRIORITY,
          number_of_expected_responses,
          discard_timeout_ms,
          &tx_options);

        last_node_id = node_id;
        send_status = zwave_tx_send_data(&connection_info,
          sizeof(ZW_NLS_STATE_GET_V2_FRAME),
          (const uint8_t *)&frame,
          &tx_options,
          on_nls_state_get_v2_send_complete,
          (void *)&last_node_id,
          NULL);      
      }
      break;
    default:
      send_status = SL_STATUS_TRANSMIT_INCOMPLETE;
      break;
  }

  if (send_status == SL_STATUS_OK) {
    sl_log_debug(LOG_TAG, "Sending NLS State Get Command to node ID: %d", node_id);
  } else {
    sl_log_error(LOG_TAG, "Failed to send NLS State Get Command to node ID: %d, status: %d", node_id, send_status);
  }
}

static sl_status_t
  zwave_command_class_security_2_nls_state_set(zwave_node_id_t node_id)
{
  ZW_NLS_STATE_SET_V2_FRAME frame = {0};
  frame.cmdClass    = COMMAND_CLASS_SECURITY_2;
  frame.cmd         = NLS_STATE_SET_V2;
  frame.nlsState    = true;

  zwave_controller_connection_info_t connection_info  = {0};
  zwave_tx_options_t tx_options                       = {0};
  uint8_t number_of_expected_responses                = 0;
  uint32_t discard_timeout_ms                         = 5 * CLOCK_CONF_SECOND;
  sl_status_t send_status                             = SL_STATUS_OK;

  zwave_tx_scheme_get_node_connection_info(node_id, 0, &connection_info);
  zwave_tx_scheme_get_node_tx_options(
    ZWAVE_TX_QOS_RECOMMENDED_GET_ANSWER_PRIORITY,
    number_of_expected_responses,
    discard_timeout_ms,
    &tx_options);
  
  last_node_id = node_id;
  send_status = zwave_tx_send_data(
    &connection_info,
    sizeof(ZW_NLS_STATE_SET_V2_FRAME),
    (const uint8_t *)&frame,
    &tx_options,
    on_nls_state_set_v2_send_complete,
    (void *)&last_node_id,
    NULL);

  if (send_status == SL_STATUS_OK) {
    sl_log_debug(LOG_TAG, "Sending NLS State Set Command to node ID: %d", node_id);
  } else {
    sl_log_error(LOG_TAG, "Failed to send NLS State Set Command to node ID: %d, status: %d", node_id, send_status);
  }

  return send_status;
}

static void on_attribute_zwave_nls_state_desired_change(
  attribute_store_node_t node, attribute_store_change_t change)
{
  if (change != ATTRIBUTE_UPDATED) {
    sl_log_debug(LOG_TAG, "NLS State Desired attribute change ignored");
    return;
  }

  zwave_node_id_t node_id = 0;
  attribute_store_network_helper_get_node_id_from_node(node, &node_id);

  zwave_command_class_security_2_nls_state_set(node_id);
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
//////////////////////////////////////////////////////////////////////////////
sl_status_t
  zwave_command_class_security_2_commands_supported_get(uint8_t *frame,
                                                        uint16_t *frame_len)
{
  ZW_SECURITY_2_COMMANDS_SUPPORTED_GET_FRAME *security_2_get_frame
    = (ZW_SECURITY_2_COMMANDS_SUPPORTED_GET_FRAME *)frame;
  security_2_get_frame->cmdClass = COMMAND_CLASS_SECURITY_2;
  security_2_get_frame->cmd      = SECURITY_2_COMMANDS_SUPPORTED_GET;
  *frame_len = sizeof(ZW_SECURITY_2_COMMANDS_SUPPORTED_GET_FRAME);

  return SL_STATUS_OK;
}

sl_status_t zwave_command_class_security_2_init()
{
  // Register the S2 CC handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  handler.support_handler    = &zwave_command_class_security_2_support_handler;
  handler.control_handler    = &zwave_command_class_security_2_control_handler;
  handler.minimal_scheme     = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  handler.command_class      = COMMAND_CLASS_SECURITY_2;
  handler.version            = SECURITY_2_VERSION;
  handler.command_class_name = "Security 2";
  handler.manual_security_validation = true;

  attribute_store_register_callback_by_type_and_state(
    on_attribute_zwave_nls_state_desired_change,
    ATTRIBUTE_ZWAVE_NLS_STATE,
    DESIRED_ATTRIBUTE);

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}
