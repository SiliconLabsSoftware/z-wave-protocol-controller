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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "zwapi_ip.h"
// #include "clock.h"
#include "log.h"
#include "zwapi_connection.h"

#define LOG_TAG "zwapi_ip"

static int socket_fd;

// Stores the last used IP port
static char last_used_ip_port[PATH_MAX] = {0};
static int last_used_ip_port_port       = 0;

// Previously named OpenSerialPort in the legacy SerialAPI module
/**
 * Open a serial device and configure it.
 * Returns the file descriptor associated to the device.
 */
static int zwapi_open_ip_port(const char *ip_address, const int ip_port)
{
    // Open the serial port read/write, with no controlling terminal,
    // and don't wait for a connection. The O_NONBLOCK flag also causes
    // subsequent I/O on the device to be non-blocking.
    // See open(2) ("man 2 open") for details.
    int file_descriptor = -1;
    file_descriptor     = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (file_descriptor == -1) {
        sl_log_critical(LOG_TAG, "Error opening socket - %s(%d).\n", strerror(errno), errno);
        goto error;
    }

    // Disable Nagle's algorithm
    int on = 1;
    setsockopt(file_descriptor, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

    struct sockaddr_in server_address;
    server_address.sin_family      = AF_INET;
    server_address.sin_port        = htons(ip_port);
    server_address.sin_addr.s_addr = inet_addr(ip_address);

    int ret = connect(file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address));
    if (ret == -1) {
        sl_log_critical(LOG_TAG, "Error connecting to IP port %s - %s(%d).\n", ip_address, strerror(errno), errno);
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

int zwapi_ip_init(const char *ip_address, const int ip_port)
{
    socket_fd = zwapi_open_ip_port(ip_address, ip_port);
    if (0 < socket_fd) {
        snprintf(last_used_ip_port, sizeof(last_used_ip_port), "%s", ip_address);
        last_used_ip_port_port = ip_port;
    }
    return socket_fd;
}

void zwapi_ip_close(void)
{
    flock(socket_fd, LOCK_UN);
    close(socket_fd);
}

int zwapi_ip_restart(void)
{
    if (last_used_ip_port[0]) {
        zwapi_ip_close();
        return zwapi_ip_init(last_used_ip_port, last_used_ip_port_port);
    }
    return 0;
}

int zwapi_ip_get_byte(uint8_t *c)
{
    return zwapi_ip_get_buffer(c, 1);
}

void zwapi_ip_put_byte(uint8_t c)
{
    zwapi_ip_put_buffer(&c, 1);
}

int zwapi_ip_get_buffer(uint8_t *c, int len)
{
    int res = recv(socket_fd, c, len, MSG_WAITALL);
    if (res <= 0) {
        sl_log_error(LOG_TAG, "IP Read Error: %s | Returned %d", strerror(errno), res);
        exit(1);
    }

    return res;
}

void zwapi_ip_put_buffer(uint8_t *c, int len)
{
    int res = send(socket_fd, c, len, 0);
    if (res < 0) {
        sl_log_error(LOG_TAG, "IP Write Error: %s", strerror(errno));
    } else if (res != len) {
        sl_log_error(LOG_TAG, "IP Write Error: %d bytes written, expected %d bytes", res, len);
        exit(1);
    }

    // Log to file
    zwapi_log_tx_start(c, res);
}

bool zwapi_ip_is_file_available(void)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(socket_fd, &rfds);

    // From the manpage:
    // If both fields of the timeval structure are zero, then
    // select() returns immediately.  (This is useful for polling.)
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    retval = select(socket_fd + 1, &rfds, NULL, NULL, &tv);
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

void zwapi_ip_drain_buffer(void)
{
    // IP was written with blocking send, so no need to drain buffer
}