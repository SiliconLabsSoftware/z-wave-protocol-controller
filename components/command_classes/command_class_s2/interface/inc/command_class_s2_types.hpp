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

#ifndef COMMAND_CLASS_S2_TYPES_H
#define COMMAND_CLASS_S2_TYPES_H

#include <stdint.h>

#include <variant>
#include <vector>
#include <string>
#include <map>

#include "attribute_store.h"
#include "attribute_store_defined_attribute_types.h"
#include "zwave_utils.h"

namespace zwave_command_class
{
    namespace command_class_s2_types
    {

        // Attributes for the command class
        enum class command_class_s2_attributes_t : attribute_store_type_t {
            supported_version = ZWAVE_CC_VERSION_ATTRIBUTE(159),
        };

        enum class s2_commands_supported_report_group_attributes_t : attribute_store_type_t {
            S2_COMMANDS_SUPPORTED_REPORT_GROUP = (159 << 8) | 2,
            command_class                      = (159 << 8) | 3,
        };

        // Commands for the command class
        enum class command_class_s2_commands_t : uint8_t {
            S2_COMMANDS_SUPPORTED_REPORT    = 14,
            S2_COMMANDS_CAPABILITIES_REPORT = 15,
        };

        using s2_commands_supported_report_cc_list_t = std::vector<uint8_t>;

        // Flexible value type that can hold int, vector, or string
        using command_class_s2_flexible_map_value_t = std::variant<int, uint8_t, uint16_t, uint32_t, s2_commands_supported_report_cc_list_t, std::string>;

        // Map type that can hold different value types
        using command_class_s2_attribute_map_t = std::map<std::string, command_class_s2_flexible_map_value_t>;

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

    }  // namespace command_class_s2_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_S2_TYPES_H
