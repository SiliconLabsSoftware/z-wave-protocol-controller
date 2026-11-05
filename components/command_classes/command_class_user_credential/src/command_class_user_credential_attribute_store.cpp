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

#include <cstdint>
#include <string_view>
#include <vector>

#include "command_class_user_credential_attribute_store.hpp"

namespace zwave_command_class
{
    using namespace command_class_user_credential_types;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_credential_attribute_store";

    template<typename T> void command_class_user_credential_attribute_store::store_value(attribute_store::attribute parent, attribute_store_type_t type, T value)
    {
        parent.emplace_node(type).set_reported(value);
    }

    template<typename T> void command_class_user_credential_attribute_store::store_if_present(attribute_store::attribute parent, attribute_store_type_t type, const command_class_user_credential_attribute_map_t &map, const char *key, T fallback)
    {
        if (map.find(key) == map.end()) {
            return;
        }
        store_value(parent, type, get_value_or_default<T>(map, key, fallback));
    }

    template<typename Item, typename Extract> std::vector<uint8_t> command_class_user_credential_attribute_store::pack_bytes(const std::vector<Item> &items, Extract extract)
    {
        std::vector<uint8_t> bytes;
        bytes.reserve(items.size());
        for (const auto &item: items) {
            bytes.push_back(extract(item));
        }
        return bytes;
    }

    template<typename Item, typename Extract> std::vector<uint8_t> command_class_user_credential_attribute_store::pack_u16(const std::vector<Item> &items, Extract extract)
    {
        std::vector<uint8_t> bytes;
        bytes.reserve(items.size() * 2);
        for (const auto &item: items) {
            const uint16_t value = extract(item);
            bytes.push_back(static_cast<uint8_t>(value >> 8));
            bytes.push_back(static_cast<uint8_t>(value & 0xFF));
        }
        return bytes;
    }

    template<typename T> bool command_class_user_credential_attribute_store::reported_equals(attribute_store::attribute group, attribute_store_type_t attribute_type, T value)
    {
        auto node = group.child_by_type(attribute_type);
        return node.is_valid() && node.reported_exists() && node.reported<T>() == value;
    }

    template<typename Matches> attribute_store::attribute command_class_user_credential_attribute_store::group_for_report(attribute_store::attribute endpoint, attribute_store_type_t group_type, attribute_store_type_t identity_type, Matches matches)
    {
        for (auto group: endpoint.children(group_type)) {
            if (matches(group)) {
                return group;
            }
        }
        for (auto group: endpoint.children(group_type)) {
            auto identity = group.child_by_type(identity_type);
            if (!identity.is_valid() || !identity.reported_exists()) {
                return group;
            }
        }
        return endpoint.add_node(group_type);
    }

