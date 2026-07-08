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

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "zwapi_init.h"
#include "zwapi_session.h"
#include "zwapi_connection.h"
#include "zwapi_timestamp.h"
#include "log.h"

#define LOG_TAG "zwapi_session"

#define TIMEOUT_TIME 1600
// This number should be higher than the MAX_TRANSMISSION_RETRIES
#define MAX_RX_QUEUE_LENGTH 64
// CLEANUP: MAX_TRANSMISSION_RETRIES used to be 20 in send_frame()
// Find out what our strategy should be, INS12350 says it should be 3
#define MAX_TRANSMISSION_RETRIES           20
#define MAX_RX_FRAMES_WAITING_FOR_RESPONSE 3
#define MAX_TX_TIMEOUTS                    3

typedef struct list_elem {
        struct list_elem *next;
        uint8_t *data;
        uint8_t len;
} zwapi_session_list_elem_t;

static zwapi_session_list_elem_t *zwapi_session_rx_queue;

static pthread_mutex_t session_serial_mutex = PTHREAD_MUTEX_INITIALIZER;

// Previously named rxQueue_Len in the legacy SerialAPI module
/**
 Give the number of outstanding packets in zwapi_session_rx_queue
*****************************************************************************/
static int zwapi_session_get_rx_queue_length(void)
{
    zwapi_session_list_elem_t *q = zwapi_session_rx_queue;
    int len                      = 0;

    while (q) {
        q = q->next;
        len++;
    }
    return len;
}

// Previously named QueueFrame in the legacy SerialAPI module
/**
 * Enqueue a new element in the zwapi_session_rx_queue from the zwapi_connection
 * buffer. Only REQUEST type frames will be enqueued, others will be ignored
 * TODO: Get rid of malloc
 *****************************************************************************/
static void zwapi_session_enqueue_frame(void)
{
    zwapi_session_list_elem_t *new_elem;
    if (zwapi_session_get_rx_queue_length() >= MAX_RX_QUEUE_LENGTH) {
        sl_log_error(LOG_TAG, "Rx queue is full! Aborting\n");
        return;
    }
    new_elem       = (zwapi_session_list_elem_t *)malloc(sizeof(zwapi_session_list_elem_t));
    new_elem->data = malloc(FRAME_LENGTH_MAX * sizeof(uint8_t));
    new_elem->next = NULL;
    new_elem->len  = zwapi_connection_get_last_rx_frame(new_elem->data, FRAME_LENGTH_MAX);

    // Verify that this is a REQUEST frame (RESPONSES are not queued here)
    if (new_elem->len >= IDX_TYPE + 1) {
        if (new_elem->data[IDX_TYPE] == FRAME_TYPE_REQUEST) {
            // Add frame to end of queue
            if (zwapi_session_rx_queue != NULL) {
                zwapi_session_list_elem_t *e;
                for (e = zwapi_session_rx_queue; e->next; e = e->next) {
                    // Nothing here. Just iterating to the end of the queue.
                }
                e->next = new_elem;
            } else {
                zwapi_session_rx_queue = new_elem;
            }
            // Make an additional poll request to ensure
            // that are no pending frames unread.
            if (zwave_api_get_callbacks()->poll_request) {
                zwave_api_get_callbacks()->poll_request();
            }
        } else {  // it is a RESPONSE (or other?) frame type, do not enqueue
            free(new_elem->data);
            free(new_elem);
        }
    } else {  // Frame is too short, this should never happen
        free(new_elem->data);
        free(new_elem);
    }
}

// Previously named WaitResponse in the legacy SerialAPI module
/**
Wait for a acknowledgement (ACK, NAK, CAN) or possibly another SOF (REQ/RES)
frame from the Z-Wave serial device.
*****************************************************************************/
static zwapi_connection_status_t zwapi_session_wait_for_response(void)
{
    zwapi_connection_status_t connection_status;
    static zwapi_timestamp_t session_timer;
    zwapi_timestamp_get(&session_timer, TIMEOUT_TIME);
    while (1) {
        connection_status = zwapi_connection_refresh();
        if (connection_status != ZWAPI_CONNECTION_STATUS_IDLE) {
            break;
        }

        if (zwapi_is_timestamp_elapsed(&session_timer)) {
            sl_log_warning(LOG_TAG, "Timed out waiting for a response\n");
            connection_status = ZWAPI_CONNECTION_STATUS_RX_TIMEOUT;
            break;
        }
    }
    return connection_status;
}

int zwapi_session_init(const zwapi_connection_params_t *connection_params)
{
    return zwapi_connection_init(connection_params);
}

void zwapi_session_shutdown()
{
    zwapi_session_flush_queue();
    zwapi_connection_shutdown();
}

int zwapi_session_restart()
{
    return zwapi_connection_restart();
}

