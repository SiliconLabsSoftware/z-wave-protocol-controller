/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMMAND_CLASS_VERSION_EVENTS_H
#define COMMAND_CLASS_VERSION_EVENTS_H

#include <stdint.h>

enum class command_class_version_events_t : uint32_t {
    COMMAND_CLASS_VERSION_BASE_EVENT = (134 << 8),
    COMMAND_CLASS_VERSION_CC_GET,
    COMMAND_CLASS_VERSION_CC_REPORT,
    COMMAND_CLASS_VERSION_CAPABILITIES_GET,
    COMMAND_CLASS_VERSION_CAPABILITIES_REPORT,
    COMMAND_CLASS_GET_VERSION_REPORT,
    COMMAND_CLASS_VERSION_GET_INTERVIEW,
    COMMAND_CLASS_VERSION_CAPABILITIES_GET_INTERVIEW,
    COMMAND_CLASS_VERSION_ZWAVE_SOFTWARE_GET_INTERVIEW,
    COMMAND_CLASS_VERSION_REPORT_RECEIVED
};

#endif  // COMMAND_CLASS_VERSION_EVENTS_H
