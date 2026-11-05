/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMMAND_CLASS_USER_CREDENTIAL_EVENTS_H
#define COMMAND_CLASS_USER_CREDENTIAL_EVENTS_H

#include <stdint.h>

#include "attribute.hpp"

enum class command_class_user_credential_events_t : uint32_t {
    COMMAND_CLASS_USER_CREDENTIAL_BASE_EVENT = (131 << 8),
    COMMAND_CLASS_USER_CREDENTIAL_SUPPORTED_USER_COUNT,
    COMMAND_CLASS_USER_CREDENTIAL_INTERVIEW_USER_SCHEDULE,
    COMMAND_CLASS_USER_CREDENTIAL_PRESENCE_QUERY,
    COMMAND_CLASS_USER_CREDENTIAL_PRESENCE_RESPONSE,
};

struct user_credential_presence_query_payload_t {
        attribute_store::attribute endpoint;
};

struct user_credential_presence_response_payload_t {
        attribute_store::attribute endpoint;
        bool present;
};

struct user_credential_supported_user_count_payload_t {
        attribute_store::attribute endpoint;
        uint16_t supported_user_count;
};

struct user_credential_interview_user_schedule_payload_t {
        attribute_store::attribute endpoint;
        uint8_t target_command_class;
        uint16_t user_unique_identifier;
};

#endif  // COMMAND_CLASS_USER_CREDENTIAL_EVENTS_H
