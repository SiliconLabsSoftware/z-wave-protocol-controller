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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include "zwapi_serial.h"
// #include "clock.h"
#include "log.h"
#include "zwapi_connection.h"

#define LOG_TAG "zwapi_serial"

static int serial_fd;

// Stores the last used serial port
static char last_used_serial_port[PATH_MAX] = {0};

// Previously named OpenSerialPort in the legacy SerialAPI module
/**
 * Open a serial device and configure it.
 * Returns the file descriptor associated to the device.
 */
static int zwapi_open_serial_port(const char *serial_device_path)
{
    // Open the serial port read/write, with no controlling terminal,
    // and don't wait for a connection. The O_NONBLOCK flag also causes
    // subsequent I/O on the device to be non-blocking.
    // See open(2) ("man 2 open") for details.

    int file_descriptor = -1;
    file_descriptor     = open(serial_device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (file_descriptor == -1 || flock(file_descriptor, LOCK_EX) == -1) {
        sl_log_warning(LOG_TAG, "Error opening serial port %s - %s(%d).\n", serial_device_path, strerror(errno), errno);
        goto error;
    }

    // Now that the device is open, clear the O_NONBLOCK flag so
    // subsequent I/O will block. See fcntl(2) ("man 2 fcntl") for details.
    if (fcntl(file_descriptor, F_SETFL, 0) == -1) {
        sl_log_critical(LOG_TAG, "Error clearing O_NONBLOCK %s - %s(%d).\n", serial_device_path, strerror(errno), errno);
        goto error;
    }

    if (ioctl(file_descriptor, TIOCEXCL, (char *)0) < 0) {
        sl_log_critical(LOG_TAG, "Error setting TIOCEXCL %s - %s(%d).\n", serial_device_path, strerror(errno), errno);
        goto error;
    }

    struct termios options;
    memset(&options, 0, sizeof(options));
    // The baud rate, word length, and handshake options can be set as follows:
    options.c_iflag     = 0;
    options.c_oflag     = 0;
    options.c_cflag     = CS8 | CREAD | CLOCAL;  // 8n1, see termios.h for more information
    options.c_lflag     = 0;
    options.c_cc[VMIN]  = 1;
    options.c_cc[VTIME] = 5;
    cfsetospeed(&options, B115200);  // Set 115200 baud
    cfsetispeed(&options, B115200);

    // Apply the new options
    if (tcsetattr(file_descriptor, TCSANOW, &options) == -1) {
        printf("Error setting tty attributes %s - %s(%d).\n", serial_device_path, strerror(errno), errno);
        goto error;
    }

    // Success
    return file_descriptor;

    // Failure path
error:
    if (file_descriptor != -1) {
        close(file_descriptor);
    }
    // Stop the application, as nothing will work.
    return -1;
}

int zwapi_serial_init(const char *port)
{
    serial_fd = zwapi_open_serial_port(port);
    if (0 < serial_fd) {
        tcflush(serial_fd, TCIOFLUSH);
        snprintf(last_used_serial_port, sizeof(last_used_serial_port), "%s", port);
    }
    return serial_fd;
}

void zwapi_serial_close(void)
{
    flock(serial_fd, LOCK_UN);
    close(serial_fd);
}

int zwapi_serial_restart(void)
{
    if (last_used_serial_port[0]) {
        zwapi_serial_close();
        return zwapi_serial_init(last_used_serial_port);
    }
    return 0;
}

int zwapi_serial_get_byte(uint8_t *c)
{
    return zwapi_serial_get_buffer(c, 1);
}

void zwapi_serial_put_byte(uint8_t c)
{
    zwapi_serial_put_buffer(&c, 1);
}

int zwapi_serial_get_buffer(uint8_t *c, int len)
{
    int k = 0;

    while (k < len) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(serial_fd, &rfds);
        struct timeval tv = {
          .tv_sec  = RX_BYTE_TIMEOUT_DEFAULT / 1000,
          .tv_usec = (RX_BYTE_TIMEOUT_DEFAULT % 1000) * 1000,
        };
        int sel = select(serial_fd + 1, &rfds, NULL, NULL, &tv);
        if (sel < 0) {
            if (errno == EINTR) {
                continue;
            }
            sl_log_warning(LOG_TAG, "Serial select error: %s\n", strerror(errno));
            return k;
        }
        if (sel == 0) {
            sl_log_warning(LOG_TAG, "Serial read timeout after %d/%d bytes\n", k, len);
            return k;
        }
        int res = read(serial_fd, c + k, len - k);
        if (res <= 0) {
            sl_log_warning(LOG_TAG, "Serial read error: %s\n", strerror(errno));
            return k;
        }
        k += res;
    }

    return k;
}

void zwapi_serial_put_buffer(uint8_t *c, int len)
{
    int n = 0;
    do {
        int res = write(serial_fd, c, len);
        if (res < 0) {
            sl_log_error(LOG_TAG, "Serial Write Error: %s", strerror(errno));
        } else {
            n += res;
            if (n == len) {
                break;
            }
        }
    } while (errno == EAGAIN);

    // Log to file
    zwapi_log_tx_start(c, n);
}

bool zwapi_serial_is_file_available(void)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(serial_fd, &rfds);

    // From the manpage:
    // If both fields of the timeval structure are zero, then
    // select() returns immediately.  (This is useful for polling.)
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    retval = select(serial_fd + 1, &rfds, NULL, NULL, &tv);
    // Don't rely on the value of tv now!
    if (retval == -1) {
        perror("select()");
        return false;
    }
    if (retval > 0) {
        return true;
    }
    return false;
}

void zwapi_serial_drain_buffer(void)
{
    if (tcdrain(serial_fd)) {
        sl_log_error(LOG_TAG, "Unable to drain serial buffer. Target might be dead....\n");
    }
}
