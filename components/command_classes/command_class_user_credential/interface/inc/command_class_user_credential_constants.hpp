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

#ifndef COMMAND_CLASS_USER_CREDENTIAL_CONSTANTS_H
#define COMMAND_CLASS_USER_CREDENTIAL_CONSTANTS_H

#include <cstdint>

#include "ZW_classcmd.h"

namespace zwave_command_class
{
    namespace command_class_user_credential_constants
    {
        constexpr uint8_t OPERATION_ADD    = 0;
        constexpr uint8_t OPERATION_MODIFY = 1;
        constexpr uint8_t OPERATION_DELETE = 2;

        constexpr uint8_t USER_TYPE_GENERAL     = 0x00;
        constexpr uint8_t USER_TYPE_PROGRAMMING = 0x03;
        constexpr uint8_t USER_TYPE_NON_ACCESS  = 0x04;
        constexpr uint8_t USER_TYPE_DURESS      = 0x05;
        constexpr uint8_t USER_TYPE_DISPOSABLE  = 0x06;
        constexpr uint8_t USER_TYPE_EXPIRING    = 0x07;
        constexpr uint8_t USER_TYPE_REMOTE_ONLY = 0x09;

        constexpr uint8_t USER_NAME_ENCODING_ASCII          = 0;
        constexpr uint8_t USER_NAME_ENCODING_EXTENDED_ASCII = 1;
        constexpr uint8_t USER_NAME_ENCODING_UTF_16         = 2;

        constexpr uint8_t CREDENTIAL_RULE_SINGLE = 0x01;
        constexpr uint8_t CREDENTIAL_RULE_DUAL   = 0x02;
        constexpr uint8_t CREDENTIAL_RULE_TRIPLE = 0x03;

        constexpr uint8_t CREDENTIAL_TYPE_NONE = 0x00;
        constexpr uint8_t CREDENTIAL_TYPE_PIN  = 0x01;

        constexpr uint8_t PIN_LENGTH_MIN  = 4;
        constexpr uint8_t PIN_LENGTH_MAX  = 10;
        constexpr uint8_t ASCII_DIGIT_MIN = 0x30;
        constexpr uint8_t ASCII_DIGIT_MAX = 0x39;

        constexpr uint8_t USER_ACTIVE   = 1;
        constexpr uint8_t USER_INACTIVE = 0;

        constexpr uint8_t TARGET_COMMAND_CLASS   = COMMAND_CLASS_USER_CREDENTIAL;
        constexpr uint16_t DELETE_ALL_IDENTIFIER = 0;
        constexpr uint16_t FIRST_SLOT            = 0;
    }  // namespace command_class_user_credential_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CREDENTIAL_CONSTANTS_H
