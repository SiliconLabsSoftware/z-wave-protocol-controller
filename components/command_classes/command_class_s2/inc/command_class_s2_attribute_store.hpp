
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

#ifndef COMMAND_CLASS_S2_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_S2_ATTRIBUTE_STORE_H

#include "command_class_s2_core.hpp"
#include "command_class_s2_types.hpp"  // command_class_s2_types

namespace zwave_command_class
{

    class command_class_s2_attribute_store : public virtual command_class_s2_core
    {
            const attribute_list_registration_t attributes
              = {{static_cast<attribute_store_type_t>(command_class_s2_attributes_t::supported_version), "supported_version", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
                 {static_cast<attribute_store_type_t>(s2_commands_supported_report_group_attributes_t::S2_COMMANDS_SUPPORTED_REPORT_GROUP), "S2_COMMANDS_SUPPORTED_REPORT_GROUP", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
                 {static_cast<attribute_store_type_t>(s2_commands_supported_report_group_attributes_t::command_class), "command_class", static_cast<attribute_store_type_t>(s2_commands_supported_report_group_attributes_t::S2_COMMANDS_SUPPORTED_REPORT_GROUP), BYTE_ARRAY_STORAGE_TYPE}};

        public:
            command_class_s2_attribute_store();
            ~command_class_s2_attribute_store() = default;

            sl_status_t on_s2_commands_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_s2_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_S2_ATTRIBUTE_STORE_H
