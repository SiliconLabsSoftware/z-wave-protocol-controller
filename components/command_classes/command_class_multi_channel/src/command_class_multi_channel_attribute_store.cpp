
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

// Base class
#include "command_class_multi_channel.hpp"

#include "command_class_multi_channel_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_multi_channel_attribute_store";

    command_class_multi_channel_attribute_store::command_class_multi_channel_attribute_store() {}

    sl_status_t command_class_multi_channel_attribute_store::on_multi_channel_end_point_report_received_store(attribute_store::attribute endpoint_node, command_class_multi_channel_attribute_map_t attribute_map)
    {
        uint8_t identical             = 0;
        uint8_t dynamic               = 0;
        uint8_t individual_end_points = 0;
        uint8_t aggregated_end_points = 0;

        identical             = get_value_or_default(attribute_map, "identical", identical);
        dynamic               = get_value_or_default(attribute_map, "dynamic", dynamic);
        individual_end_points = get_value_or_default(attribute_map, "individual_end_points", individual_end_points);
        aggregated_end_points = get_value_or_default(attribute_map, "aggregated_end_points", aggregated_end_points);

        auto group_node                 = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_report_group_attributes_t::MULTI_CHANNEL_END_POINT_REPORT_GROUP));
        auto identical_node             = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_report_group_attributes_t::identical));
        auto dynamic_node               = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_report_group_attributes_t::dynamic));
        auto individual_end_points_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_report_group_attributes_t::individual_end_points));
        auto aggregated_end_points_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_report_group_attributes_t::aggregated_end_points));

        identical_node.set_reported<uint8_t>(identical);
        dynamic_node.set_reported<uint8_t>(dynamic);
        individual_end_points_node.set_reported<uint8_t>(individual_end_points);
        aggregated_end_points_node.set_reported<uint8_t>(aggregated_end_points);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_attribute_store::on_multi_channel_capability_report_received_store(attribute_store::attribute endpoint_node, command_class_multi_channel_attribute_map_t attribute_map)
    {
        multi_channel_capability_report_generic_device_class_t generic_device_class   = 0;
        multi_channel_capability_report_specific_device_class_t specific_device_class = 0;
        multi_channel_capability_report_command_class_t command_class                 = {};

        uint8_t end_point = 0;
        end_point         = get_value_or_default(attribute_map, "end_point", end_point);

        uint8_t dynamic = 0;
        dynamic         = get_value_or_default(attribute_map, "dynamic", dynamic);

        generic_device_class  = get_value_or_default(attribute_map, "generic_device_class", generic_device_class);
        specific_device_class = get_value_or_default(attribute_map, "specific_device_class", specific_device_class);
        command_class         = get_value_or_default(attribute_map, "command_class", command_class);

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_report_group_attributes_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP));

        auto end_point_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_report_group_attributes_t::end_point));
        end_point_node.set_reported<uint8_t>(end_point);

        auto dynamic_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_report_group_attributes_t::dynamic));
        dynamic_node.set_reported<uint8_t>(dynamic);

        auto generic_device_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_report_group_attributes_t::generic_device_class));
        generic_device_class_node.set_reported<uint8_t>(generic_device_class);

        auto specific_device_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_report_group_attributes_t::specific_device_class));
        specific_device_class_node.set_reported<uint8_t>(specific_device_class);

        auto command_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_report_group_attributes_t::command_class));
        command_class_node.set_reported<multi_channel_capability_report_command_class_t>(command_class);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_attribute_store::on_multi_channel_end_point_find_report_received_store(attribute_store::attribute endpoint_node, command_class_multi_channel_attribute_map_t attribute_map)
    {

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_report_group_attributes_t::MULTI_CHANNEL_END_POINT_FIND_REPORT_GROUP));

        multi_channel_end_point_find_report_reports_to_follow_t reports_to_follow = 0;
        reports_to_follow                                                         = get_value_or_default(attribute_map, "reports_to_follow", reports_to_follow);
        auto reports_to_follow_node                                               = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_report_group_attributes_t::reports_to_follow));
        reports_to_follow_node.set_reported<multi_channel_end_point_find_report_reports_to_follow_t>(reports_to_follow);

        multi_channel_end_point_find_report_generic_device_class_t generic_device_class = 0;
        generic_device_class                                                            = get_value_or_default(attribute_map, "generic_device_class", generic_device_class);
        auto generic_device_class_node                                                  = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_report_group_attributes_t::generic_device_class));
        generic_device_class_node.set_reported<multi_channel_end_point_find_report_generic_device_class_t>(generic_device_class);

        multi_channel_end_point_find_report_specific_device_class_t specific_device_class = 0;
        specific_device_class                                                             = get_value_or_default(attribute_map, "specific_device_class", specific_device_class);
        auto specific_device_class_node                                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_report_group_attributes_t::specific_device_class));
        specific_device_class_node.set_reported<multi_channel_end_point_find_report_specific_device_class_t>(specific_device_class);

        multi_channel_end_point_find_report_vg_t vg;
        vg           = get_value_or_default(attribute_map, "vg", vg);
        auto vg_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_report_group_attributes_t::vg));
        vg_node.set_reported<multi_channel_end_point_find_report_vg_t>(vg);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class