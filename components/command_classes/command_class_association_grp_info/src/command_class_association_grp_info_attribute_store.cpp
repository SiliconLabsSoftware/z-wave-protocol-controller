
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

#include "command_class_association_grp_info.hpp"

#include "command_class_association_grp_info_attribute_store.hpp"
#include "command_class_association_grp_info_attributes.hpp"

namespace zwave_command_class
{
    using namespace command_class_association_grp_info_types;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_association_grp_info_attribute_store";

    command_class_association_grp_info_attribute_store::command_class_association_grp_info_attribute_store()
    {
        register_attribute_types({
          {AGI_ZPC_GROUP, "AGI ZPC Group", ATTRIBUTE_HOME_ID, U8_STORAGE_TYPE},
          {node_ids, "AGI ZPC Group NodeIDs", AGI_ZPC_GROUP, BYTE_ARRAY_STORAGE_TYPE},
          {endpoint_associations, "AGI ZPC Group Endpoint Associations", AGI_ZPC_GROUP, BYTE_ARRAY_STORAGE_TYPE},
          {max_nodes_supported, "AGI ZPC Group Max Nodes Supported", AGI_ZPC_GROUP, U8_STORAGE_TYPE},
          {lifeline_command_list, "AGI ZPC Group Lifeline Command List", AGI_ZPC_GROUP, BYTE_ARRAY_STORAGE_TYPE},
          {AGI_NODE_GROUP, "AGI Node Group", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {group_name, "AGI Group Name", AGI_NODE_GROUP, BYTE_ARRAY_STORAGE_TYPE},
          {group_command_list, "AGI Group Command List", AGI_NODE_GROUP, BYTE_ARRAY_STORAGE_TYPE},
          {group_profile, "AGI Group Profile", AGI_NODE_GROUP, U16_STORAGE_TYPE},
        });
    }

    sl_status_t command_class_association_grp_info_attribute_store::on_association_group_name_report_received_store(attribute_store::attribute endpoint_node, command_class_association_grp_info_attribute_map_t attribute_map)
    {
        association_group_name_report_grouping_identifier_t grouping_identifier = 0;
        grouping_identifier                                                     = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        association_group_name_report_length_of_name_t length_of_name = 0;
        length_of_name                                                = get_value_or_default(attribute_map, "length_of_name", length_of_name);

        association_group_name_report_name_t name = {};
        name                                      = get_value_or_default(attribute_map, "name", name);

        auto parent_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_report_group_attributes_t::ASSOCIATION_GROUP_NAME_REPORT_GROUP));

        auto grouping_identifier_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_report_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_reported<association_group_name_report_grouping_identifier_t>(grouping_identifier);

        auto length_of_name_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_report_group_attributes_t::length_of_name));
        length_of_name_node.set_reported<association_group_name_report_length_of_name_t>(length_of_name);

        auto name_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_report_group_attributes_t::name));
        name_node.set_reported<association_group_name_report_name_t>(name);

        auto group_node    = endpoint_node.emplace_node(agi_group_attributes_t::AGI_NODE_GROUP, grouping_identifier);
        auto agi_name_node = group_node.emplace_node(agi_group_attributes_t::group_name);
        agi_name_node.set_reported<association_group_name_report_name_t>(name);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info_attribute_store::on_association_group_info_report_received_store(attribute_store::attribute endpoint_node, command_class_association_grp_info_attribute_map_t attribute_map)
    {
        uint8_t group_count = 0;
        group_count         = get_value_or_default(attribute_map, "group_count", group_count);

        uint8_t dynamic_info = 0;
        dynamic_info         = get_value_or_default(attribute_map, "dynamic_info", dynamic_info);

        uint8_t list_mode = 0;
        list_mode         = get_value_or_default(attribute_map, "list_mode", list_mode);

        association_group_info_report_vg1_t vg1 = {};
        vg1                                     = get_value_or_default(attribute_map, "vg1", vg1);

        auto parent_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::ASSOCIATION_GROUP_INFO_REPORT_GROUP));

        auto group_count_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::group_count));
        group_count_node.set_reported<uint8_t>(group_count);

        auto dynamic_info_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::dynamic_info));
        dynamic_info_node.set_reported<uint8_t>(dynamic_info);

        auto list_mode_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::list_mode));
        list_mode_node.set_reported<uint8_t>(list_mode);

        if (!vg1.empty()) {
            std::vector<uint8_t> vg1_serialized;
            for (const auto &item: vg1) {
                vg1_serialized.push_back(item.grouping_identifier);
                vg1_serialized.push_back(item.mode);
                vg1_serialized.push_back(item.profile1);
                vg1_serialized.push_back(item.profile2);
                vg1_serialized.push_back(item.reserved);
                vg1_serialized.push_back(static_cast<uint8_t>(item.event_code >> 8));
                vg1_serialized.push_back(static_cast<uint8_t>(item.event_code & 0xFF));
            }
            auto vg1_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::vg1));
            vg1_node.set_reported<std::vector<uint8_t>>(vg1_serialized);
        }

        for (const auto &item: vg1) {
            auto group_node   = endpoint_node.emplace_node(agi_group_attributes_t::AGI_NODE_GROUP, item.grouping_identifier);
            uint16_t profile  = (static_cast<uint16_t>(item.profile1) << 8) | item.profile2;
            auto profile_node = group_node.emplace_node(agi_group_attributes_t::group_profile);
            profile_node.set_reported<uint16_t>(profile);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info_attribute_store::on_association_group_command_list_report_received_store(attribute_store::attribute endpoint_node, command_class_association_grp_info_attribute_map_t attribute_map)
    {
        association_group_command_list_report_grouping_identifier_t grouping_identifier = 0;
        grouping_identifier                                                             = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        association_group_command_list_report_list_length_t list_length = 0;
        list_length                                                     = get_value_or_default(attribute_map, "list_length", list_length);

        association_group_command_list_report_command_t command = {};
        command                                                 = get_value_or_default(attribute_map, "command", command);

        auto parent_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_report_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_REPORT_GROUP));

        auto grouping_identifier_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_report_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_reported<association_group_command_list_report_grouping_identifier_t>(grouping_identifier);

        auto list_length_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_report_group_attributes_t::list_length));
        list_length_node.set_reported<association_group_command_list_report_list_length_t>(list_length);

        auto command_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_report_group_attributes_t::command));
        command_node.set_reported<association_group_command_list_report_command_t>(command);

        auto group_node    = endpoint_node.emplace_node(agi_group_attributes_t::AGI_NODE_GROUP, grouping_identifier);
        auto cmd_list_node = group_node.emplace_node(agi_group_attributes_t::group_command_list);
        cmd_list_node.set_reported<association_group_command_list_report_command_t>(command);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
