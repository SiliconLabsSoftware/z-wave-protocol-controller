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
 * @defgroup zwave_rx_fixt Z-Wave RX fixture
 * @ingroup zwave_rx
 * @addtogroup zwave_rx_fixt
 * @brief Z-Wave RX fixture
 *
 * The Z-Wave RX fixture is responsible for starting up the @ref zwave_api using
 * the Z-Wave related parameters from the @ref zpc_config.
 *
 * @{
 */

#ifndef ZWAVE_RX_FIXT_H
#define ZWAVE_RX_FIXT_H

#include <stdbool.h>
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  /// Name of the serial port of the Z-Wave module
  const char *serial_port;
  /// If set the Serial log will be written here
  const char *serial_log_file;
  /// Z-Wave RF Region setting. It can be one of the following:
  // "EU", "US","ANZ", HK", "IN", "IL", "RU", "CN", "JP", "KR"
  const char *zwave_rf_region;
  /// Transmit power for the Z-Wave module. Refer to \ref zwave_rx_init
  int zwave_normal_tx_power_dbm;
  /// Measured 0dBm output power for the Z-Wave module.
  /// Refer to \ref zwave_rx_init
  int zwave_measured_0dbm_power;
  /// Max Z-Wave Long Range Transmit power
  int zwave_max_lr_tx_power_dbm;
} zwave_rx_config_t;

void zwave_rx_set_config(const zwave_rx_config_t* config);

/** Setup fixture for Z-Wave RX.
 *
 * This setup function will initialize the communication with the
 * Z-Wave module using the \ref zpc_config and register the
 * Serial file descriptor and Z-Wave RX Process to \ref contiki.
 *
 * @returns SL_STATUS_OK if successful and SL_STATUS_FAIL if an error occurred
 */
sl_status_t zwave_rx_fixt_setup(void);

/** Teardown fixture for the Z-Wave RX.
 *
 * This setup function will shutdown the communication with the Z-Wave
 * serial device, free resources and return.
 *
 * @returns This always return 0.
 */
int zwave_rx_fixt_teardown(void);

#ifdef __cplusplus
}
#endif

#endif  // ZWAVE_RX_FIXT_H
        /** @} end of zwave_rx */
