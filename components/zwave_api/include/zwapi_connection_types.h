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
 * @file zwapi_init.h
 * @brief Z-Wave API initialization functions.
 *
 */

#ifndef ZWAPI_CONNECTION_TYPES_H
#define ZWAPI_CONNECTION_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zwapi_connection_params {
        const char *serial_port;
        const char *ip_address;
        const int ip_port;
} zwapi_connection_params_t;

typedef struct zwapi_connection_interface {
        int (*init)(const zwapi_connection_params_t *connection_params);
        void (*close)();
        int (*restart)();
        int (*get_byte)(uint8_t *c);
        void (*put_byte)(uint8_t c);
        int (*get_buffer)(uint8_t *c, int len);
        void (*put_buffer)(uint8_t *c, int len);
        bool (*is_file_available)();
        void (*drain_buffer)();
} zwapi_connection_interface_t;

#ifdef __cplusplus
}
#endif

#endif  // ZWAPI_CONNECTION_TYPES_H