    sl_status_t command_class_user_credential_attribute_store::on_user_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        auto group    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_capabilities_report_group_attributes_t::USER_CAPABILITIES_REPORT_GROUP));
        using group_t = user_capabilities_report_group_attributes_t;
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::number_of_supported_user_unique_identifiers), attribute_map, "number_of_supported_user_unique_identifiers", 0);
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(group_t::supported_credential_rules_bit_mask), attribute_map, "supported_credential_rules_bit_mask", {});
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::max_length_of_user_name), attribute_map, "max_length_of_user_name", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::ascii_encoding_support), attribute_map, "ascii_encoding_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::extended_ascii_encoding_support), attribute_map, "extended_ascii_encoding_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::utf_16_encoding_support), attribute_map, "utf_16_encoding_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_checksum_support), attribute_map, "user_checksum_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::all_users_checksum_support), attribute_map, "all_users_checksum_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_schedule_support), attribute_map, "user_schedule_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::supported_user_types_bit_mask_length), attribute_map, "supported_user_types_bit_mask_length", 0);
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(group_t::supported_user_types_bit_mask), attribute_map, "supported_user_types_bit_mask", {});
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_credential_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        auto group    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_capabilities_report_group_attributes_t::CREDENTIAL_CAPABILITIES_REPORT_GROUP));
        using group_t = credential_capabilities_report_group_attributes_t;
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::admin_code_deactivation_support), attribute_map, "admin_code_deactivation_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::admin_code_support), attribute_map, "admin_code_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_checksum_support), attribute_map, "credential_checksum_support", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::number_of_supported_credential_types), attribute_map, "number_of_supported_credential_types", 0);
        if (attribute_map.find("vg1_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg1_credential_type_t>(attribute_map, "vg1_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg1_credential_type), pack_bytes(items, [](const auto &item) { return item.credential_type; }));
        }
        if (attribute_map.find("vg2_properties2_per_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg2_properties2_per_credential_type_t>(attribute_map, "vg2_properties2_per_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg2_properties2_per_credential_type), pack_bytes(items, [](const auto &item) { return item.properties2.value; }));
        }
        if (attribute_map.find("vg3_number_of_supported_credential_slots_per_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg3_number_of_supported_credential_slots_per_credential_type_t>(attribute_map, "vg3_number_of_supported_credential_slots_per_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg3_number_of_supported_credential_slots_per_credential_type), pack_u16(items, [](const auto &item) { return item.number_of_supported_credential_slots; }));
        }
        if (attribute_map.find("vg4_min_length_of_credential_data_per_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg4_min_length_of_credential_data_per_credential_type_t>(attribute_map, "vg4_min_length_of_credential_data_per_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg4_min_length_of_credential_data_per_credential_type), pack_bytes(items, [](const auto &item) { return item.min_length_of_credential_data; }));
        }
        if (attribute_map.find("vg5_max_length_of_credential_data_per_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg5_max_length_of_credential_data_per_credential_type_t>(attribute_map, "vg5_max_length_of_credential_data_per_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg5_max_length_of_credential_data_per_credential_type), pack_bytes(items, [](const auto &item) { return item.max_length_of_credential_data; }));
        }
        if (attribute_map.find("vg6_cl_recommended_timeout_per_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg6_cl_recommended_timeout_per_credential_type_t>(attribute_map, "vg6_cl_recommended_timeout_per_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg6_cl_recommended_timeout_per_credential_type), pack_bytes(items, [](const auto &item) { return item.cl_recommended_timeout; }));
        }
        if (attribute_map.find("vg7_cl_number_of_steps_per_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg7_cl_number_of_steps_per_credential_type_t>(attribute_map, "vg7_cl_number_of_steps_per_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg7_cl_number_of_steps_per_credential_type), pack_bytes(items, [](const auto &item) { return item.cl_number_of_steps; }));
        }
        if (attribute_map.find("vg8_maximum_credential_hash_length_per_credential_type") != attribute_map.end()) {
            const auto items = get_value_or_default<credential_capabilities_report_vg8_maximum_credential_hash_length_per_credential_type_t>(attribute_map, "vg8_maximum_credential_hash_length_per_credential_type", {});
            store_value(group, static_cast<attribute_store_type_t>(group_t::vg8_maximum_credential_hash_length_per_credential_type), pack_bytes(items, [](const auto &item) { return item.maximum_credential_hash_length; }));
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_user_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        using group_t          = user_report_group_attributes_t;
        const uint16_t user_id = get_value_or_default<uint16_t>(attribute_map, "user_unique_identifier", 0);
        const auto id_type     = static_cast<attribute_store_type_t>(group_t::user_unique_identifier);
        auto group             = group_for_report(endpoint_node, static_cast<attribute_store_type_t>(group_t::USER_REPORT_GROUP), id_type, [&](attribute_store::attribute candidate) { return reported_equals(candidate, id_type, user_id); });
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_report_type), attribute_map, "user_report_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::next_user_unique_identifier), attribute_map, "next_user_unique_identifier", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_modifier_type), attribute_map, "user_modifier_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::user_modifier_node_id), attribute_map, "user_modifier_node_id", 0);
        store_value(group, static_cast<attribute_store_type_t>(group_t::user_unique_identifier), user_id);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_type), attribute_map, "user_type", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_active_state), attribute_map, "user_active_state", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_rule), attribute_map, "credential_rule", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::expiring_timeout_minutes), attribute_map, "expiring_timeout_minutes", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_name_encoding), attribute_map, "user_name_encoding", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_name_length), attribute_map, "user_name_length", 0);
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(group_t::user_name), attribute_map, "user_name", {});
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_credential_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        using group_t          = credential_report_group_attributes_t;
        const uint16_t user_id = get_value_or_default<uint16_t>(attribute_map, "user_unique_identifier", 0);
        const uint8_t type     = get_value_or_default<uint8_t>(attribute_map, "credential_type", 0);
        const uint16_t slot    = get_value_or_default<uint16_t>(attribute_map, "credential_slot", 0);
        const auto id_type     = static_cast<attribute_store_type_t>(group_t::user_unique_identifier);
        auto group             = group_for_report(endpoint_node, static_cast<attribute_store_type_t>(group_t::CREDENTIAL_REPORT_GROUP), id_type, [&](attribute_store::attribute candidate) {
            return reported_equals(candidate, id_type, user_id) && reported_equals(candidate, static_cast<attribute_store_type_t>(group_t::credential_type), type) && reported_equals(candidate, static_cast<attribute_store_type_t>(group_t::credential_slot), slot);
        });
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_report_type), attribute_map, "credential_report_type", 0);
        store_value(group, static_cast<attribute_store_type_t>(group_t::user_unique_identifier), user_id);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_type), attribute_map, "credential_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::credential_slot), attribute_map, "credential_slot", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::crb), attribute_map, "crb", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_length), attribute_map, "credential_length", 0);
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(group_t::credential_data), attribute_map, "credential_data", {});
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_modifier_type), attribute_map, "credential_modifier_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::credential_modifier_node_id), attribute_map, "credential_modifier_node_id", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::next_credential_type), attribute_map, "next_credential_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::next_credential_slot), attribute_map, "next_credential_slot", 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_credential_learn_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        auto group    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(credential_learn_report_group_attributes_t::CREDENTIAL_LEARN_REPORT_GROUP));
        using group_t = credential_learn_report_group_attributes_t;
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_learn_status), attribute_map, "credential_learn_status", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::user_unique_identifier), attribute_map, "user_unique_identifier", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_type), attribute_map, "credential_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::credential_slot), attribute_map, "credential_slot", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_learn_steps_remaining), attribute_map, "credential_learn_steps_remaining", 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_user_credential_association_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        auto group    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_credential_association_report_group_attributes_t::USER_CREDENTIAL_ASSOCIATION_REPORT_GROUP));
        using group_t = user_credential_association_report_group_attributes_t;
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::credential_type), attribute_map, "credential_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::credential_slot), attribute_map, "credential_slot", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(group_t::destination_user_unique_identifier), attribute_map, "destination_user_unique_identifier", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::user_credential_association_status), attribute_map, "user_credential_association_status", 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_all_users_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        auto group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(all_users_checksum_report_group_attributes_t::ALL_USERS_CHECKSUM_REPORT_GROUP));
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(all_users_checksum_report_group_attributes_t::all_users_checksum), attribute_map, "all_users_checksum", 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_user_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        const uint16_t user_id = get_value_or_default<uint16_t>(attribute_map, "user_unique_identifier", 0);
        const auto id_type     = static_cast<attribute_store_type_t>(user_checksum_report_group_attributes_t::user_unique_identifier);
        auto group             = group_for_report(endpoint_node, static_cast<attribute_store_type_t>(user_checksum_report_group_attributes_t::USER_CHECKSUM_REPORT_GROUP), id_type, [&](attribute_store::attribute candidate) { return reported_equals(candidate, id_type, user_id); });
        store_value(group, id_type, user_id);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(user_checksum_report_group_attributes_t::user_checksum), attribute_map, "user_checksum", 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_credential_checksum_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        const uint8_t type   = get_value_or_default<uint8_t>(attribute_map, "credential_type", 0);
        const auto type_node = static_cast<attribute_store_type_t>(credential_checksum_report_group_attributes_t::credential_type);
        auto group           = group_for_report(endpoint_node, static_cast<attribute_store_type_t>(credential_checksum_report_group_attributes_t::CREDENTIAL_CHECKSUM_REPORT_GROUP), type_node, [&](attribute_store::attribute candidate) { return reported_equals(candidate, type_node, type); });
        store_if_present<uint8_t>(group, type_node, attribute_map, "credential_type", 0);
        store_if_present<uint16_t>(group, static_cast<attribute_store_type_t>(credential_checksum_report_group_attributes_t::credential_checksum), attribute_map, "credential_checksum", 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_credential_attribute_store::on_admin_pin_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_credential_attribute_map_t attribute_map)
    {
        auto group    = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_pin_code_report_group_attributes_t::ADMIN_PIN_CODE_REPORT_GROUP));
        using group_t = admin_pin_code_report_group_attributes_t;
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::admin_pin_code_length), attribute_map, "admin_pin_code_length", 0);
        store_if_present<uint8_t>(group, static_cast<attribute_store_type_t>(group_t::admin_pin_code_operation_result), attribute_map, "admin_pin_code_operation_result", 0);
        store_if_present<std::vector<uint8_t>>(group, static_cast<attribute_store_type_t>(group_t::admin_code), attribute_map, "admin_code", {});
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
