
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

#ifndef GRANTED_KEYS_RESOLVER_TYPES_H
#define GRANTED_KEYS_RESOLVER_TYPES_H

#include "attribute_store.h"
#include "zwave_keyset_definitions.h"
#include "zwave_generic_types.h"
#include "zwave_controller_connection_info.h"

namespace zwave_command_class
{
    class granted_keys_resolver_types
    {
        public:
            struct granted_keys_resolver_payload_t {
                    attribute_store_node_t endpoint_node;
                    zwave_controller_encapsulation_scheme_t encapsulation;
            };
    };
}  // namespace zwave_command_class
#endif  // GRANTED_KEYS_RESOLVER_TYPES_H
