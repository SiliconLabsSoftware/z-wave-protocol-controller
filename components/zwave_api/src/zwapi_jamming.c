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

#include "zwapi_jamming.h"
#include "zwapi_internal.h"
#include "zwapi_session.h"
#include "log.h"
#include <string.h>

#define LOG_TAG "zwapi_jamming"

static zwapi_jamming_notification_callback_t notification_callback = NULL;

void zwapi_jamming_register_notification_callback(zwapi_jamming_notification_callback_t cb)
{
    notification_callback = cb;
}

static sl_status_t jamming_send_and_read_echo(const uint8_t *payload, uint8_t payload_len, uint8_t *echo)
{
    uint8_t response[16] = {0};
    uint8_t response_len = sizeof(response);
    sl_status_t status   = zwapi_send_command_with_response(FUNC_ID_JAMMING_DETECTION, payload, payload_len, response, &response_len);
    if (status != SL_STATUS_OK) {
        return status;
    }
    if (response_len < (uint8_t)(IDX_DATA + payload_len) || response[IDX_DATA] != payload[0]) {
        sl_log_warning(LOG_TAG, "Jamming command echo missing or mismatched (sub-cmd 0x%02x, len %u)\n", payload[0], (unsigned)response_len);
        return SL_STATUS_FAIL;
    }
    memcpy(echo, &response[IDX_DATA], payload_len);
    return SL_STATUS_OK;
}

sl_status_t zwapi_jamming_set_channel_config(uint8_t channel, int8_t threshold_dbm, uint8_t nb_samples, uint8_t *echoed_channel, int8_t *echoed_threshold_dbm, uint8_t *echoed_nb_samples)
{
    uint8_t payload[]  = {JAMMING_SUB_CMD_CHANNEL_CFG, channel, (uint8_t)threshold_dbm, nb_samples};
    uint8_t echo[4]    = {0};
    sl_status_t status = jamming_send_and_read_echo(payload, sizeof(payload), echo);
    if (status == SL_STATUS_OK) {
        *echoed_channel       = echo[1];
        *echoed_threshold_dbm = (int8_t)echo[2];
        *echoed_nb_samples    = echo[3];
    }
    return status;
}

sl_status_t zwapi_jamming_set_report_period(uint16_t period_secs, uint16_t *echoed_period_secs)
{
    uint8_t payload[]  = {JAMMING_SUB_CMD_PERIOD_CFG, (uint8_t)(period_secs >> 8), (uint8_t)(period_secs & 0xFF)};
    uint8_t echo[3]    = {0};
    sl_status_t status = jamming_send_and_read_echo(payload, sizeof(payload), echo);
    if (status == SL_STATUS_OK) {
        *echoed_period_secs = ((uint16_t)echo[1] << 8) | echo[2];
    }
    return status;
}

sl_status_t zwapi_jamming_set_rssi_collection(uint16_t period_100ms, uint16_t *echoed_period_100ms)
{
    uint8_t payload[]  = {JAMMING_SUB_CMD_COLLECTION, (uint8_t)(period_100ms >> 8), (uint8_t)(period_100ms & 0xFF)};
    uint8_t echo[3]    = {0};
    sl_status_t status = jamming_send_and_read_echo(payload, sizeof(payload), echo);
    if (status == SL_STATUS_OK) {
        *echoed_period_100ms = ((uint16_t)echo[1] << 8) | echo[2];
    }
    return status;
}

void zwapi_jamming_handle_notification(const uint8_t *data, uint16_t len)
{
    if (len < 1) {
        sl_log_warning(LOG_TAG, "Jamming notification too short (%u bytes), ignoring\n", (unsigned)len);
        return;
    }
    if (notification_callback != NULL) {
        notification_callback(data[0], data + 1, (uint8_t)(len - 1));
    }
}
