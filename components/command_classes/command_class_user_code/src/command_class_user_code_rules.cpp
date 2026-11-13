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

#include <algorithm>
#include <bit>
#include <cstdint>
#include <optional>
#include <vector>

#include "attribute_store_defined_attribute_types.h"
#include "command_class_user_code_constants.hpp"
#include "command_class_user_code_rules.hpp"
#include "zwave_command_class_utils.hpp"
#include "ZW_classcmd.h"

namespace zwave_command_class
{
    using namespace command_class_user_code_types;
    using namespace command_class_user_code_constants;

    attribute_store::attribute command_class_user_code_rules::capabilities_group(attribute_store::attribute endpoint)
    {
        return endpoint.child_by_type(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP));
    }

    std::optional<uint8_t> command_class_user_code_rules::reported_flag(attribute_store::attribute endpoint, attribute_store_type_t flag)
    {
        auto group = capabilities_group(endpoint);
        if (!group.is_valid()) {
            return std::nullopt;
        }
        auto node = group.child_by_type(flag);
        if (!node.is_valid() || !node.reported_exists()) {
            return std::nullopt;
        }
        return node.reported<uint8_t>();
    }

    bool command_class_user_code_rules::capability_flag_allows(attribute_store::attribute endpoint, attribute_store_type_t flag)
    {
        // A missing capabilities report does not refuse the command. A stored 0 does.
        const auto value = reported_flag(endpoint, flag);
        return !value.has_value() || *value != 0;
    }

    std::vector<uint8_t> command_class_user_code_rules::reported_mask(attribute_store::attribute endpoint, attribute_store_type_t type)
    {
        auto group = capabilities_group(endpoint);
        if (!group.is_valid()) {
            return {};
        }
        auto node = group.child_by_type(type);
        if (!node.is_valid() || !node.reported_exists()) {
            return {};
        }
        return node.reported<std::vector<uint8_t>>();
    }

    bool command_class_user_code_rules::mask_bit_is_set(const std::vector<uint8_t> &mask, uint8_t index)
    {
        const size_t byte_index = static_cast<size_t>(index / 8);
        if (byte_index >= mask.size()) {
            return false;
        }
        return (mask[byte_index] & static_cast<uint8_t>(1U << (index % 8))) != 0;
    }

    uint8_t command_class_user_code_rules::mask_popcount(const std::vector<uint8_t> &mask)
    {
        uint8_t count = 0;
        for (uint8_t byte: mask) {
            count = static_cast<uint8_t>(count + std::popcount(byte));
        }
        return count;
    }

    bool command_class_user_code_rules::users_number_is_known(attribute_store::attribute endpoint)
    {
        auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP));
        if (!group.is_valid()) {
            return false;
        }
        auto node = group.child_by_type(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::supported_users));
        return node.is_valid() && node.reported_exists();
    }

    bool command_class_user_code_rules::digits_only(const std::vector<uint8_t> &code)
    {
        return std::all_of(code.begin(), code.end(), [](uint8_t byte) { return byte >= ASCII_DIGIT_MIN && byte <= ASCII_DIGIT_MAX; });
    }

    bool command_class_user_code_rules::printable_ascii(const std::vector<uint8_t> &code)
    {
        return std::all_of(code.begin(), code.end(), [](uint8_t byte) { return byte >= ASCII_PRINTABLE_MIN && byte <= ASCII_PRINTABLE_MAX; });
    }

    bool command_class_user_code_rules::supported_key_bytes(attribute_store::attribute endpoint, const std::vector<uint8_t> &code)
    {
        const auto mask = reported_mask(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keys_bit_mask));
        if (mask.empty()) {
            return printable_ascii(code);
        }
        return std::all_of(code.begin(), code.end(), [&mask](uint8_t byte) { return mask_bit_is_set(mask, byte); });
    }

    bool command_class_user_code_rules::status_is_supported(attribute_store::attribute endpoint, uint8_t status)
    {
        if (status == STATUS_AVAILABLE) {
            return true;
        }
        if (status < STATUS_OCCUPIED || status > STATUS_PASSAGE) {
            return false;
        }
        const auto mask = reported_mask(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_user_id_status_bit_mask));
        if (mask.empty()) {
            return true;
        }
        return mask_bit_is_set(mask, status);
    }

    bool command_class_user_code_rules::is_erased_user_code(const std::vector<uint8_t> &code)
    {
        return code.size() == ERASED_CODE_LENGTH && std::all_of(code.begin(), code.end(), [](uint8_t byte) { return byte == 0; });
    }

    bool command_class_user_code_rules::multiple_user_code_set_is_supported(attribute_store::attribute endpoint, uint8_t count)
    {
        if (count <= 1) {
            return true;
        }
        return capability_flag_allows(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::mucs_support));
    }

    uint8_t command_class_user_code_rules::peer_version(attribute_store::attribute endpoint)
    {
        auto node = endpoint.child_by_type(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_USER_CODE));
        if (!node.is_valid() || !node.reported_exists()) {
            return 0;
        }
        return node.reported<uint8_t>();
    }

    bool command_class_user_code_rules::capability_flag_is_set(attribute_store::attribute endpoint, attribute_store_type_t flag)
    {
        const auto value = reported_flag(endpoint, flag);
        return value.has_value() && *value != 0;
    }

    uint16_t command_class_user_code_rules::supported_user_total(attribute_store::attribute endpoint)
    {
        auto group         = endpoint.child_by_type(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP));
        uint16_t supported = 0;
        uint16_t extended  = 0;
        if (group.is_valid()) {
            auto supported_node = group.child_by_type(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::supported_users));
            if (supported_node.reported_exists()) {
                supported = supported_node.reported<uint8_t>();
            }
            auto extended_node = group.child_by_type(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::extended_supported_users));
            if (extended_node.reported_exists()) {
                extended = extended_node.reported<uint16_t>();
            }
        }
        // CC:0063.02.05.11.002 / .003: version 2 uses the extended total when the peer sent one.
        if (peer_version(endpoint) >= 2 && extended != 0) {
            return extended;
        }
        return supported;
    }

    bool command_class_user_code_rules::multiple_keypad_modes_are_supported(attribute_store::attribute endpoint)
    {
        const auto mask = reported_mask(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keypad_modes_bit_mask));
        return mask_popcount(mask) > 1;
    }

    std::vector<uint8_t> command_class_user_code_rules::pack_user_code_block(uint16_t identifier, uint8_t status, uint8_t length, const std::vector<uint8_t> &code)
    {
        const uint8_t stored_length = static_cast<uint8_t>(length & LENGTH_NIBBLE_MASK);
        std::vector<uint8_t> packed {static_cast<uint8_t>(identifier >> 8), static_cast<uint8_t>(identifier & 0xFF), status, stored_length};
        const auto stored = std::min(code.size(), static_cast<size_t>(stored_length));
        packed.insert(packed.end(), code.begin(), code.begin() + static_cast<std::ptrdiff_t>(stored));
        return packed;
    }

    bool command_class_user_code_rules::user_identifier_is_supported(attribute_store::attribute endpoint, uint16_t identifier)
    {
        if (identifier == 0 || !users_number_is_known(endpoint)) {
            return true;
        }
        return identifier <= supported_user_total(endpoint);
    }

    bool command_class_user_code_rules::keypad_mode_set_is_supported(attribute_store::attribute endpoint, uint8_t mode)
    {
        const auto mask = reported_mask(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keypad_modes_bit_mask));
        if (mask.empty()) {
            return true;
        }
        // CL:0063.01.31.05.1: the action exists only when more than one keypad mode is supported.
        if (mask_popcount(mask) <= 1) {
            return false;
        }
        return mask_bit_is_set(mask, mode);
    }

    bool command_class_user_code_rules::admin_code_is_valid(attribute_store::attribute endpoint, uint8_t length, const std::vector<uint8_t> &code)
    {
        if (!capability_flag_allows(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ac_support))) {
            return false;
        }
        if (length == 0) {
            // CC:0063.02.0E.11.005: length 0 deactivates the Admin Code and is ignored without ACD.
            return code.empty() && capability_flag_allows(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::acd_support));
        }
        if (length < USER_CODE_LENGTH_MIN || length > USER_CODE_LENGTH_MAX || code.size() != length) {
            return false;
        }
        return supported_key_bytes(endpoint, code);
    }

    bool command_class_user_code_rules::user_code_v1_set_is_valid(uint8_t status, const std::vector<uint8_t> &code)
    {
        if (status == STATUS_AVAILABLE) {
            return is_erased_user_code(code);
        }
        if (status != STATUS_OCCUPIED && status != STATUS_RESERVED) {
            return false;
        }
        return code.size() >= USER_CODE_LENGTH_MIN && code.size() <= USER_CODE_LENGTH_MAX && digits_only(code);
    }

    bool command_class_user_code_rules::extended_user_code_blocks_are_valid(attribute_store::attribute endpoint, uint8_t count, const std::vector<uint8_t> &packed)
    {
        if (count == 0 || !multiple_user_code_set_is_supported(endpoint, count)) {
            return false;
        }
        size_t offset       = 0;
        uint8_t seen_blocks = 0;
        while (offset < packed.size()) {
            if (packed.size() - offset < EXTENDED_BLOCK_HEADER) {
                return false;
            }
            const uint16_t identifier = static_cast<uint16_t>((static_cast<uint16_t>(packed[offset]) << 8) | packed[offset + 1]);
            const uint8_t status      = packed[offset + 2];
            const uint8_t properties  = packed[offset + 3];
            const uint8_t length      = static_cast<uint8_t>(properties & LENGTH_NIBBLE_MASK);
            if ((properties & static_cast<uint8_t>(~LENGTH_NIBBLE_MASK)) != 0) {
                return false;
            }
            offset += EXTENDED_BLOCK_HEADER;
            if (packed.size() - offset < length) {
                return false;
            }
            const std::vector<uint8_t> code(packed.begin() + static_cast<std::ptrdiff_t>(offset), packed.begin() + static_cast<std::ptrdiff_t>(offset + length));
            offset += length;
            if (!user_identifier_is_supported(endpoint, identifier) || !status_is_supported(endpoint, status)) {
                return false;
            }
            if (status == STATUS_AVAILABLE) {
                if (length != 0 || !code.empty()) {
                    return false;
                }
            } else if (length < USER_CODE_LENGTH_MIN || length > USER_CODE_LENGTH_MAX || !supported_key_bytes(endpoint, code)) {
                return false;
            }
            ++seen_blocks;
        }
        return seen_blocks == count;
    }

    bool command_class_user_code_rules::user_code_checksum_get_is_supported(attribute_store::attribute endpoint)
    {
        return capability_flag_allows(endpoint, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ucc_support));
    }

    std::vector<uint8_t> command_class_user_code_rules::pack_set_block(uint16_t identifier, uint8_t status, const std::vector<uint8_t> &code)
    {
        if (status == STATUS_AVAILABLE) {
            return pack_user_code_block(identifier, status, 0, {});
        }
        return pack_user_code_block(identifier, status, static_cast<uint8_t>(code.size()), code);
    }

}  // namespace zwave_command_class
