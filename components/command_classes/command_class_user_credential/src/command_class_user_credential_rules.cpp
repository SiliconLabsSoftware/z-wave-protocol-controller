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

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include "attribute_store_defined_attribute_types.h"
#include "command_class_user_credential_constants.hpp"
#include "command_class_user_credential_rules.hpp"
#include "command_class_user_credential_types.hpp"
#include "zwave_command_class_utils.hpp"
#include "ZW_classcmd.h"

namespace zwave_command_class
{
    using namespace command_class_user_credential_types;
    using namespace command_class_user_credential_constants;

    attribute_store::attribute command_class_user_credential_rules::user_capabilities(attribute_store::attribute endpoint)
    {
        return endpoint.child_by_type(static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::USER_CAPABILITIES_REPORT_GROUP));
    }

    attribute_store::attribute command_class_user_credential_rules::credential_capabilities(attribute_store::attribute endpoint)
    {
        return endpoint.child_by_type(static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::CREDENTIAL_CAPABILITIES_REPORT_GROUP));
    }

    std::optional<uint8_t> command_class_user_credential_rules::reported_u8(attribute_store::attribute group, attribute_store_type_t type)
    {
        if (!group.is_valid()) {
            return std::nullopt;
        }
        auto node = group.child_by_type(type);
        if (!node.is_valid() || !node.reported_exists()) {
            return std::nullopt;
        }
        return node.reported<uint8_t>();
    }

    std::vector<uint8_t> command_class_user_credential_rules::reported_bytes(attribute_store::attribute group, attribute_store_type_t type)
    {
        if (!group.is_valid()) {
            return {};
        }
        auto node = group.child_by_type(type);
        if (!node.is_valid() || !node.reported_exists()) {
            return {};
        }
        return node.reported<std::vector<uint8_t>>();
    }

    bool command_class_user_credential_rules::capability_flag_is_set(attribute_store::attribute group, attribute_store_type_t flag)
    {
        const auto value = reported_u8(group, flag);
        return value.has_value() && *value != 0;
    }

    bool command_class_user_credential_rules::capability_flag_allows(attribute_store::attribute group, attribute_store_type_t flag)
    {
        const auto value = reported_u8(group, flag);
        return !value.has_value() || *value != 0;
    }

    bool command_class_user_credential_rules::mask_bit_is_set(const std::vector<uint8_t> &mask, uint8_t index)
    {
        const size_t byte_index = static_cast<size_t>(index / 8);
        if (byte_index >= mask.size()) {
            return false;
        }
        return (mask[byte_index] & static_cast<uint8_t>(1U << (index % 8))) != 0;
    }

    bool command_class_user_credential_rules::digits_only(const std::vector<uint8_t> &data)
    {
        return std::all_of(data.begin(), data.end(), [](uint8_t byte) { return byte >= ASCII_DIGIT_MIN && byte <= ASCII_DIGIT_MAX; });
    }

    std::optional<uint16_t> command_class_user_credential_rules::u16_at(const std::vector<uint8_t> &bytes, size_t index)
    {
        const size_t offset = index * 2;
        if (bytes.size() < offset + 2) {
            return std::nullopt;
        }
        return static_cast<uint16_t>((static_cast<uint16_t>(bytes[offset]) << 8) | bytes[offset + 1]);
    }

    std::optional<uint16_t> command_class_user_credential_rules::supported_user_count(attribute_store::attribute endpoint)
    {
        auto group = user_capabilities(endpoint);
        auto node  = group.child_by_type(static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::number_of_supported_user_unique_identifiers));
        if (!node.is_valid() || !node.reported_exists()) {
            return std::nullopt;
        }
        return node.reported<uint16_t>();
    }

    bool command_class_user_credential_rules::endpoint_advertises_command_class(attribute_store::attribute endpoint, uint8_t command_class)
    {
        const auto listed = [command_class](attribute_store::attribute node) {
            if (!node.is_valid() || !node.reported_exists()) {
                return false;
            }
            try {
                const auto normal = command_class_utils::get_normal_command_classes(node.reported<std::vector<uint8_t>>());
                return std::find(normal.begin(), normal.end(), command_class) != normal.end();
            } catch (...) {
                return false;
            }
        };
        return listed(endpoint.child_by_type(ATTRIBUTE_ZWAVE_NIF)) || listed(endpoint.child_by_type(ATTRIBUTE_ZWAVE_SECURE_NIF));
    }

    bool command_class_user_credential_rules::user_identifier_is_supported(attribute_store::attribute endpoint, uint16_t identifier, bool zero_is_supported)
    {
        if (identifier == DELETE_ALL_IDENTIFIER) {
            return zero_is_supported;
        }
        const auto count = supported_user_count(endpoint);
        if (!count.has_value()) {
            return true;
        }
        return identifier >= 1 && identifier <= *count;
    }

    bool command_class_user_credential_rules::user_type_is_supported(attribute_store::attribute endpoint, uint8_t user_type)
    {
        const bool known = user_type == USER_TYPE_GENERAL || user_type == USER_TYPE_PROGRAMMING || user_type == USER_TYPE_NON_ACCESS || user_type == USER_TYPE_DURESS || user_type == USER_TYPE_DISPOSABLE || user_type == USER_TYPE_EXPIRING || user_type == USER_TYPE_REMOTE_ONLY;
        if (!known) {
            return false;
        }
        const auto mask = reported_bytes(user_capabilities(endpoint), static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::supported_user_types_bit_mask));
        if (mask.empty()) {
            return true;
        }
        return mask_bit_is_set(mask, user_type);
    }

    bool command_class_user_credential_rules::credential_rule_is_supported(attribute_store::attribute endpoint, uint8_t credential_rule)
    {
        if (credential_rule < CREDENTIAL_RULE_SINGLE || credential_rule > CREDENTIAL_RULE_TRIPLE) {
            return false;
        }
        const auto mask = reported_bytes(user_capabilities(endpoint), static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::supported_credential_rules_bit_mask));
        if (mask.empty()) {
            return true;
        }
        return mask_bit_is_set(mask, credential_rule);
    }

    bool command_class_user_credential_rules::user_name_encoding_is_supported(attribute_store::attribute endpoint, uint8_t encoding)
    {
        auto group = user_capabilities(endpoint);
        if (encoding == USER_NAME_ENCODING_ASCII) {
            return capability_flag_allows(group, static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::ascii_encoding_support));
        }
        if (encoding == USER_NAME_ENCODING_EXTENDED_ASCII) {
            return capability_flag_allows(group, static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::extended_ascii_encoding_support));
        }
        if (encoding == USER_NAME_ENCODING_UTF_16) {
            return capability_flag_allows(group, static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::utf_16_encoding_support));
        }
        return false;
    }

    bool command_class_user_credential_rules::user_set_is_valid(attribute_store::attribute endpoint,
                                                                uint8_t operation_type,
                                                                uint16_t user_unique_identifier,
                                                                uint8_t user_type,
                                                                uint8_t user_active_state,
                                                                uint8_t credential_rule,
                                                                uint16_t expiring_timeout_minutes,
                                                                uint8_t user_name_encoding,
                                                                uint8_t user_name_length,
                                                                const std::vector<uint8_t> &user_name)
    {
        if (operation_type > OPERATION_DELETE) {
            return false;
        }
        if (user_active_state > USER_ACTIVE) {
            return false;
        }
        const bool delete_all_is_allowed = operation_type == OPERATION_DELETE;
        if (!user_identifier_is_supported(endpoint, user_unique_identifier, delete_all_is_allowed)) {
            return false;
        }
        if (operation_type == OPERATION_DELETE && user_unique_identifier == DELETE_ALL_IDENTIFIER) {
            return true;
        }
        if (operation_type == OPERATION_DELETE) {
            return true;
        }
        if (!user_type_is_supported(endpoint, user_type) || !credential_rule_is_supported(endpoint, credential_rule) || !user_name_encoding_is_supported(endpoint, user_name_encoding)) {
            return false;
        }
        if (user_type == USER_TYPE_EXPIRING) {
            if (expiring_timeout_minutes == 0) {
                return false;
            }
        } else if (expiring_timeout_minutes != 0) {
            return false;
        }
        if (user_name_length != user_name.size()) {
            return false;
        }
        if (user_name_encoding == USER_NAME_ENCODING_UTF_16 && (user_name_length % 2) != 0) {
            return false;
        }
        const auto max_length = reported_u8(user_capabilities(endpoint), static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::max_length_of_user_name));
        return !max_length.has_value() || user_name_length <= *max_length;
    }

    std::optional<size_t> command_class_user_credential_rules::credential_type_index(attribute_store::attribute endpoint, uint8_t credential_type, bool &list_is_known)
    {
        const auto types = reported_bytes(credential_capabilities(endpoint), static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::vg1_credential_type));
        list_is_known    = !types.empty();
        if (!list_is_known) {
            return std::nullopt;
        }
        const auto found = std::find(types.begin(), types.end(), credential_type);
        if (found == types.end()) {
            return std::nullopt;
        }
        return static_cast<size_t>(found - types.begin());
    }

    bool command_class_user_credential_rules::credential_type_is_supported(attribute_store::attribute endpoint, uint8_t credential_type)
    {
        if (credential_type == CREDENTIAL_TYPE_NONE) {
            return false;
        }
        bool list_is_known = false;
        const auto index   = credential_type_index(endpoint, credential_type, list_is_known);
        if (!list_is_known) {
            return true;
        }
        return index.has_value();
    }

    bool command_class_user_credential_rules::credential_slot_is_supported(attribute_store::attribute endpoint, uint8_t credential_type, uint16_t credential_slot, bool zero_slot_is_supported)
    {
        if (credential_slot == FIRST_SLOT) {
            return zero_slot_is_supported;
        }
        bool list_is_known = false;
        const auto index   = credential_type_index(endpoint, credential_type, list_is_known);
        if (!list_is_known || !index.has_value()) {
            return !list_is_known;
        }
        const auto slots = reported_bytes(credential_capabilities(endpoint), static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::vg3_number_of_supported_credential_slots_per_credential_type));
        const auto count = u16_at(slots, *index);
        if (!count.has_value()) {
            return true;
        }
        return credential_slot >= 1 && credential_slot <= *count;
    }

    bool command_class_user_credential_rules::credential_length_is_supported(attribute_store::attribute endpoint, uint8_t credential_type, uint8_t length)
    {
        bool list_is_known = false;
        const auto index   = credential_type_index(endpoint, credential_type, list_is_known);
        if (!list_is_known || !index.has_value()) {
            if (credential_type == CREDENTIAL_TYPE_PIN) {
                return length >= PIN_LENGTH_MIN && length <= PIN_LENGTH_MAX;
            }
            return true;
        }
        const auto mins = reported_bytes(credential_capabilities(endpoint), static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::vg4_min_length_of_credential_data_per_credential_type));
        const auto maxs = reported_bytes(credential_capabilities(endpoint), static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::vg5_max_length_of_credential_data_per_credential_type));
        if (*index >= mins.size() || *index >= maxs.size()) {
            return true;
        }
        return length >= mins[*index] && length <= maxs[*index];
    }

    bool command_class_user_credential_rules::credential_set_is_valid(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot, uint8_t operation_type, uint8_t credential_length, const std::vector<uint8_t> &credential_data)
    {
        if (operation_type > OPERATION_DELETE || credential_length != credential_data.size()) {
            return false;
        }
        const bool delete_all = operation_type == OPERATION_DELETE && user_unique_identifier == DELETE_ALL_IDENTIFIER && credential_slot == FIRST_SLOT;
        if (delete_all) {
            return credential_length == 0 && credential_type_is_supported(endpoint, credential_type);
        }
        if (!user_identifier_is_supported(endpoint, user_unique_identifier, false) || !credential_type_is_supported(endpoint, credential_type)) {
            return false;
        }
        if (!credential_slot_is_supported(endpoint, credential_type, credential_slot, false)) {
            return false;
        }
        if (operation_type == OPERATION_DELETE) {
            return credential_length == 0;
        }
        if (!credential_length_is_supported(endpoint, credential_type, credential_length)) {
            return false;
        }
        if (credential_type == CREDENTIAL_TYPE_PIN) {
            return digits_only(credential_data);
        }
        return true;
    }

    bool command_class_user_credential_rules::credential_get_is_valid(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot)
    {
        if (!user_identifier_is_supported(endpoint, user_unique_identifier, true)) {
            return false;
        }
        if (credential_type == CREDENTIAL_TYPE_NONE && credential_slot == FIRST_SLOT) {
            return true;
        }
        return credential_type_is_supported(endpoint, credential_type) && credential_slot_is_supported(endpoint, credential_type, credential_slot, false);
    }

    bool command_class_user_credential_rules::credential_learn_start_is_valid(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot, uint8_t operation_type)
    {
        if (operation_type > OPERATION_MODIFY) {
            return false;
        }
        return user_identifier_is_supported(endpoint, user_unique_identifier, false) && credential_type_is_supported(endpoint, credential_type) && credential_slot_is_supported(endpoint, credential_type, credential_slot, false);
    }

    bool command_class_user_credential_rules::user_credential_association_set_is_valid(attribute_store::attribute endpoint, uint8_t credential_type, uint16_t credential_slot, uint16_t destination_user_unique_identifier)
    {
        return user_identifier_is_supported(endpoint, destination_user_unique_identifier, false) && credential_type_is_supported(endpoint, credential_type) && credential_slot_is_supported(endpoint, credential_type, credential_slot, false);
    }

    bool command_class_user_credential_rules::admin_pin_code_set_is_valid(attribute_store::attribute endpoint, uint8_t length, const std::vector<uint8_t> &admin_code)
    {
        auto group                        = credential_capabilities(endpoint);
        const bool admin_code_supported   = capability_flag_allows(group, static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::admin_code_support));
        const bool deactivation_supported = capability_flag_is_set(group, static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::admin_code_deactivation_support));
        if (!admin_code_supported) {
            return false;
        }
        if (length == 0) {
            return admin_code.empty() && deactivation_supported;
        }
        if (length != admin_code.size() || !digits_only(admin_code)) {
            return false;
        }
        return credential_length_is_supported(endpoint, CREDENTIAL_TYPE_PIN, length);
    }

    bool command_class_user_credential_rules::admin_pin_code_get_is_supported(attribute_store::attribute endpoint)
    {
        return capability_flag_allows(credential_capabilities(endpoint), static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::admin_code_support));
    }

    bool command_class_user_credential_rules::all_users_checksum_get_is_supported(attribute_store::attribute endpoint)
    {
        return capability_flag_allows(user_capabilities(endpoint), static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::all_users_checksum_support));
    }

    bool command_class_user_credential_rules::user_checksum_get_is_supported(attribute_store::attribute endpoint, uint16_t user_unique_identifier)
    {
        return capability_flag_allows(user_capabilities(endpoint), static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::user_checksum_support)) && user_identifier_is_supported(endpoint, user_unique_identifier, false);
    }

    bool command_class_user_credential_rules::credential_checksum_get_is_supported(attribute_store::attribute endpoint, uint8_t credential_type)
    {
        return capability_flag_allows(credential_capabilities(endpoint), static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::credential_checksum_support)) && credential_type_is_supported(endpoint, credential_type);
    }

    bool command_class_user_credential_rules::user_schedule_is_supported(attribute_store::attribute endpoint)
    {
        return capability_flag_is_set(user_capabilities(endpoint), static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::user_schedule_support));
    }

    bool command_class_user_credential_rules::all_users_checksum_is_supported(attribute_store::attribute endpoint)
    {
        return capability_flag_is_set(user_capabilities(endpoint), static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::all_users_checksum_support));
    }

}  // namespace zwave_command_class
