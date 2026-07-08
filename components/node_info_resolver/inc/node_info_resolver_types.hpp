
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

#ifndef NODE_INFO_RESOLVER_TYPES_H
#define NODE_INFO_RESOLVER_TYPES_H

#include "attribute_store.h"
#include "zwave_keyset_definitions.h"
#include "zwave_generic_types.h"
#include "zwave_controller_connection_info.h"

namespace zwave_command_class
{
    class node_info_resolver_types
    {
        public:
            // Node information frame structure. It is not provided by ZW_classcmd.h
            typedef struct _request_node_info_frame_t_ {
                    uint8_t command_class; /* The command class */
                    uint8_t command;       /* The command */
            } request_node_info_frame_t;
    };
}  // namespace zwave_command_class
#endif  // NODE_INFO_RESOLVER_TYPES_H
