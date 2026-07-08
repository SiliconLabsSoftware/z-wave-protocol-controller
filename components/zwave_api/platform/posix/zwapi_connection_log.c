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

#include "log.h"
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#define LOG_TAG "zwapi_connection_log"

FILE *log_fd_connection = NULL;
struct timeval last_time;

static void zwapi_connection_log_timestamp(const struct timeval *cur_time)
{
    if (log_fd_connection) {
        char timebuf[28];
        struct tm timeinfo;
        localtime_r(&cur_time->tv_sec, &timeinfo);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        fprintf(log_fd_connection, "%s:%06d", timebuf, (int)cur_time->tv_usec);
    }
}

sl_status_t zwapi_connection_log_to_file_enable(const char *filename)
{
    // ignore empty filename, as this is the default config for not enabling log
    if (strcmp(filename, "") == 0) {
        return SL_STATUS_OK;
    }
    if (log_fd_connection) {
        sl_log_error(LOG_TAG, "Tried to enable log to file, while it is already enabled");
        return SL_STATUS_ALREADY_INITIALIZED;
    }
    log_fd_connection = fopen(filename, "a");
    if (NULL == log_fd_connection) {
        sl_log_error(LOG_TAG, "Failed to open file '%s' Error: %s", filename, strerror(errno));
        return SL_STATUS_FAIL;
    }
    fprintf(log_fd_connection, "\n");
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    zwapi_connection_log_timestamp(&cur_time);
    fprintf(log_fd_connection, " ============== Start of new log ==============\n");
    return SL_STATUS_OK;
}

sl_status_t zwapi_connection_log_to_file_disable()
{
    if (log_fd_connection) {
        fflush(log_fd_connection);
        fclose(log_fd_connection);
        log_fd_connection = NULL;
    }
    return SL_STATUS_OK;
}

sl_status_t zwapi_connection_log_to_file(const uint8_t *buffer, int length, bool direction, bool newline)
{
    if (!log_fd_connection) {
        // If log is not enabled, do nothing
        return SL_STATUS_OK;
    }

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    if (newline) {
        fprintf(log_fd_connection, "\n");
        zwapi_connection_log_timestamp(&cur_time);
        fprintf(log_fd_connection, direction ? " W " : " R ");
        fflush(log_fd_connection);
    }
    for (int i = 0; i < length; i++) {
        fprintf(log_fd_connection, "%02x ", (unsigned int)(buffer[i] & 0xFF));
    }
    last_time = cur_time;
    return SL_STATUS_OK;
}