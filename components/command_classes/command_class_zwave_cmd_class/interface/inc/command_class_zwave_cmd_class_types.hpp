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

#ifndef ZWAVE_CMD_CLASS_TYPES_H
#define ZWAVE_CMD_CLASS_TYPES_H

#include "command_class_zwave_cmd_class_generated_types.hpp"
#include "zwave_utils.h"

namespace zwave_command_class
{
    namespace command_class_zwave_cmd_class_types
    {
        struct command_class_protocol_commands_request_node_info_payload_t {
                zwave_node_id_t node_id;
        };

    }  // namespace command_class_zwave_cmd_class_types
}  // namespace zwave_command_class

#endif  // ZWAVE_CMD_CLASS_TYPES_H
