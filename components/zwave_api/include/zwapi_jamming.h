/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
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
 * @file zwapi_jamming.h
 * @brief Serial API wrapper for NCP jamming detection (FUNC_ID 0xF1).
 *
 * The NCP samples background RSSI per physical RF channel and pushes
 * unsolicited reports when a jamming condition is detected or cleared, and
 * when RSSI collection data is available. This module exposes the three
 * configuration write commands and registers the notification callback.
 */

#ifndef ZWAPI_JAMMING_H
#define ZWAPI_JAMMING_H

#include <stdint.h>
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** FUNC_ID assigned to the jamming detection proprietary command (0xF1). */
#define FUNC_ID_JAMMING_DETECTION 0xF1U

/** Number of RF channels reported by the NCP (channels 0–4). */
#define ZWAPI_JAMMING_CHANNEL_COUNT 5U

/**
 * @brief Jamming detection sub-commands (byte following FUNC_ID in the frame).
 */
typedef enum {
    JAMMING_SUB_CMD_REPORT      = 0x00,  ///< NCP → host: jamming state changed or periodic re-report.
    JAMMING_SUB_CMD_COLLECTION  = 0x01,  ///< Bidirectional: host writes to start collection; NCP sends unsolicited RSSI snapshots.
    JAMMING_SUB_CMD_CHANNEL_CFG = 0x02,  ///< Host → NCP: set threshold and trigger for one channel.
    JAMMING_SUB_CMD_PERIOD_CFG  = 0x03,  ///< Host → NCP: set periodic re-report interval while jamming persists.
} jamming_sub_cmd_t;

/**
 * @brief Callback invoked for every unsolicited jamming notification from the NCP.
 *
 * Called from the zwave_rx polling thread.  Implementations must be
 * non-blocking.
 *
 * @param sub_cmd  Sub-command identifying the notification (0x00 or 0x01).
 * @param data     Payload bytes that follow the sub-command byte.
 * @param len      Number of bytes in @p data.
 */
typedef void (*zwapi_jamming_notification_callback_t)(uint8_t sub_cmd, const uint8_t *data, uint8_t len);

/**
 * @brief Register the callback for unsolicited jamming notifications.
 *
 * Only one callback is supported. Passing NULL clears the registration.
 *
 * @param cb Callback function, or NULL to deregister.
 */
void zwapi_jamming_register_notification_callback(zwapi_jamming_notification_callback_t cb);

/**
 * @brief Configure jamming detection parameters for a single channel.
 *
 * Sends FUNC_ID_JAMMING_DETECTION / JAMMING_SUB_CMD_CHANNEL_CFG (0x02) and
 * waits for the NCP acknowledgement.
 *
 * @param channel               Channel index (0–4).
 * @param threshold_dbm         RSSI threshold in dBm (−128–0). Samples at or above
 *                              this level count toward the trigger.
 * @param nb_samples            Number of samples that must exceed the threshold to
 *                              trigger a jamming event (1–150).
 * @param[out] echoed_channel       Channel echoed by the NCP.
 * @param[out] echoed_threshold_dbm Threshold echoed by the NCP.
 * @param[out] echoed_nb_samples    Trigger sample count echoed by the NCP.
 * @return SL_STATUS_OK if the NCP echoed the configuration back (accepted).
 * @return SL_STATUS_FAIL if the NCP did not respond or rejected the request.
 */
sl_status_t zwapi_jamming_set_channel_config(uint8_t channel, int8_t threshold_dbm, uint8_t nb_samples, uint8_t *echoed_channel, int8_t *echoed_threshold_dbm, uint8_t *echoed_nb_samples);

/**
 * @brief Set the periodic re-report interval while a jamming condition persists.
 *
 * Sends FUNC_ID_JAMMING_DETECTION / JAMMING_SUB_CMD_PERIOD_CFG (0x03).
 *
 * @param period_secs              Interval in seconds (0–20). 0 disables periodic re-reporting.
 * @param[out] echoed_period_secs  Period echoed by the NCP.
 * @return SL_STATUS_OK on success, SL_STATUS_FAIL on failure.
 */
sl_status_t zwapi_jamming_set_report_period(uint16_t period_secs, uint16_t *echoed_period_secs);

/**
 * @brief Start or stop periodic RSSI collection on the NCP.
 *
 * Sends FUNC_ID_JAMMING_DETECTION / JAMMING_SUB_CMD_COLLECTION (0x01).
 * After this call, the NCP sends one unsolicited collection report per 100 ms
 * period for @p period_100ms periods.
 *
 * @param period_100ms              Number of 100 ms collection periods.
 *                                  0 stops collection, 1–65534 sets a finite count,
 *                                  0xFFFF (65535) starts continuous collection.
 * @param[out] echoed_period_100ms  Period count echoed by the NCP.
 * @return SL_STATUS_OK on success, SL_STATUS_FAIL on failure.
 */
sl_status_t zwapi_jamming_set_rssi_collection(uint16_t period_100ms, uint16_t *echoed_period_100ms);

/**
 * @brief Dispatch an incoming FUNC_ID_JAMMING_DETECTION REQUEST frame.
 *
 * Called exclusively by the RX dispatcher (zwapi_protocol_rx_dispatch.c).
 * Do not call directly.
 *
 * @param data Raw payload bytes starting at the sub-command byte.
 * @param len  Number of bytes in @p data (includes the sub-command byte).
 */
void zwapi_jamming_handle_notification(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif  // ZWAPI_JAMMING_H
