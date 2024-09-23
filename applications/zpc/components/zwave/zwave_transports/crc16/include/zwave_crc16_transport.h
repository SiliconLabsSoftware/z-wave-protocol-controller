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

/**
 * @defgroup zwave_crc16_transport CRC16 Transport
 * @ingroup zwave_transports
 * @brief Transport for CRC16
 *
 * @{
 */

#ifndef ZWAVE_CRC_16_TRANSPORT_H
#define ZWAVE_CRC_16_TRANSPORT_H

#include "sl_status.h"
#include "zwave_node_id_definitions.h"

// We allow to encapsulate the maximum minus our encapsulation command overhead
#define CRC_16_ENCAPSULATION_HEADER 2
#define CRC_16_ENCAPSULATION_FOOTER 2
#define CRC_16_ENCAPSULATION_OVERHEAD (CRC_16_ENCAPSULATION_HEADER + CRC_16_ENCAPSULATION_FOOTER)

#define CRC_16_ENCAPSULATED_COMMAND_MAXIMUM_SIZE \
  (ZWAVE_MAX_FRAME_SIZE - CRC_16_ENCAPSULATION_OVERHEAD)

typedef struct zwave_crc16_encapsulation_frame {
  uint8_t command_class; /* The command class */
  uint8_t command;       /* The command */
  uint8_t encapsulated_command
    [CRC_16_ENCAPSULATED_COMMAND_MAXIMUM_SIZE]; /* The checksum will be appended to the command*/
/* Encapsulated command */
} zwave_crc16_encapsulation_frame_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the CRC16 Transport
 * *
 * @returns SL_STATUS_OK if successful
 * @returns SL_STATUS_FAIL if an error occurred
 */
sl_status_t zwave_crc16_transport_init(void);

#ifdef __cplusplus
}
#endif

#endif  //ZWAVE_CRC_16_TRANSPORT_H
/** @} end zwave_crc16_transport */
