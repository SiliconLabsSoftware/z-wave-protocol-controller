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
#include <assert.h>
#include "zwapi_protocol_controller.h"
#include "zwapi_func_ids.h"
#include "zwapi_session.h"
#include "zwapi_init.h"
#include "zwapi_internal.h"
#include "zwapi_utils.h"

// Unify includes
#include "sl_log.h"
#define LOG_TAG "zwave_protocol_controller"

sl_status_t zwapi_request_neighbor_update(zwave_node_id_t bNodeID,
                                           void (*completedFunc)(uint8_t))
{
  uint8_t func_id, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  func_id                                 = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, bNodeID);
  request_buffer[index++]                 = func_id;
  zwapi_request_neighbor_update_callback = completedFunc;

  return zwapi_send_command(FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE,
                            request_buffer,
                            index);
}

sl_status_t zwapi_get_protocol_info(zwave_node_id_t bNodeID,
                                    zwapi_node_info_header_t *node_info_header)
{
  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};

  zwapi_write_node_id(request_buffer, &index, bNodeID);
  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > (IDX_DATA + 5)) {
    node_info_header->capability         = response_buffer[IDX_DATA];
    node_info_header->security           = response_buffer[IDX_DATA + 1];
    node_info_header->reserved           = response_buffer[IDX_DATA + 2];
    node_info_header->node_type.basic    = response_buffer[IDX_DATA + 3];
    node_info_header->node_type.generic  = response_buffer[IDX_DATA + 4];
    node_info_header->node_type.specific = response_buffer[IDX_DATA + 5];
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_get_virtual_nodes(zwave_nodemask_t node_mask)
{
  uint8_t response_length                   = 0;
  uint8_t response_buffer[FRAME_LENGTH_MAX] = {0};
  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_GET_VIRTUAL_NODES,
                                       NULL,
                                       0,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK
      && response_length >= (IDX_DATA + ZW_MAX_NODEMASK_LENGTH)) {
    memcpy(node_mask, response_buffer + IDX_DATA, ZW_MAX_NODEMASK_LENGTH);
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_set_default(
  void (*completedFunc)(void)) /* IN Command completed call back function */
{
  uint8_t func_id, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  func_id                    = (completedFunc == NULL ? 0 : 0x03);
  request_buffer[index++]    = func_id;
  zwapi_set_default_callback = completedFunc;

  return zwapi_send_command(FUNC_ID_ZW_SET_DEFAULT, request_buffer, index);
}

sl_status_t
  zwapi_transfer_primary_ctrl_role(uint8_t bMode,
                                   void (*completedFunc)(LEARN_INFO *))
{
  uint8_t func_id, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  func_id                 = (completedFunc == NULL ? 0 : 0x03);
  request_buffer[index++] = bMode;
  request_buffer[index++] = func_id;
  zwapi_transfer_primary_ctrl_role_callback = completedFunc;

  return zwapi_send_command(FUNC_ID_ZW_CONTROLLER_CHANGE,
                            request_buffer,
                            index);
}

sl_status_t zwapi_create_new_primary_ctrl(uint8_t bMode,
                                          void (*completedFunc)(LEARN_INFO *))
{
  uint8_t func_id, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  func_id                                 = (completedFunc == NULL ? 0 : 0x03);
  request_buffer[index++]                 = bMode;
  request_buffer[index++]                 = func_id;
  zwapi_create_new_primarly_ctrl_callback = completedFunc;

  return zwapi_send_command(FUNC_ID_ZW_NEW_CONTROLLER, request_buffer, index);
}

sl_status_t zwapi_add_node_to_network(uint8_t bMode,
                                      void (*completedFunc)(LEARN_INFO *))
{
  uint8_t func_id, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  func_id                            = (completedFunc == NULL ? 0 : 0x03);
  request_buffer[index++]            = bMode;
  request_buffer[index++]            = func_id;
  zwapi_add_node_to_network_callback = completedFunc;

  return zwapi_send_command(FUNC_ID_ZW_ADD_NODE_TO_NETWORK,
                            request_buffer,
                            index);
}

sl_status_t zwapi_remove_node_from_network(uint8_t bMode,
                                           void (*completedFunc)(LEARN_INFO *))
{
  uint8_t func_id, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  func_id                                 = (completedFunc == NULL ? 0 : 0x03);
  request_buffer[index++]                 = bMode;
  request_buffer[index++]                 = func_id;
  zwapi_remove_node_from_network_callback = completedFunc;

  return zwapi_send_command(FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK,
                            request_buffer,
                            index);
}

uint8_t zwapi_remove_failed_node(zwave_node_id_t NodeID, void (*completedFunc)(uint8_t))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                           = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, NodeID);
  request_buffer[index++]           = func_id;
  zwapi_remove_failed_node_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_REMOVE_FAILED_NODE_ID,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);
  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA) {
    return response_buffer[IDX_DATA];
  }

  return ZW_FAILED_NODE_REMOVE_FAIL;
}

