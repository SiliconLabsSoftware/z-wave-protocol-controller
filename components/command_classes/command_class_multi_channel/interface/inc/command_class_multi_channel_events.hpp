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

#ifndef COMMAND_CLASS_MULTI_CHANNEL_EVENTS_H
#define COMMAND_CLASS_MULTI_CHANNEL_EVENTS_H

#include <stdint.h>

enum class command_class_multi_channel_events_t : uint32_t {
    COMMAND_CLASS_MULTI_CHANNEL_BASE_EVENT = (96 << 8),
    COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_GET,
    COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT,
    COMMAND_CLASS_MULTI_CHANNEL_POLL_ENDPOINT_CAPABILITIES,
    COMMAND_CLASS_MULTI_CHANNEL_END_POINT_FIND,
    COMMAND_CLASS_MULTI_CHANNEL_END_POINT_FIND_REPORT,
    COMMAND_CLASS_MULTI_CHANNEL_GET_LIST_OF_ENDPOINTS,
    COMMAND_CLASS_MULTI_CHANNEL_END_POINT_GET_INTERVIEW,
    COMMAND_CLASS_MULTI_CHANNEL_END_POINT_REPORT_RECEIVED
};

#endif  // COMMAND_CLASS_MULTI_CHANNEL_EVENTS_H
