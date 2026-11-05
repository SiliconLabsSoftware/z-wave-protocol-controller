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

#include <string_view>
#include <variant>

#include "command_class_user_code_attribute_store.hpp"
#include "command_class_user_code_constants.hpp"
#include "command_class_user_code_rules.hpp"

namespace zwave_command_class
{
    using namespace command_class_user_code_types;
    using namespace command_class_user_code_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_code_attribute_store";

    template<typename T> void command_class_user_code_attribute_store::store_value(attribute_store::attribute parent, attribute_store_type_t type, T value)
    {
        parent.emplace_node(type).set_reported(value);
    }

    template<typename T> void command_class_user_code_attribute_store::store_if_present(attribute_store::attribute parent, attribute_store_type_t type, const command_class_user_code_attribute_map_t &map, const char *key)
    {
        auto it = map.find(key);
        if (it == map.end() || !std::holds_alternative<T>(it->second)) {
            return;
        }
        store_value(parent, type, std::get<T>(it->second));
    }

    attribute_store::attribute command_class_user_code_attribute_store::user_code_group(attribute_store::attribute endpoint, uint8_t identifier)
    {
        const auto group_type = static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::USER_CODE_REPORT_GROUP);
        const auto id_type    = static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_identifier);
        for (auto group: endpoint.children(group_type)) {
            auto id_node = group.child_by_type(id_type);
            if (id_node.reported_exists() && id_node.reported<uint8_t>() == identifier) {
                return group;
            }
        }
        auto group = endpoint.add_node(group_type);
        group.add_node(id_type).set_reported(identifier);
        return group;
    }

    void command_class_user_code_attribute_store::upsert_extended_block(attribute_store::attribute group, const extended_user_code_report_vg1_t_item_t &item)
    {
        const auto vg_type   = static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::vg1);
        const uint8_t msb    = static_cast<uint8_t>(item.user_identifier >> 8);
        const uint8_t lsb    = static_cast<uint8_t>(item.user_identifier & 0xFF);
        const uint8_t length = item.properties1.flags.extended_user_code_report_user_code_length;
        const auto packed    = command_class_user_code_rules::pack_user_code_block(item.user_identifier, item.user_id_status, length, item.user_code);
        for (auto node: group.children(vg_type)) {
            if (!node.reported_exists()) {
                continue;
            }
            const auto stored = node.reported<std::vector<uint8_t>>();
            if (stored.size() >= 2 && stored[0] == msb && stored[1] == lsb) {
                node.set_reported(packed);
                return;
            }
        }
        group.add_node(vg_type).set_reported(packed);
    }

    sl_status_t command_class_user_code_attribute_store::on_user_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        const uint8_t identifier = get_value_or_default<user_code_report_user_identifier_t>(attribute_map, "user_identifier", 0);
        const uint8_t status     = get_value_or_default<user_code_report_user_id_status_t>(attribute_map, "user_id_status", static_cast<uint8_t>(0));
        auto code                = get_value_or_default<user_code_report_user_code_t>(attribute_map, "user_code", user_code_report_user_code_t {});
        // CC:0063.01.01.11.009: Available is exactly four 0x00 bytes. The USER_CODE variant ends at 0x00, so that code arrives empty.
        if (status == STATUS_AVAILABLE && code.empty()) {
            code.assign(ERASED_CODE_LENGTH, 0);
        }
        auto group = user_code_group(endpoint_node, identifier);
        store_value(group, static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_id_status), status);
        store_value(group, static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_code), code);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_users_number_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP));
        store_if_present<users_number_report_supported_users_t>(group, static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::supported_users), attribute_map, "supported_users");
        store_if_present<users_number_report_extended_supported_users_t>(group, static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::extended_supported_users), attribute_map, "extended_supported_users");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_extended_user_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::EXTENDED_USER_CODE_REPORT_GROUP));
        store_if_present<extended_user_code_report_number_of_user_codes_t>(group, static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::number_of_user_codes), attribute_map, "number_of_user_codes");
        store_if_present<extended_user_code_report_next_user_identifier_t>(group, static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::next_user_identifier), attribute_map, "next_user_identifier");
        for (const auto &item: get_value_or_default<extended_user_code_report_vg1_t>(attribute_map, "vg1", extended_user_code_report_vg1_t {})) {
            upsert_extended_block(group, item);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_user_code_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP));
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_user_id_status_bit_mask_length), attribute_map, "supported_user_id_status_bit_mask_length");
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::acd_support), attribute_map, "acd_support");
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ac_support), attribute_map, "ac_support");
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_user_id_status_bit_mask), attribute_map, "supported_user_id_status_bit_mask");
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keypad_modes_bit_mask_length), attribute_map, "supported_keypad_modes_bit_mask_length");
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::mucs_support), attribute_map, "mucs_support");
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::mucr_support), attribute_map, "mucr_support");
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ucc_support), attribute_map, "ucc_support");
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keypad_modes_bit_mask), attribute_map, "supported_keypad_modes_bit_mask");
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keys_bit_mask_length), attribute_map, "supported_keys_bit_mask_length");
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keys_bit_mask), attribute_map, "supported_keys_bit_mask");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_user_code_keypad_mode_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::USER_CODE_KEYPAD_MODE_REPORT_GROUP));
        store_if_present<user_code_keypad_mode_report_keypad_mode_t>(group, static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::keypad_mode), attribute_map, "keypad_mode");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_admin_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::ADMIN_CODE_REPORT_GROUP));
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code_length), attribute_map, "admin_code_length");
        store_if_present<admin_code_report_admin_code_t>(group, static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code), attribute_map, "admin_code");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_user_code_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_checksum_report_group_attributes_t::USER_CODE_CHECKSUM_REPORT_GROUP));
        store_if_present<user_code_checksum_report_user_code_checksum_t>(group, static_cast<attribute_store_type_t>(user_code_checksum_report_group_attributes_t::user_code_checksum), attribute_map, "user_code_checksum");
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