bool zwapi_session_dequeue_frame(uint8_t **frame_ptr, uint8_t *frame_len)
{
    if (zwapi_session_rx_queue) {
        zwapi_session_list_elem_t *elem = zwapi_session_rx_queue;
        zwapi_session_rx_queue          = zwapi_session_rx_queue->next;

        *frame_ptr = elem->data;
        *frame_len = elem->len;
        free(elem);

    } else {
        *frame_ptr = NULL;
        *frame_len = 0;
    }

    // Are there more elements in our session queue?
    return (zwapi_session_rx_queue) ? true : false;
}

static void enqueue_rx_frames(void)
{
    while (zwapi_connection_refresh() == ZWAPI_CONNECTION_STATUS_FRAME_RECEIVED) {
        // Enqueue available REQ frames.
        zwapi_session_enqueue_frame();
    }
}

void zwapi_session_enqueue_rx_frames()
{
    pthread_mutex_lock(&session_serial_mutex);
    enqueue_rx_frames();
    pthread_mutex_unlock(&session_serial_mutex);
}

// send_frame acquires session_serial_mutex for each individual transmit+ACK
// exchange and releases it between retries so competing threads may interleave
// during backoff.
//
// Return contract:
//   SL_STATUS_OK   – the ACK was received.  The mutex is STILL HELD on return.
//                    The caller must release it when it is done with the serial
//                    port (immediately for fire-and-forget, or after collecting
//                    the RES frame for with-response callers).
//   SL_STATUS_FAIL – all retries exhausted or hard failure.  The mutex is NOT
//                    held on return.
static sl_status_t send_frame(uint8_t command, const uint8_t *payload_buffer, uint8_t payload_buffer_length)
{
    // Drain any pending RX frames before the first attempt.
    pthread_mutex_lock(&session_serial_mutex);
    enqueue_rx_frames();
    pthread_mutex_unlock(&session_serial_mutex);

    uint8_t consecutive_tx_timeout_count = 0;
    for (int i = 0; i < MAX_TRANSMISSION_RETRIES; i++) {
        pthread_mutex_lock(&session_serial_mutex);
        zwapi_connection_tx(command, FRAME_TYPE_REQUEST, payload_buffer, payload_buffer_length, true);
        zwapi_connection_status_t connection_status = zwapi_session_wait_for_response();

        bool do_backoff = true;
        switch (connection_status) {
            case ZWAPI_CONNECTION_STATUS_FRAME_SENT:
                if (i > 0) {
                    sl_log_debug(LOG_TAG, "Serial TX recovered: cmd=0x%02x after %d retries\n", command, i);
                }
                // Return with mutex held — caller releases it.  See contract above.
                return SL_STATUS_OK;

            case ZWAPI_CONNECTION_STATUS_FRAME_RECEIVED:
                consecutive_tx_timeout_count = 0;
                zwapi_session_enqueue_frame();
                sl_log_debug(LOG_TAG, "Received a frame while trying to send\n");
                /* If we received a frame here then we were both sending. The embedded target will have
                 * queued a CAN at this point, since we have been sending a frame to the uart buffer.
                 * before ACK'ing the received frame.
                 */
                break;

            case ZWAPI_CONNECTION_STATUS_TX_CAN:
                sl_log_debug(LOG_TAG, "Frame collision detected.\n");
                consecutive_tx_timeout_count = 0;
                break;

            case ZWAPI_CONNECTION_STATUS_TX_TIMEOUT:
                // Follow up on how many consecutive TX timeout we had.
                consecutive_tx_timeout_count++;
                sl_log_warning(LOG_TAG, "Timed out waiting for ACK frame\n");
                if (consecutive_tx_timeout_count >= MAX_TX_TIMEOUTS) {
                    // We should restart the serial port
                    sl_log_warning(LOG_TAG, "Reopening serial port\n");
                    zwapi_connection_restart();
                    pthread_mutex_unlock(&session_serial_mutex);
                    return SL_STATUS_FAIL;
                }
                break;

            case ZWAPI_CONNECTION_STATUS_RX_TIMEOUT:
                // Nothing special to do here, we are missing an Ack.
                // Try again without back-off.
                do_backoff = false;
                break;

            case ZWAPI_CONNECTION_STATUS_TX_NAK:
                // The other end is unhappy about our frame.
                // Parsing went off the rails for them
                pthread_mutex_unlock(&session_serial_mutex);
                return SL_STATUS_FAIL;

            default:
                sl_log_error(LOG_TAG, "Unknown Z-Wave API connection state: %d. Ignoring\n", connection_status);
                break;
        }

        pthread_mutex_unlock(&session_serial_mutex);

        sl_log_debug(LOG_TAG, "Retransmission %d/%d of 0x%02x\n", i + 1, MAX_TRANSMISSION_RETRIES, command);

        if (do_backoff) {
            /*TODO consider to use an exponential backoff, and do not backoff until our own framehandler is idle. Also
             * the magnitude of the backoff seem very large... this is to be analyzed. */
            // CLEANUP: INS12350 says it should be 100 + i * 1000. But this is probably not doable here.
            zwapi_timestamp_t retry_timer;
            zwapi_timestamp_get(&retry_timer, 20);
            while (!zwapi_is_timestamp_elapsed(&retry_timer)) {
                pthread_mutex_lock(&session_serial_mutex);
                enqueue_rx_frames();
                pthread_mutex_unlock(&session_serial_mutex);
            }
        }
    }

    sl_log_error(LOG_TAG, "All attempts to transmit a frame have failed\n");
    return SL_STATUS_FAIL;
}

