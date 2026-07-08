#ifndef ZWAVE_RX_PROCESS_H
#define ZWAVE_RX_PROCESS_H

#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Initialize and start the Z-Wave RX process thread
 * \param fd The file descriptor of the Z-Wave API
 * \return SL_STATUS_OK on success, error code otherwise
 */
sl_status_t zwave_rx_process_init_and_start(int fd);

/**
 * \brief Stop and cleanup the Z-Wave RX process thread
 * \return SL_STATUS_OK on success, error code otherwise
 */
sl_status_t zwave_rx_process_stop_and_cleanup(void);

/**
 * \brief Request a poll of the Z-Wave API
 */
void zwave_rx_process_request_poll(void);

#ifdef __cplusplus
}
#endif

#endif  // ZWAVE_RX_PROCESS_H