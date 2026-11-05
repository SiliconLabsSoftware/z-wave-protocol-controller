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

#ifndef COMMAND_CLASS_USER_CREDENTIAL_RULES_H
#define COMMAND_CLASS_USER_CREDENTIAL_RULES_H

#include "attribute.hpp"
#include "attribute_store.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace zwave_command_class
{

    class command_class_user_credential_rules
    {
        public:
            command_class_user_credential_rules() = delete;

            static std::optional<uint16_t> supported_user_count(attribute_store::attribute endpoint);
            static bool endpoint_advertises_command_class(attribute_store::attribute endpoint, uint8_t command_class);
            static bool user_identifier_is_supported(attribute_store::attribute endpoint, uint16_t identifier, bool zero_is_supported);
            static bool user_type_is_supported(attribute_store::attribute endpoint, uint8_t user_type);
            static bool credential_rule_is_supported(attribute_store::attribute endpoint, uint8_t credential_rule);
            static bool user_name_encoding_is_supported(attribute_store::attribute endpoint, uint8_t encoding);
            static bool user_set_is_valid(attribute_store::attribute endpoint,
                                          uint8_t operation_type,
                                          uint16_t user_unique_identifier,
                                          uint8_t user_type,
                                          uint8_t user_active_state,
                                          uint8_t credential_rule,
                                          uint16_t expiring_timeout_minutes,
                                          uint8_t user_name_encoding,
                                          uint8_t user_name_length,
                                          const std::vector<uint8_t> &user_name);
            static bool credential_type_is_supported(attribute_store::attribute endpoint, uint8_t credential_type);
            static bool credential_slot_is_supported(attribute_store::attribute endpoint, uint8_t credential_type, uint16_t credential_slot, bool zero_slot_is_supported);
            static bool credential_set_is_valid(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot, uint8_t operation_type, uint8_t credential_length, const std::vector<uint8_t> &credential_data);
            static bool credential_get_is_valid(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot);
            static bool credential_learn_start_is_valid(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot, uint8_t operation_type);
            static bool user_credential_association_set_is_valid(attribute_store::attribute endpoint, uint8_t credential_type, uint16_t credential_slot, uint16_t destination_user_unique_identifier);
            static bool admin_pin_code_set_is_valid(attribute_store::attribute endpoint, uint8_t length, const std::vector<uint8_t> &admin_code);
            static bool admin_pin_code_get_is_supported(attribute_store::attribute endpoint);
            static bool all_users_checksum_get_is_supported(attribute_store::attribute endpoint);
            static bool user_checksum_get_is_supported(attribute_store::attribute endpoint, uint16_t user_unique_identifier);
            static bool credential_checksum_get_is_supported(attribute_store::attribute endpoint, uint8_t credential_type);
            static bool user_schedule_is_supported(attribute_store::attribute endpoint);
            static bool all_users_checksum_is_supported(attribute_store::attribute endpoint);

        private:
            static attribute_store::attribute user_capabilities(attribute_store::attribute endpoint);
            static attribute_store::attribute credential_capabilities(attribute_store::attribute endpoint);
            static std::optional<uint8_t> reported_u8(attribute_store::attribute group, attribute_store_type_t type);
            static std::vector<uint8_t> reported_bytes(attribute_store::attribute group, attribute_store_type_t type);
            static bool capability_flag_is_set(attribute_store::attribute group, attribute_store_type_t flag);
            static bool capability_flag_allows(attribute_store::attribute group, attribute_store_type_t flag);
            static bool mask_bit_is_set(const std::vector<uint8_t> &mask, uint8_t index);
            static bool digits_only(const std::vector<uint8_t> &data);
            static std::optional<size_t> credential_type_index(attribute_store::attribute endpoint, uint8_t credential_type, bool &list_is_known);
            static std::optional<uint16_t> u16_at(const std::vector<uint8_t> &bytes, size_t index);
            static bool credential_length_is_supported(attribute_store::attribute endpoint, uint8_t credential_type, uint8_t length);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CREDENTIAL_RULES_H
