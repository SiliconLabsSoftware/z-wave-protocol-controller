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

#ifndef COMMAND_CLASS_SECURITY_2_TYPES_H
#define COMMAND_CLASS_SECURITY_2_TYPES_H

#include "command_class_security_2_generated_types.hpp"
#include "attribute_store.h"
#include "zwave_controller.h"
#include "zwave_utils.h"

namespace zwave_command_class
{
    namespace command_class_security_2_types
    {
        using s2_commands_supported_report_cc_list_t = std::vector<uint8_t>;

        struct s2_supported_get_payload_t {
                uint8_t endpoint_id;
                attribute_store_node_t device_node;
                attribute_store_node_t endpoint_node;
                zwave_node_id_t zwave_node_id;
                zwave_keyset_t granted_keys;
        };

        struct s2_supported_get_tx_failed_payload_t {
                zwave_node_id_t zwave_node_id;
                uint8_t endpoint_id;
                uint8_t status;
        };

        struct s2_supported_report_payload_t {
                zwave_controller_connection_info_t connection_info;
                s2_commands_supported_report_cc_list_t supported_cc_list;
        };

        struct s2_get_supported_command_class_list_payload_t {
                attribute_store_node_t endpoint_node;
        };

    }  // namespace command_class_security_2_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SECURITY_2_TYPES_H
