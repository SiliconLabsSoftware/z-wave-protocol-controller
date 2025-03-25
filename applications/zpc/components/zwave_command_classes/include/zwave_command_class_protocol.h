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

#ifndef ZWAVE_COMMAND_CLASS_PROTOCOL_H
#define ZWAVE_COMMAND_CLASS_PROTOCOL_H

#include "sl_status.h"
#include "zwave_controller_connection_info.h"
#include "zwave_rx.h"
#include "zwave_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief Setup fixture for the Protocol Command Class.
 *
 * This setup will register the Protocol command handler
 * to the Z-Wave CC framework,
 *
 * @returns SL_STATUS_OK if successful
 * @returns SL_STATUS_FAIL if an error occurred
 */
sl_status_t zwave_command_class_protocol_init(void);

/**
 * @brief 
 */
void zwave_on_protocol_cc_encryption_request(
  const zwave_node_id_t destination_node_id,
  const uint8_t payload_length,
  const uint8_t *const payload,
  const uint8_t protocol_metadata_length,
  const uint8_t *const protocol_metadata,
  const uint8_t use_supervision,
  const uint8_t session_id);

#ifdef __cplusplus
}
#endif

#endif  //ZWAVE_COMMAND_CLASS_PROTOCOL_H