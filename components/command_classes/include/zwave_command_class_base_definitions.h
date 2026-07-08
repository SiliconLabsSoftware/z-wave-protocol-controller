/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef ZWAVE_COMMAND_CLASS_BASE_DEFINITIONS_H
#define ZWAVE_COMMAND_CLASS_BASE_DEFINITIONS_H

#include <string>
#include "attribute_store.h"                    // attribute_store_type_t
#include "attribute_store_type_registration.h"  // attribute_store_register_type

#include "zwave_controller_connection_info.h"  // zwave_controller_encapsulation_scheme_t

namespace zwave_command_class
{

    struct command_class_properties {
            const zwave_command_class_t command_class_id = 0x0000;
            /// The minimal security level which this command is supported on.
            /// This is ignored for the control_handler.
            const zwave_controller_encapsulation_scheme_t supported_handler_minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME;
            /// Name of the Command Class (not including Command Class)
            const std::string command_class_name;
            /// Comments for the Command Class implementation, that is printed to the log
            const std::string comments;
            /// Version of the implemented command class
            const uint8_t supported_version = 0;
            /// Use manual-security filtering for incoming frames
            /// If set to true, the command class dispatch handler will send frames to the
            /// handler without validating their security level.
            /// If set to false, the command class handler can assume that the frame has
            /// been received at an approved security level.
            const bool manual_security_validation = false;
    };

}  // namespace zwave_command_class
#endif  // ZWAVE_COMMAND_CLASS_BASE_DEFINITIONS_H