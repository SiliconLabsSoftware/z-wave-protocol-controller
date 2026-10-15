
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

#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>
#include <vector>

#include "command_class_user_code.hpp"
#include "command_class_user_code_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_user_code_attribute_store";

    command_class_user_code_attribute_store::command_class_user_code_attribute_store() {}

    sl_status_t command_class_user_code_attribute_store::on_users_number_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP));

        users_number_report_supported_users_t supported_users = 0;
        supported_users                                       = get_value_or_default(attribute_map, "supported_users", supported_users);
        auto supported_users_node                             = group_node.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::supported_users));
        supported_users_node.set_reported<users_number_report_supported_users_t>(supported_users);

        users_number_report_extended_supported_users_t extended_supported_users = 0;
        extended_supported_users                                                = get_value_or_default(attribute_map, "extended_supported_users", extended_supported_users);
        auto extended_supported_users_node                                      = group_node.emplace_node(static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::extended_supported_users));
        extended_supported_users_node.set_reported<users_number_report_extended_supported_users_t>(extended_supported_users);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_user_code_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP));

        uint8_t supported_user_id_status_bit_mask_length = 0;
        supported_user_id_status_bit_mask_length         = get_value_or_default(attribute_map, "supported_user_id_status_bit_mask_length", supported_user_id_status_bit_mask_length);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_user_id_status_bit_mask_length)).set_reported<uint8_t>(supported_user_id_status_bit_mask_length);

        uint8_t acd_support = 0;
        acd_support         = get_value_or_default(attribute_map, "acd_support", acd_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::acd_support)).set_reported<uint8_t>(acd_support);

        uint8_t ac_support = 0;
        ac_support         = get_value_or_default(attribute_map, "ac_support", ac_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ac_support)).set_reported<uint8_t>(ac_support);

        std::vector<uint8_t> empty_mask;
        auto supported_user_id_status_bit_mask = get_value_or_default(attribute_map, "supported_user_id_status_bit_mask", empty_mask);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_user_id_status_bit_mask)).set_reported<std::vector<uint8_t>>(supported_user_id_status_bit_mask);

        uint8_t supported_keypad_modes_bit_mask_length = 0;
        supported_keypad_modes_bit_mask_length         = get_value_or_default(attribute_map, "supported_keypad_modes_bit_mask_length", supported_keypad_modes_bit_mask_length);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keypad_modes_bit_mask_length)).set_reported<uint8_t>(supported_keypad_modes_bit_mask_length);

        uint8_t mucs_support = 0;
        mucs_support         = get_value_or_default(attribute_map, "mucs_support", mucs_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::mucs_support)).set_reported<uint8_t>(mucs_support);

        uint8_t mucr_support = 0;
        mucr_support         = get_value_or_default(attribute_map, "mucr_support", mucr_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::mucr_support)).set_reported<uint8_t>(mucr_support);

        uint8_t ucc_support = 0;
        ucc_support         = get_value_or_default(attribute_map, "ucc_support", ucc_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ucc_support)).set_reported<uint8_t>(ucc_support);

        auto supported_keypad_modes_bit_mask = get_value_or_default(attribute_map, "supported_keypad_modes_bit_mask", empty_mask);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keypad_modes_bit_mask)).set_reported<std::vector<uint8_t>>(supported_keypad_modes_bit_mask);

        uint8_t supported_keys_bit_mask_length = 0;
        supported_keys_bit_mask_length         = get_value_or_default(attribute_map, "supported_keys_bit_mask_length", supported_keys_bit_mask_length);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keys_bit_mask_length)).set_reported<uint8_t>(supported_keys_bit_mask_length);

        auto supported_keys_bit_mask = get_value_or_default(attribute_map, "supported_keys_bit_mask", empty_mask);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keys_bit_mask)).set_reported<std::vector<uint8_t>>(supported_keys_bit_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_user_code_keypad_mode_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::USER_CODE_KEYPAD_MODE_REPORT_GROUP));

        uint8_t keypad_mode = 0;
        keypad_mode         = get_value_or_default(attribute_map, "keypad_mode", keypad_mode);
        group_node.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::keypad_mode)).set_reported<uint8_t>(keypad_mode);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_user_code_attribute_store::on_admin_code_report_received_store(attribute_store::attribute endpoint_node, command_class_user_code_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::ADMIN_CODE_REPORT_GROUP));

        uint8_t admin_code_length = 0;
        admin_code_length         = get_value_or_default(attribute_map, "admin_code_length", admin_code_length);
        group_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code_length)).set_reported<uint8_t>(admin_code_length);

        std::vector<uint8_t> empty_code;
        auto admin_code = get_value_or_default(attribute_map, "admin_code", empty_code);
        group_node.emplace_node(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code)).set_reported<std::vector<uint8_t>>(admin_code);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
