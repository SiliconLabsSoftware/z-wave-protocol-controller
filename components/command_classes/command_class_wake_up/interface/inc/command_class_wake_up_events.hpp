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

#ifndef COMMAND_CLASS_WAKE_UP_EVENTS_H
#define COMMAND_CLASS_WAKE_UP_EVENTS_H

#include <stdint.h>

enum class command_class_wake_up_events_t : uint32_t {
    COMMAND_CLASS_WAKE_UP_BASE_EVENT = (132 << 8),
    COMMAND_CLASS_WAKE_UP_CAPABILITIES_GET_INTERVIEW,
    COMMAND_CLASS_WAKE_UP_CAPABILITIES_REPORT_RECEIVED,
    COMMAND_CLASS_WAKE_UP_INTERVAL_GET_INTERVIEW,
    COMMAND_CLASS_WAKE_UP_INTERVAL_REPORT_RECEIVED,
    COMMAND_CLASS_WAKE_UP_INTERVAL_SET,
    /// Fired after interview-originated Interval Set resolution completes, before Interval Get is queued.
    COMMAND_CLASS_WAKE_UP_INTERVAL_SET_INTERVIEW_RESOLUTION_COMPLETED,
    COMMAND_CLASS_WAKE_UP_NOTIFICATION_RECEIVED,
    COMMAND_CLASS_WAKE_UP_INTERVAL_REQUESTED,
    /// Fired after Wake Up No More Information resolution completes (wake-up session ended).
    COMMAND_CLASS_WAKE_UP_NO_MORE_INFORMATION_SENT,
    /// Arm resolution-idle → Wake Up No More Information (e.g. NL interview start before any WUN).
    COMMAND_CLASS_WAKE_UP_ARM_NO_MORE_INFORMATION,
};

#endif  // COMMAND_CLASS_WAKE_UP_EVENTS_H