sl_status_t zwapi_session_send_frame(uint8_t command, const uint8_t *payload_buffer, uint8_t payload_buffer_length)
{
    sl_status_t status = send_frame(command, payload_buffer, payload_buffer_length);
    if (status == SL_STATUS_OK) {
        // send_frame returns with the mutex held on success; release it now
        // since this caller does not need to wait for a RES frame.
        pthread_mutex_unlock(&session_serial_mutex);
    }
    return status;
}

sl_status_t zwapi_session_send_frame_with_response(uint8_t command, const uint8_t *payload_buffer, uint8_t payload_buffer_length, uint8_t *response_buf, uint8_t *response_len)
{
    // send_frame() takes care of retries and receiving the ACK, locking
    // per attempt.  The RES wait below must be atomic with the preceding ACK
    // so it takes its own lock for the full response-collection loop.
    sl_status_t send_frame_status = send_frame(command, payload_buffer, payload_buffer_length);

    sl_status_t result = SL_STATUS_FAIL;
    if (send_frame_status != SL_STATUS_OK) {
        sl_log_warning(LOG_TAG, "The frame was not ACK'ed\n");
        return result;
    }

    // send_frame() returned SL_STATUS_OK with the mutex already held, so the
    // ACK-to-RES window is atomic — no other thread can drain the UART between
    // the ACK and the start of the RES wait.
    for (int i = 0; i < MAX_RX_FRAMES_WAITING_FOR_RESPONSE; i++) {
        zwapi_connection_status_t connection_status = zwapi_session_wait_for_response();
        if (connection_status == ZWAPI_CONNECTION_STATUS_FRAME_RECEIVED) {
            // Here we need to retrieve the frame to parse it
            // before we can decide what to do.
            uint8_t tmp_buffer[FRAME_LENGTH_MAX];
            int tmp_buffer_length = 0;
            tmp_buffer_length     = zwapi_connection_get_last_rx_frame(tmp_buffer, FRAME_LENGTH_MAX);

            if (tmp_buffer_length >= FRAME_LENGTH_MIN) {
                if (tmp_buffer[IDX_TYPE] == FRAME_TYPE_RESPONSE) {
                    if (tmp_buffer[IDX_CMD] == command) {
                        if (response_buf) {
                            memcpy(response_buf, tmp_buffer, tmp_buffer_length);
                        }
                        if (response_len) {
                            *response_len = tmp_buffer_length;
                        }
                        result = SL_STATUS_OK;
                        break;
                    } /* This if for the case where we get a callback from another function instead of a response */
                    sl_log_error(LOG_TAG,
                                 "Got new RES frame for Cmd 0x%x (not 0x%x) \
                          while sending %d\n",
                                 tmp_buffer[IDX_CMD],
                                 command,
                                 i);

                } else {  // Did we receive a REQ frame instead ? Then just enqueue it
                    zwapi_session_enqueue_frame();
                }
            } else {  // Corrupt frame data ? Just ignore it.
                sl_log_error(LOG_TAG, "Received too short frame from \
                      zwapi_connection_get_last_rx_frame()! \n");
            }
        } else {
            sl_log_warning(LOG_TAG, "Unexpected receive state! %s\n", zwapi_connection_status_to_string(connection_status));
        }
    }
    pthread_mutex_unlock(&session_serial_mutex);
    return result;
}

sl_status_t zwapi_session_send_frame_no_ack(uint8_t command, const uint8_t *payload_buffer, uint8_t payload_buffer_length)
{
    // First check for incoming frames
    pthread_mutex_lock(&session_serial_mutex);
    enqueue_rx_frames();
    // Send our command
    zwapi_connection_tx(command, FRAME_TYPE_REQUEST, payload_buffer, payload_buffer_length, false);
    pthread_mutex_unlock(&session_serial_mutex);
    return SL_STATUS_OK;
}

void zwapi_session_flush_queue(void)
{
    zwapi_session_list_elem_t *e;
    while (zwapi_session_rx_queue) {
        e                      = zwapi_session_rx_queue;
        zwapi_session_rx_queue = zwapi_session_rx_queue->next;
        free(e->data);
        free(e);
    }
}