uint8_t zwapi_replace_failed_node(zwave_node_id_t NodeID,
                                  bool bNormalPower,
                                  void (*completedFunc)(uint8_t txStatus))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                            = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, NodeID);
  request_buffer[index++]            = func_id;
  zwapi_replace_failed_node_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_REPLACE_FAILED_NODE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);
  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA) {
    return response_buffer[IDX_DATA];
  }

  // Not a typo. Replace failed and remove fail use the same return status codes
  return ZW_FAILED_NODE_REMOVE_FAIL;
}

sl_status_t zwapi_assign_return_route(zwave_node_id_t bSrcNodeID,
                                      zwave_node_id_t bDstNodeID,
                                      void (*completedFunc)(uint8_t bStatus))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                            = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, bSrcNodeID);
  zwapi_write_node_id(request_buffer, &index, bDstNodeID);
  request_buffer[index++]            = func_id;
  zwapi_assign_return_route_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_ASSIGN_RETURN_ROUTE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_assign_priority_return_route(zwave_node_id_t bNodeID,
                                               zwave_node_id_t bDstNodeID,
                                               uint8_t * route,
                                               uint8_t speed,
                                               void (*completedFunc)(uint8_t bStatus))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                            = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, bNodeID);
  zwapi_write_node_id(request_buffer, &index, bDstNodeID);
  request_buffer[index++] = route[0];
  request_buffer[index++] = route[1];
  request_buffer[index++] = route[2];
  request_buffer[index++] = route[3];
  request_buffer[index++] = speed;
  request_buffer[index++] = func_id;
  zwapi_assign_priority_return_route_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_ASSIGN_PRIORITY_RETURN_ROUTE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_delete_return_route(zwave_node_id_t nodeID,
                                      void (*completedFunc)(uint8_t))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                            = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, nodeID);
  request_buffer[index++]            = func_id;
  zwapi_delete_return_route_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_DELETE_RETURN_ROUTE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_assign_suc_return_route(zwave_node_id_t bSrcNodeID,
                                          void (*completedFunc)(uint8_t))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                                = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, bSrcNodeID);
  request_buffer[index++]                = func_id;
  zwapi_assign_suc_return_route_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_ASSIGN_SUC_RETURN_ROUTE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_delete_suc_return_route(zwave_node_id_t nodeID,
                                          void (*completedFunc)(uint8_t))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                                = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, nodeID);
  request_buffer[index++]                = func_id;
  zwapi_delete_suc_return_route_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_DELETE_SUC_RETURN_ROUTE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

