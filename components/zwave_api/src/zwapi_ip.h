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
 * @file zwapi_ip.h
 * @brief IP API for the Z-Wave module.
 * Used for low level read/write functions to the IP port.
 */

#ifndef ZWAPI_IP_H
#define ZWAPI_IP_H

#include <stdint.h>
#include <stdbool.h>
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

// CLEANUP: Should really be private to zwapi, but zw_programmer.c and
// ZWFirmwareUpdate.c calls these functions directly.

/**
 * @brief Initialize the IP port
 * @returns the file descriptor for the IP device.
 *
 * aka SerialInit
 */
int zwapi_ip_init(const char *ip_address, const int ip_port);

/**
 * @brief De-Initialize the serial port.
 *
 * aka SerialClose
 */
void zwapi_ip_close(void);

/**
 * @brief Closes and re-initialize the last open serial port.
 * @returns the file descriptor for the serial device.
 *
 * aka SerialRestart
 */
int zwapi_ip_restart(void);

/**
 * @brief Reads a byte from the serial port device.
 * @param c char  to store data from serial port to.
 * @returns 0 if no characters are available, otherwise this returns 1
 * (length of the read data)
 *
 * aka SerialGetByte
 */
int zwapi_ip_get_byte(uint8_t *c);

/**
 * @brief Write a byte to serial port device.
 *
 * aka SerialPutByte
 */
void zwapi_ip_put_byte(uint8_t c);

/**
 * @brief Reads the output of the serial port device and stores the content in
 * a buffer.
 * @param c buffer to store data from serial port to.
 * @param len length of buffer.
 * @returns The length of the data copied in the buffer.
 *
 * aka SerialGetBuffer
 */
int zwapi_ip_get_buffer(uint8_t *c, int len);

/**
 * @brief Writes the content of a buffer to the serial port device.
 *
 * @param c buffer with data to write to serial port.
 * @param len length of data in buffer.
 *
 * aka SerialPutBuffer
 */
void zwapi_ip_put_buffer(uint8_t *c, int len);

/**
 * @brief Check the file descriptor for the serial device is available for I/O
 * operations.
 *
 * @returns true if the file is available, false if not or if an error occurred.
 *
 * aka SerialCheck
 */
bool zwapi_ip_is_file_available(void);

/**
 * @brief Flush the serial output if using buffered output.
 *
 * This function calls tcdrain, which waits that the TTY buffer data has been
 * written to the hardware.
 *
 * aka SerialFlush
 */
void zwapi_ip_drain_buffer(void);

/**
 * @brief Disable/stop logging serial data to file.
 *
 * This will stop the logging of serial data to file.
 *
 * @return \ref SL_STATUS_OK on success
 */
sl_status_t zwapi_ip_log_to_file_disable();

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
sl_status_t zwapi_ip_log_to_file_enable(const char *filename);

/**
 * @brief Start the websocket keepalive thread
 *
 * This function starts a background thread that sends a keepalive byte (0)
 * to the websocket every 5 seconds to prevent connection timeouts.
 *
 * @return 0 on success, -1 on failure
 */
int zwapi_ip_start_keepalive_thread(void);

/**
 * @brief Stop the websocket keepalive thread
 *
 * This function stops the background keepalive thread.
 */
void zwapi_ip_stop_keepalive_thread(void);

/**
 * @brief Manually send a keepalive message
 *
 * This function can be called to manually send a keepalive byte (0)
 * to the websocket. Useful for testing or immediate keepalive.
 *
 * @return 0 on success, -1 if socket is not available
 */
int zwapi_ip_send_keepalive_now(void);

#ifdef __cplusplus
}
#endif

#endif  // ZWAPI_IP_H
