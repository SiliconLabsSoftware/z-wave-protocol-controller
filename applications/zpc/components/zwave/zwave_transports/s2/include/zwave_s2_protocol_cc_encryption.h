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

/**
 * @defgroup zwave_s2_protocol_cc_encryption Security 2 Protocol Command Class Encryption
 * @ingroup zwave_transports
 * @brief Protocol Command Class Encryption implementation for Z-Wave S2
 *
 * @{
 */

#ifndef ZWAVE_S2_PROTOCOL_CC_ENCRYPTION_H
#define ZWAVE_S2_PROTOCOL_CC_ENCRYPTION_H

#include <stdbool.h>
#include "sl_status.h"
#include "zwave_node_id_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Triggered when a Protocol CC Encryption Request is received.
 *
 * This callback has to be registered to the Z-Wave Controller callbacks.
 *
 * @param destination_node_id       Destination node ID.
 * @param payload_length            Length of the payload to be encrypted.
 * @param payload                   Payload to be encrypted.
 * @param protocol_metadata_length  Length of the protocol metadata.
 * @param protocol_metadata         Protocol metadata.
 * @param use_supervision           Whether to use supervision.
 * @param session_id                Session ID.
 *
 */
void zwave_s2_on_protocol_cc_encryption_request(
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

#endif  //ZWAVE_S2_PROTOCOL_CC_ENCRYPTION_H
/** @} end zwave_s2_protocol_cc_encryption */

