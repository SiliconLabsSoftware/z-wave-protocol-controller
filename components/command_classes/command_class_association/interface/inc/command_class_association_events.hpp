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

#ifndef COMMAND_CLASS_ASSOCIATION_EVENTS_H
#define COMMAND_CLASS_ASSOCIATION_EVENTS_H

#include <stdint.h>

enum class command_class_association_events_t : uint32_t {
    COMMAND_CLASS_ASSOCIATION_BASE_EVENT = (133 << 8),
    COMMAND_CLASS_ASSOCIATION_GROUPINGS_GET,
    COMMAND_CLASS_ASSOCIATION_GROUPINGS_REPORT,
    COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT,
    COMMAND_CLASS_ASSOCIATION_GET,
    COMMAND_CLASS_ASSOCIATION_REPORT_RECEIVED,
    COMMAND_CLASS_ASSOCIATION_SET,
};

#endif  // COMMAND_CLASS_ASSOCIATION_EVENTS_H
