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

#ifndef COMMAND_CLASS_S0_TYPES_H
#define COMMAND_CLASS_S0_TYPES_H

#include <stdint.h>

#include <variant>
#include <vector>
#include <string>
#include <map>

#include "attribute_store.h"
#include "zwave_utils.h"

namespace zwave_command_class
{
    namespace command_class_s0_types
    {
        // Number of Reports to Follow
        using reports_to_follow_t = uint8_t;

        // Attributes for the command class
        enum class command_class_s0_attributes_t : attribute_store_type_t {
            supported_version = ZWAVE_CC_VERSION_ATTRIBUTE(152),
        };

        enum class s0_commands_supported_report_group_attributes_t : attribute_store_type_t {
            S0_COMMANDS_SUPPORTED_REPORT_GROUP = (152 << 8) | 2,
            command_class                      = (152 << 8) | 3,
        };

        enum class s0_reports_to_follow_attributes_t : attribute_store_type_t {
            S0_REPORTS_TO_FOLLOW_ATTRIBUTE = (152 << 8) | 4,
        };

        // Commands for the command class
        enum class command_class_s0_commands_t : uint8_t {
            S0_COMMANDS_SUPPORTED_REPORT     = 3,
            S0_GET_SUPPORTED_COMMAND_CLASSES = 4,
        };

        struct command_class_s0_payload_t {
                uint8_t *frame;
                uint16_t *frame_len;
        };

        struct s0_supported_get_payload_t {
                uint8_t endpoint_id;
                attribute_store_node_t device_node;
                attribute_store_node_t endpoint_node;
                zwave_node_id_t zwave_node_id;
                zwave_keyset_t granted_keys;
        };

        using s0_commands_supported_report_cc_list_t = std::vector<uint8_t>;

        struct s0_supported_report_payload_t {
                zwave_controller_connection_info_t connection_info;
                s0_commands_supported_report_cc_list_t supported_cc_list;
        };

        struct s0_get_supported_command_class_list_payload_t {
                attribute_store_node_t endpoint_node;
        };

        // Flexible value type that can hold int, vector, or string
        using command_class_s0_flexible_map_value_t = std::variant<int, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string>;

        // Map type that can hold different value types
        using command_class_s0_attribute_map_t = std::map<std::string, command_class_s0_flexible_map_value_t>;

    }  // namespace command_class_s0_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_S0_TYPES_H