zwave_node_id_t zwapi_get_suc_node_id(void)
{
  uint8_t response_length                   = 0;
  uint8_t response_buffer[FRAME_LENGTH_MAX] = {0};
  zwave_node_id_t suc_node_id = 0;
  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_GET_SUC_NODE_ID,
                                       NULL,
                                       0,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK &&
      (((NODEID_8BITS == zwapi_get_node_id_basetype()) &&(response_length > IDX_DATA)) ||
      ((NODEID_16BITS == zwapi_get_node_id_basetype()) &&(response_length > IDX_DATA + 1)))) {
    int current_index = IDX_DATA;
    if (NODEID_16BITS == zwapi_get_node_id_basetype()) {
      suc_node_id = response_buffer[current_index++] << 8;
    }
    suc_node_id |= response_buffer[current_index++] & 0xFF;
  }

  return suc_node_id;
}

sl_status_t zwapi_set_suc_node_id(zwave_node_id_t nodeID,
                                  uint8_t SUCState,
                                  uint8_t bTxOption,
                                  uint8_t bCapabilities,
                                  void (*completedFunc)(uint8_t txStatus))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                 = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, nodeID);
  request_buffer[index++] = SUCState; /* Do we want to enable or disable?? */
  request_buffer[index++] = bTxOption;
  request_buffer[index++] = bCapabilities;
  request_buffer[index++] = func_id;
  zwapi_set_suc_node_id_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_SET_SUC_NODE_ID,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_set_learn_mode(uint8_t mode,
                                 void (*completedFunc)(LEARN_INFO *))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                       = (completedFunc == NULL ? 0 : 0x03);
  request_buffer[index++]       = mode;
  request_buffer[index++]       = func_id;
  zwapi_set_learn_mode_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_SET_LEARN_MODE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }
  return SL_STATUS_FAIL;
}

sl_status_t zwapi_set_virtual_node_to_learn_mode(
  zwave_node_id_t node, uint8_t mode, void (*completedFunc)(uint8_t, zwave_node_id_t, zwave_node_id_t))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                 = (completedFunc == NULL ? 0 : 0x03);
  zwapi_write_node_id(request_buffer, &index, node);
  request_buffer[index++] = mode;
  request_buffer[index++] = func_id;
  zwapi_set_virtual_node_to_learn_mode_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_SET_SLAVE_LEARN_MODE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_request_node_info(zwave_node_id_t node_id)
{
  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  zwapi_write_node_id(request_buffer, &index, node_id);

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_REQUEST_NODE_INFO,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_request_protocol_cc_encryption_callback(uint8_t tx_status, const zwapi_tx_report_t *tx_report, uint8_t session_id)
{
  uint8_t index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};

  request_buffer[index++] = session_id;
  request_buffer[index++] = tx_status;

  request_buffer[index++] = (tx_report->transmit_ticks >> 8) & 0xFF;
  request_buffer[index++] = tx_report->transmit_ticks & 0xFF;
  request_buffer[index++] = tx_report->number_of_repeaters;
  request_buffer[index++] = tx_report->ack_rssi;
  request_buffer[index++] = tx_report->rssi_values.incoming[0];
  request_buffer[index++] = tx_report->rssi_values.incoming[1];
  request_buffer[index++] = tx_report->rssi_values.incoming[2];
  request_buffer[index++] = tx_report->rssi_values.incoming[3];
  request_buffer[index++] = tx_report->ack_channel_number;
  request_buffer[index++] = tx_report->tx_channel_number;
  request_buffer[index++] = tx_report->route_scheme_state;
  request_buffer[index++] = tx_report->last_route_repeaters[0];
  request_buffer[index++] = tx_report->last_route_repeaters[1];
  request_buffer[index++] = tx_report->last_route_repeaters[2];
  request_buffer[index++] = tx_report->last_route_repeaters[3];
  request_buffer[index] = (tx_report->beam_1000ms & 0x01) << 6;
  request_buffer[index] |= (tx_report->beam_250ms & 0x01) << 5;
  request_buffer[index] |= (tx_report->last_route_speed & 0x07);
  index++;
  request_buffer[index++] = tx_report->routing_attempts;
  request_buffer[index++] = tx_report->last_failed_link.from;
  request_buffer[index++] = tx_report->last_failed_link.to;
  request_buffer[index++] = tx_report->tx_power;
  request_buffer[index++] = tx_report->measured_noise_floor;
  request_buffer[index++] = tx_report->destination_ack_mpdu_tx_power;
  request_buffer[index++] = tx_report->destination_ack_mpdu_measured_rssi;
  request_buffer[index++] = tx_report->destination_ack_mpdu_measured_noise_floor;

  sl_status_t send_command_status
    = zwapi_send_command(FUNC_ID_ZW_REQUEST_PROTOCOL_CC_ENCRYPTION,
                         request_buffer,
                         index);

  if (send_command_status == SL_STATUS_OK) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t
  zwapi_send_controller_replication(zwave_node_id_t nodeID,
                                    const uint8_t *pData,
                                    uint8_t dataLength,
                                    uint8_t txOptions,
                                    void (*completedFunc)(uint8_t txStatus))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id = (completedFunc == NULL ? 0 : 0x03);

  zwapi_write_node_id(request_buffer, &index, nodeID);
  request_buffer[index++] = dataLength;
  for (uint8_t i = 0; i < dataLength; i++) {
    request_buffer[index++] = pData[i];
  }
  request_buffer[index++]                    = txOptions;
  request_buffer[index++]                    = func_id;
  zwapi_send_controller_replication_callback = completedFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_REPLICATION_SEND_DATA,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

uint8_t zwapi_get_controller_capabilities(void)
{
  uint8_t response_length                   = 0;
  uint8_t response_buffer[FRAME_LENGTH_MAX] = {0};
  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES,
                                       NULL,
                                       0,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA) {
    return response_buffer[IDX_DATA];
  }

  // Old types of controllers also return a 0 bitmask, but the application
  // should double check if the controller returns no capabilities
  return 0;
}

