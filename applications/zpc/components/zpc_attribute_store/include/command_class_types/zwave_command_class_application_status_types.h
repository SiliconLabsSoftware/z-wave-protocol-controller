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


#ifndef ZWAVE_COMMAND_CLASS_APPLICATION_STATUS_TYPES_H
#define ZWAVE_COMMAND_CLASS_APPLICATION_STATUS_TYPES_H

#include <stdint.h>

///> Application busy status. uint8_t
typedef uint8_t application_busy_status;

///> Application busy wait time. uint8_t
typedef uint8_t application_busy_wait_time;

///> Application reject request. uint8_t
typedef uint8_t application_reject_request;

typedef enum {
    TRY_AGAIN_LATER = 0,
    TRY_AGAIN_WAIT_TIME = 1,
    REQUEST_QUEUED = 2
} application_busy_status_enum;

#endif  //ZWAVE_COMMAND_CLASS_APPLICATION_STATUS_TYPES_H
/** @} end zwave_command_class_application_status_types */
