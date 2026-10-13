
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
#include <vector>

#include "command_class_schedule.hpp"
#include "command_class_schedule_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_schedule_attribute_store";

    command_class_schedule_attribute_store::command_class_schedule_attribute_store() {}

    attribute_store::attribute command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(attribute_store::attribute endpoint_node, uint8_t schedule_id_block)
    {
        for (auto report_group: endpoint_node.children(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::SCHEDULE_SUPPORTED_REPORT_GROUP))) {
            auto block_node = report_group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::schedule_id_block));
            if (block_node.is_valid() && block_node.reported_exists() && block_node.reported<uint8_t>() == schedule_id_block) {
                return report_group;
            }
        }
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    attribute_store::attribute command_class_schedule_attribute_store::find_or_create_supported_report_group_by_schedule_id_block(attribute_store::attribute endpoint_node, uint8_t schedule_id_block)
    {
        auto report_group = find_supported_report_group_by_schedule_id_block(endpoint_node, schedule_id_block);
        if (report_group.is_valid()) {
            return report_group;
        }
        report_group = endpoint_node.add_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::SCHEDULE_SUPPORTED_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::schedule_id_block)).set_reported<uint8_t>(schedule_id_block);
        return report_group;
    }

    attribute_store::attribute command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(attribute_store::attribute endpoint_node, uint8_t schedule_id_block)
    {
        for (auto report_group: endpoint_node.children(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_REPORT_GROUP))) {
            auto block_node = report_group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::schedule_id_block));
            if (block_node.is_valid() && block_node.reported_exists() && block_node.reported<uint8_t>() == schedule_id_block) {
                return report_group;
            }
        }
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    attribute_store::attribute command_class_schedule_attribute_store::find_or_create_supported_commands_report_group_by_schedule_id_block(attribute_store::attribute endpoint_node, uint8_t schedule_id_block)
    {
        auto report_group = find_supported_commands_report_group_by_schedule_id_block(endpoint_node, schedule_id_block);
        if (report_group.is_valid()) {
            return report_group;
        }
        report_group = endpoint_node.add_node(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::schedule_id_block)).set_reported<uint8_t>(schedule_id_block);
        return report_group;
    }

    sl_status_t command_class_schedule_attribute_store::on_schedule_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_schedule_attribute_map_t attribute_map)
    {
        uint8_t schedule_id_block = 0;
        schedule_id_block         = get_value_or_default(attribute_map, "schedule_id_block", schedule_id_block);

        auto group_node = find_or_create_supported_report_group_by_schedule_id_block(endpoint_node, schedule_id_block);

        schedule_supported_report_number_of_supported_schedule_id_t number_of_supported_schedule_id = 0;
        number_of_supported_schedule_id                                                             = get_value_or_default(attribute_map, "number_of_supported_schedule_id", number_of_supported_schedule_id);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_id)).set_reported(number_of_supported_schedule_id);

        uint8_t start_time_support = 0;
        start_time_support         = get_value_or_default(attribute_map, "start_time_support", start_time_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::start_time_support)).set_reported(start_time_support);

        uint8_t fallback_support = 0;
        fallback_support         = get_value_or_default(attribute_map, "fallback_support", fallback_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::fallback_support)).set_reported(fallback_support);

        uint8_t support_enable_disable = 0;
        support_enable_disable         = get_value_or_default(attribute_map, "support_enable_disable", support_enable_disable);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::support_enable_disable)).set_reported(support_enable_disable);

        schedule_supported_report_number_of_supported_cc_t number_of_supported_cc = 0;
        number_of_supported_cc                                                    = get_value_or_default(attribute_map, "number_of_supported_cc", number_of_supported_cc);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_cc)).set_reported(number_of_supported_cc);

        schedule_supported_report_vg1_t vg1;
        vg1 = get_value_or_default(attribute_map, "vg1", vg1);
        // Wire layout is supported_cc + properties2 (1 byte each). Store as a flat byte array.
        std::vector<uint8_t> vg1_bytes;
        vg1_bytes.reserve(vg1.size() * 2);
        for (const auto &item: vg1) {
            vg1_bytes.push_back(item.supported_cc);
            vg1_bytes.push_back(item.properties2.value);
        }
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::vg1)).set_reported(vg1_bytes);

        uint8_t supported_override_types = 0;
        supported_override_types         = get_value_or_default(attribute_map, "supported_override_types", supported_override_types);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::supported_override_types)).set_reported(supported_override_types);

        uint8_t override_support = 0;
        override_support         = get_value_or_default(attribute_map, "override_support", override_support);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::override_support)).set_reported(override_support);

        schedule_supported_report_number_of_supported_schedule_blocks_t number_of_supported_schedule_blocks = 0;
        number_of_supported_schedule_blocks                                                                 = get_value_or_default(attribute_map, "number_of_supported_schedule_blocks", number_of_supported_schedule_blocks);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_blocks)).set_reported(number_of_supported_schedule_blocks);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule_attribute_store::on_schedule_supported_commands_report_received_store(attribute_store::attribute endpoint_node, command_class_schedule_attribute_map_t attribute_map)
    {
        uint8_t schedule_id_block = 0;
        schedule_id_block         = get_value_or_default(attribute_map, "schedule_id_block", schedule_id_block);

        auto group_node = find_or_create_supported_commands_report_group_by_schedule_id_block(endpoint_node, schedule_id_block);

        schedule_supported_commands_report_command_class_list_length_t command_class_list_length = 0;
        command_class_list_length                                                                = get_value_or_default(attribute_map, "command_class_list_length", command_class_list_length);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::command_class_list_length)).set_reported(command_class_list_length);

        schedule_supported_commands_report_vg1_t vg1;
        vg1 = get_value_or_default(attribute_map, "vg1", vg1);
        // Flatten: command_class, list_length, then command bytes — same order as the Z-Wave frame.
        std::vector<uint8_t> vg1_bytes;
        for (const auto &item: vg1) {
            vg1_bytes.push_back(item.command_class);
            vg1_bytes.push_back(static_cast<uint8_t>(item.supported_command.size()));
            vg1_bytes.insert(vg1_bytes.end(), item.supported_command.begin(), item.supported_command.end());
        }
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::vg1)).set_reported(vg1_bytes);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
