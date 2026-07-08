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
 * @file zwapi_connection.h
 * @brief API to connect and send frames to the Z-Wave module
 * using the serial port.
 */

#ifndef ZWAPI_CONNECTION_H
#define ZWAPI_CONNECTION_H

#include "zwapi_internal.h"
#include "zwapi_connection_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Timeout values, in ms
#define RX_ACK_TIMEOUT_DEFAULT  1500
#define RX_BYTE_TIMEOUT_DEFAULT 1500

/// State of the connection to the Z-Wave module.
typedef enum {
    ZWAPI_CONNECTION_STATUS_IDLE,            ///< Nothing has happened (no tx, no rx)
    ZWAPI_CONNECTION_STATUS_FRAME_RECEIVED,  ///< A valid frame has been received
    ZWAPI_CONNECTION_STATUS_FRAME_SENT,      ///< A frame was sent successfully and ACKed by the other end
    ZWAPI_CONNECTION_STATUS_CHECKSUM_ERROR,  ///< A frame has an incorrect Checksum
    ZWAPI_CONNECTION_STATUS_RX_TIMEOUT,      ///< Rx timeout has happened
    ZWAPI_CONNECTION_STATUS_TX_TIMEOUT,      ///< Tx timeout (waiting for ACK) has happened
    ZWAPI_CONNECTION_STATUS_TX_NAK,          ///< A frame was sent and the other end issued a NAK
    ZWAPI_CONNECTION_STATUS_TX_CAN,          ///< A frame was sent and the other end issued a CAN, i.e. a collision occurred
} zwapi_connection_status_t;

/**
 * @brief Initialize the zwapi_connection to the Z-Wave module.
 *
 * @param connection_params A pointer to a zwapi_connection_params_t structure
 * containing the connection parameters.
 * @returns integer file descriptor for the serial device.
 * the value will be greater than 0 if successful.
 *
 * aka ConInit
 */
int zwapi_connection_init(const zwapi_connection_params_t *connection_params);

/**
 * @brief Close the zwapi_connection to the Z-Wave module.
 *
 * aka ConDestroy
 */
void zwapi_connection_shutdown();

/**
 * @brief Closes and re-initializes the zwapi_connection to the Z-Wave module.
 * @returns integer file descriptor for the serial device.
 * the value will be greater than 0 if successful.
 *****************************************************************************/
int zwapi_connection_restart();

/**
 * @brief Transmit a frame via serial port by adding SOF, Len, Type, cmd and Checksum.
 *
 * @param cmd The command to be executed by the Z-Wave module, using FUNC_ID
 * defines
 * @param type The type of frame, either Request 0x00 or Response 0x01
 * @param Buf A pointer to the command payload buffer
 * @param len The length of the data contained in the payload buffer
 * @param ack_needed true if we expect an Ack back for this frame.
 *
 * A frame on the serial line consist of:
 * <b><tt>SOF-Len-Type-Cmd-DATA-Chksum</tt></b>, where:
 *  - @b SOF is Start of frame byte
 *  - @b Len is length of bytes between and including Len to last DATA byte
 *  - @b Type is Response or Request
 *  - @b Cmd Serial application command describing what DATA is
 *  - @b DATA as it says
 *  - @b Chksum is a XOR checksum taken on all BYTEs from Len to last DATA byte
 *
 * aka ConTxFrame
 */
void zwapi_connection_tx(uint8_t cmd, uint8_t type, const uint8_t *Buf, uint8_t len, bool ack_needed);

/**
 * @brief Parses serial data sent from the Z-Wave module to the serial port.
 * Should be frequently called by main loop.
 * @returns The current connection state. Refer to zwapi_connection_status_t values
 *
 * aka ConUpdate
 */
zwapi_connection_status_t zwapi_connection_refresh();

/**
 * @brief Provides the data of the serial buffer for the last received frame.
 * @param user_buffer a pointer to the user buffer in which the frame is to be copied
 * @param user_buffer_length The length of the user buffer
 * @returns The length of the data copied in the user buffer.
 */
int zwapi_connection_get_last_rx_frame(uint8_t *user_buffer, int user_buffer_length);

/**
 * @brief make the zwapi_connection_status types human readable.
 * @returns A string representation of a zwapi_connection_status_t value
 *
 * aka ConTypeToStr
 */
const char *zwapi_connection_status_to_string(zwapi_connection_status_t t);

/**
 * @brief Disable/stop logging serial data to file.
 *
 * This will stop the logging of serial data to file.
 *
 * @return \ref SL_STATUS_OK on success
 */
sl_status_t zwapi_connection_log_to_file_disable();

/**
 * @brief Enable logging of serial data to a file.
 *
 * The log will append to the file, it is up to the user to handle log rotation,
 * free disk space monitoring etc.
 *
 * @param filename file to log serial trace to, log will append to this file.
 * @return \ref SL_STATUS_OK on success.
 * @return \ref SL_STATUS_ALREADY_INITIALIZED if log to file is already enabled.
 * @return \ref SL_STATUS_FAIL if other failure (e.g. folder doesn't exist,
 *              lacking permissions, etc.).
 */
sl_status_t zwapi_connection_log_to_file_enable(const char *filename);

/**
 * @brief Log serial data to file.
 *
 * @param buffer pointer to the buffer to log
 * @param length length of the buffer to log
 * @param direction true for outgoing data, false for incoming data
 * @param newline true to start a new log line with timestamp, false to continue on same line
 * @return \ref SL_STATUS_OK on success
 */
sl_status_t zwapi_connection_log_to_file(uint8_t *buffer, int length, bool direction, bool newline);

// Direction macros for better readability
#define ZWAPI_LOG_DIRECTION_OUTGOING true   //< Data being sent/written (marked with 'W')
#define ZWAPI_LOG_DIRECTION_INCOMING false  //< Data being received/read (marked with 'R')

// Newline macros for better readability
#define ZWAPI_LOG_NEW_LINE      true   //< Start new line with timestamp and direction
#define ZWAPI_LOG_CONTINUE_LINE false  //< Continue on same line (append data)

// Convenience macros for common logging patterns
#define zwapi_log_tx_start(buffer, length) zwapi_connection_log_to_file(buffer, length, ZWAPI_LOG_DIRECTION_OUTGOING, ZWAPI_LOG_NEW_LINE)

#define zwapi_log_tx_continue(buffer, length) zwapi_connection_log_to_file(buffer, length, ZWAPI_LOG_DIRECTION_OUTGOING, ZWAPI_LOG_CONTINUE_LINE)

#define zwapi_log_rx_start(buffer, length) zwapi_connection_log_to_file(buffer, length, ZWAPI_LOG_DIRECTION_INCOMING, ZWAPI_LOG_NEW_LINE)

#define zwapi_log_rx_continue(buffer, length) zwapi_connection_log_to_file(buffer, length, ZWAPI_LOG_DIRECTION_INCOMING, ZWAPI_LOG_CONTINUE_LINE)

#ifdef __cplusplus
}
#endif

#endif  // ZWAPI_CONNECTION_H
