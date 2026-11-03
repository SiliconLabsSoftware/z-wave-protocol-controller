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

#ifndef COMMAND_CLASS_USER_CODE_RULES_H
#define COMMAND_CLASS_USER_CODE_RULES_H

#include "attribute.hpp"
#include "attribute_store.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace zwave_command_class
{

    class command_class_user_code_rules
    {
        public:
            command_class_user_code_rules() = delete;

            static uint8_t peer_version(attribute_store::attribute endpoint);
            static bool capability_flag_is_set(attribute_store::attribute endpoint, attribute_store_type_t flag);
            static uint16_t supported_user_total(attribute_store::attribute endpoint);
            static bool multiple_keypad_modes_are_supported(attribute_store::attribute endpoint);
            // Identifier 0 is the version 1 erase-all target. An unknown user total does not refuse an identifier.
            static bool user_identifier_is_supported(attribute_store::attribute endpoint, uint16_t identifier);
            static bool keypad_mode_set_is_supported(attribute_store::attribute endpoint, uint8_t mode);
            static bool admin_code_is_valid(attribute_store::attribute endpoint, uint8_t length, const std::vector<uint8_t> &code);
            static bool user_code_v1_set_is_valid(uint8_t status, const std::vector<uint8_t> &code);
            static bool extended_user_code_blocks_are_valid(attribute_store::attribute endpoint, uint8_t count, const std::vector<uint8_t> &packed);
            static bool user_code_checksum_get_is_supported(attribute_store::attribute endpoint);
            // Length is the User Code Length nibble. Only that many code bytes are stored after the 4-byte header.
            static std::vector<uint8_t> pack_user_code_block(uint16_t identifier, uint8_t status, uint8_t length, const std::vector<uint8_t> &code);
            // Available omits the code. Version 2 accepts an empty code or the version 1 four 0x00 bytes as that omission.
            static std::vector<uint8_t> pack_set_block(uint16_t identifier, uint8_t status, const std::vector<uint8_t> &code);

        private:
            static attribute_store::attribute capabilities_group(attribute_store::attribute endpoint);
            static std::optional<uint8_t> reported_flag(attribute_store::attribute endpoint, attribute_store_type_t flag);
            static bool capability_flag_allows(attribute_store::attribute endpoint, attribute_store_type_t flag);
            static std::vector<uint8_t> reported_mask(attribute_store::attribute endpoint, attribute_store_type_t type);
            static bool mask_bit_is_set(const std::vector<uint8_t> &mask, uint8_t index);
            static uint8_t mask_popcount(const std::vector<uint8_t> &mask);
            static bool users_number_is_known(attribute_store::attribute endpoint);
            static bool digits_only(const std::vector<uint8_t> &code);
            static bool printable_ascii(const std::vector<uint8_t> &code);
            static bool supported_key_bytes(attribute_store::attribute endpoint, const std::vector<uint8_t> &code);
            static bool status_is_supported(attribute_store::attribute endpoint, uint8_t status);
            static bool is_erased_user_code(const std::vector<uint8_t> &code);
            static bool multiple_user_code_set_is_supported(attribute_store::attribute endpoint, uint8_t count);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CODE_RULES_H
