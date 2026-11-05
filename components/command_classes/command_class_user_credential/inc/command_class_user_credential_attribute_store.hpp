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

#ifndef COMMAND_CLASS_USER_CREDENTIAL_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_USER_CREDENTIAL_ATTRIBUTE_STORE_H

#include "command_class_user_credential_core.hpp"
#include "command_class_user_credential_types.hpp"

namespace zwave_command_class
{

    class command_class_user_credential_attribute_store : public virtual command_class_user_credential_core
    {

        public:
            command_class_user_credential_attribute_store()  = default;
            ~command_class_user_credential_attribute_store() = default;

            sl_status_t on_user_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_credential_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_user_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_credential_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_credential_learn_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_user_credential_association_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_all_users_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_user_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_credential_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;
            sl_status_t on_admin_pin_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map) override;

        private:
            template<typename T> static void store_value(attribute_store::attribute parent, attribute_store_type_t type, T value);
            template<typename T> void store_if_present(attribute_store::attribute parent, attribute_store_type_t type, const command_class_user_credential_attribute_map_t &map, const char *key, T fallback);
            template<typename Item, typename Extract> static std::vector<uint8_t> pack_bytes(const std::vector<Item> &items, Extract extract);
            template<typename Item, typename Extract> static std::vector<uint8_t> pack_u16(const std::vector<Item> &items, Extract extract);
            template<typename T> static bool reported_equals(attribute_store::attribute group, attribute_store_type_t attribute_type, T value);
            // The group already holding this report's identity, else a group without a reported identity, else a new group.
            template<typename Matches> static attribute_store::attribute group_for_report(attribute_store::attribute endpoint, attribute_store_type_t group_type, attribute_store_type_t identity_type, Matches matches);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CREDENTIAL_ATTRIBUTE_STORE_H