sl_status_t zwapi_request_network_update(void (*complFunc)(uint8_t))
{
  uint8_t func_id, response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  func_id                                     = (complFunc == NULL ? 0 : 0x03);
  request_buffer[index++]                     = func_id;
  zwapi_request_network_update_callback       = complFunc;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_REQUEST_NETWORK_UPDATE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_replication_receive_complete()
{
  return zwapi_send_command(FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE, NULL, 0);
}

bool zwapi_is_node_primary_ctrl(void)
{
  return ((zwapi_get_controller_capabilities() & CONTROLLER_IS_SECONDARY) == 0);
}

bool zwapi_is_node_failed(zwave_node_id_t nodeID)
{
  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  zwapi_write_node_id(request_buffer, &index, nodeID);

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_IS_FAILED_NODE_ID,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA) {
    return response_buffer[IDX_DATA];
  }

  // Assume the node is not failed if we could not talk to the Z-Wave module
  return false;
}

sl_status_t zwapi_set_max_source_route(uint8_t maxRouteTries)
{
  if (maxRouteTries == 0 || maxRouteTries > 20) {
    return false;
  }

  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  request_buffer[index++]                     = maxRouteTries;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_SET_ROUTING_MAX,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

uint8_t zwapi_get_priority_route(zwave_node_id_t bNodeID, uint8_t *pLastWorkingRoute)
{
  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  zwapi_write_node_id(request_buffer, &index, bNodeID);

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_GET_LAST_WORKING_ROUTE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > (IDX_DATA + 6)) {
    index = IDX_DATA;
    zwave_node_id_t response_node_id = zwapi_read_node_id(response_buffer, &index);
    if (bNodeID == response_node_id) {
      memcpy(pLastWorkingRoute, &response_buffer[IDX_DATA + 2], 5);
      return response_buffer[IDX_DATA + 1];
    }
  }
  // FIXME: ZW_PRIORITY_ROUTE_* codes are missing a define for
  // "No Priority Route found for NodeID", which is 0
  return 0;
}

