
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

#ifndef COMMAND_CLASS_USER_CODE_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_USER_CODE_ATTRIBUTE_STORE_H

#include "command_class_user_code_core.hpp"
#include "command_class_user_code_types.hpp"  // command_class_user_code_types

namespace zwave_command_class
{

    class command_class_user_code_attribute_store : public virtual command_class_user_code_core
    {

        public:
            command_class_user_code_attribute_store()  = default;
            ~command_class_user_code_attribute_store() = default;

            sl_status_t on_user_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map) override;
            sl_status_t on_users_number_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map) override;
            sl_status_t on_extended_user_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map) override;
            sl_status_t on_user_code_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map) override;
            sl_status_t on_user_code_keypad_mode_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map) override;
            sl_status_t on_admin_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map) override;
            sl_status_t on_user_code_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map) override;

        private:
            template<typename T> static void store_value(attribute_store::attribute parent, attribute_store_type_t type, T value);
            template<typename T> static void store_if_present(attribute_store::attribute parent, attribute_store_type_t type, const command_class_user_code_attribute_map_t &map, const char *key);
            static attribute_store::attribute user_code_group(attribute_store::attribute endpoint, uint8_t identifier);
            static void upsert_extended_block(attribute_store::attribute group, const extended_user_code_report_vg1_t_item_t &item);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CODE_ATTRIBUTE_STORE_H
