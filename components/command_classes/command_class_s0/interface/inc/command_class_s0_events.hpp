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

#ifndef COMMAND_CLASS_S0_EVENTS_H
#define COMMAND_CLASS_S0_EVENTS_H

#include <stdint.h>

enum class command_class_s0_events_t : uint32_t { COMMAND_CLASS_S0_BASE_EVENT = (152 << 8), COMMAND_CLASS_S0_COMMANDS_SUPPORTED_GET, COMMAND_CLASS_S0_COMMANDS_SUPPORTED_REPORT, COMMAND_CLASS_S0_GET_SUPPORTED_COMMAND_CLASSES };

#endif  // COMMAND_CLASS_S0_EVENTS_H