sl_status_t zwapi_set_priority_route(zwave_node_id_t bNodeID,
                                     uint8_t *pLastWorkingRoute)
{
  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  zwapi_write_node_id(request_buffer, &index, bNodeID);
  request_buffer[index++]                     = pLastWorkingRoute[0];
  request_buffer[index++]                     = pLastWorkingRoute[1];
  request_buffer[index++]                     = pLastWorkingRoute[2];
  request_buffer[index++]                     = pLastWorkingRoute[3];
  request_buffer[index++]                     = pLastWorkingRoute[4];

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_SET_LAST_WORKING_ROUTE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_get_old_routing_info(zwave_node_id_t bNodeID,
                                       uint8_t *buf,
                                       uint8_t bRemoveBad,
                                       uint8_t bRemoveNonReps)
{
  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  zwapi_write_node_id(request_buffer, &index, bNodeID);
  request_buffer[index++]                     = bRemoveBad;
  request_buffer[index++]                     = bRemoveNonReps;
  request_buffer[index++]                     = 0;  // func_id must be set to 0

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_GET_ROUTING_TABLE_LINE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK
      && response_length >= (IDX_DATA + ZW_MAX_NODEMASK_LENGTH)) {
    memcpy(buf, response_buffer + IDX_DATA, ZW_MAX_NODEMASK_LENGTH);
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_enable_suc(uint8_t state, uint8_t capabilities)
{
  uint8_t response_length = 0, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0},
          response_buffer[FRAME_LENGTH_MAX]   = {0};
  request_buffer[index++]                     = state;
  request_buffer[index++]                     = capabilities;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_ENABLE_SUC,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_add_smartstart_node_to_network(
  uint8_t bMode, const uint8_t *dsk, void (*completedFunc)(LEARN_INFO *))
{
  uint8_t func_id, index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  func_id                            = (completedFunc == NULL ? 0 : 0x03);
  request_buffer[index++]            = bMode;
  request_buffer[index++]            = func_id;
  request_buffer[index++]            = dsk[0];
  request_buffer[index++]            = dsk[1];
  request_buffer[index++]            = dsk[2];
  request_buffer[index++]            = dsk[3];
  request_buffer[index++]            = dsk[4];
  request_buffer[index++]            = dsk[5];
  request_buffer[index++]            = dsk[6];
  request_buffer[index++]            = dsk[7];
  zwapi_add_node_to_network_callback = completedFunc;

  return zwapi_send_command(FUNC_ID_ZW_ADD_NODE_TO_NETWORK,
                            request_buffer,
                            index);
}

sl_status_t zwapi_set_virtual_node_application_node_information(
  zwave_node_id_t dstNode,
  uint8_t listening,
  const node_type_t *node_type,
  uint8_t *nodeParm,
  uint8_t parmLength)
{
  uint8_t index                               = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
  zwapi_write_node_id(request_buffer, &index, dstNode);
  request_buffer[index++]                     = listening;
  request_buffer[index++]                     = node_type->generic;
  request_buffer[index++]                     = node_type->specific;
  request_buffer[index++]                     = parmLength;

  for (uint8_t i = 0; i < parmLength; i++) {
    request_buffer[index++] = nodeParm[i];
  }

  return zwapi_send_command(FUNC_ID_ZW_SET_SLAVE_LEARN_MODE,
                            request_buffer,
                            index);
}

sl_status_t zwapi_enable_node_nls(const zwave_node_id_t nodeId)
{
  uint8_t response_length = 0;
  uint8_t index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = { 0 };
  uint8_t response_buffer[FRAME_LENGTH_MAX] = { 0 };
  zwapi_write_node_id(request_buffer, &index, nodeId);
  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_ENABLE_NODE_NLS,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE)
  {
    sl_log_debug(LOG_TAG, "NLS enabled for node %d", nodeId);
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL; 
}

sl_status_t zwapi_get_node_nls(
  const zwave_node_id_t nodeId,
  uint8_t* nls_state)
{
  uint8_t response_length = 0;
  uint8_t index = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = { 0 };
  uint8_t response_buffer[FRAME_LENGTH_MAX] = { 0 };
  zwapi_write_node_id(request_buffer, &index, nodeId);
  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_GET_NODE_NLS_STATE,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA)
  {
    *nls_state = response_buffer[IDX_DATA];
    return SL_STATUS_OK;    
  }

  return SL_STATUS_FAIL;
}

sl_status_t zwapi_get_nls_nodes(uint16_t *list_length,
                                zwave_nodemask_t node_list)
{
  uint8_t more_nodes         = 1;
  uint8_t start_offset       = 0;
  sl_status_t command_status = SL_STATUS_OK;
  *list_length               = 0;

  while (more_nodes > 0) {
    uint8_t response_length = 0;
    uint8_t index = 0;
    uint8_t request_buffer[REQUEST_BUFFER_SIZE] = {0};
    uint8_t response_buffer[FRAME_LENGTH_MAX]   = {0};
    uint8_t received_nodelist_length            = 0;
    request_buffer[index++]                     = start_offset;

    command_status = zwapi_send_command_with_response(FUNC_ID_ZW_GET_NLS_NODES,
                                                      request_buffer,
                                                      index,
                                                      response_buffer,
                                                      &response_length);

    if (command_status == SL_STATUS_OK) {
      more_nodes               = response_buffer[IDX_DATA];
      start_offset             = response_buffer[IDX_DATA + 1];
      received_nodelist_length = response_buffer[IDX_DATA + 2];
      *list_length += received_nodelist_length;
      const uint8_t *p = &response_buffer[IDX_DATA + 3];
      for (uint8_t i = 0; i < received_nodelist_length; i++) {
        if ((i + (start_offset * GET_NLS_NODES_OFFSET_GRANULARITY_IN_BYTES)) // NOSONAR
            > sizeof(zwave_nodemask_t)) {
          sl_log_debug(LOG_TAG,
                       "Z-Wave API get NLS node list index of bound\n");
          return SL_STATUS_FAIL;
        }
        node_list[i + start_offset * GET_NLS_NODES_OFFSET_GRANULARITY_IN_BYTES]
          = *p;
        p++;
      }
      start_offset++;
    } else {
      sl_log_debug(LOG_TAG,
                   "Z-Wave API get NLS node list retrieve error, %d",
                   command_status);
      return SL_STATUS_FAIL;
    }
  }

  // Buffer is shifted due to misalignement between zwave stack and ZPC.
  // It will be addressed in SWPROT-9279.
  memmove(&node_list[ZW_LR_NODEMASK_OFFSET - 1],
          &node_list[ZW_MAX_NODEMASK_LENGTH - 1],
          ZW_LR_MAX_NODEMASK_LENGTH - ZW_LR_NODEMASK_OFFSET);

  return SL_STATUS_OK;
}

sl_status_t zwapi_transfer_protocol_cc(
  const zwave_node_id_t srcNode,
  const uint8_t decryptionKey,
  const uint8_t payloadLength,
  const uint8_t * const payload)
{
  uint8_t index = 0;
  uint8_t response_length = 0;
  uint8_t request_buffer[REQUEST_BUFFER_SIZE] = { 0 };
  uint8_t response_buffer[FRAME_LENGTH_MAX] = { 0 };

  zwapi_write_node_id(request_buffer, &index, srcNode);
  request_buffer[index++] = decryptionKey;

  if (payloadLength > ZWAVE_MAX_FRAME_SIZE)
  {
    return SL_STATUS_WOULD_OVERFLOW;
  }

  request_buffer[index++] = payloadLength;
  memcpy(&request_buffer[index], payload, payloadLength);
  index += payloadLength;

  sl_status_t send_command_status
    = zwapi_send_command_with_response(FUNC_ID_ZW_TRANSFER_PROTOCOL_CC,
                                       request_buffer,
                                       index,
                                       response_buffer,
                                       &response_length);

  if (send_command_status == SL_STATUS_OK && response_length > IDX_DATA
      && response_buffer[IDX_DATA] == ZW_COMMAND_RETURN_VALUE_TRUE) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}
