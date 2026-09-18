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

#ifndef ZWAVE_CMD_CLASS_EVENTS_H
#define ZWAVE_CMD_CLASS_EVENTS_H

#include <stdint.h>

enum class command_class_zwave_cmd_class_events_t : uint32_t {
    ZWAVE_CMD_CLASS_BASE_EVENT = (1 << 8),
    COMMAND_CLASS_ZWAVE_CMD_CLASS_COMMANDS_REQUEST_NODE_INFO,
};

#endif  // ZWAVE_CMD_CLASS_EVENTS_H
