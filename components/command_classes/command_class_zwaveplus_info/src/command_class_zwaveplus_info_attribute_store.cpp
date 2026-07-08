
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
#include "command_class_zwaveplus_info.hpp"

#include "command_class_zwaveplus_info_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_zwaveplus_info_attribute_store";

    command_class_zwaveplus_info_attribute_store::command_class_zwaveplus_info_attribute_store() {}

    sl_status_t command_class_zwaveplus_info_attribute_store::on_zwaveplus_info_report_received_store(attribute_store::attribute endpoint_node, command_class_zwaveplus_info_attribute_map_t attribute_map)
    {
        zwaveplus_info_report_z_wave_plus_version_t zwave_plus_info_version = 0;
        zwaveplus_info_report_role_type_t role_type                         = 0;
        zwaveplus_info_report_node_type_t node_type                         = 0;
        zwaveplus_info_report_installer_icon_type_t installer_icon_type     = 0;
        zwaveplus_info_report_user_icon_type_t user_icon_type               = 0;

        zwave_plus_info_version = get_value_or_default(attribute_map, "z_wave_plus_version", zwave_plus_info_version);
        role_type               = get_value_or_default(attribute_map, "role_type", role_type);
        node_type               = get_value_or_default(attribute_map, "node_type", node_type);
        installer_icon_type     = get_value_or_default(attribute_map, "installer_icon_type", installer_icon_type);
        user_icon_type          = get_value_or_default(attribute_map, "user_icon_type", user_icon_type);

        auto group_node                   = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(zwaveplus_info_report_group_attributes_t::ZWAVEPLUS_INFO_REPORT_GROUP));
        auto zwave_plus_info_version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(zwaveplus_info_report_group_attributes_t::z_wave_plus_version));
        zwave_plus_info_version_node.set_reported<zwaveplus_info_report_z_wave_plus_version_t>(zwave_plus_info_version);

        auto role_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(zwaveplus_info_report_group_attributes_t::role_type));
        role_type_node.set_reported<zwaveplus_info_report_role_type_t>(role_type);

        auto node_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(zwaveplus_info_report_group_attributes_t::node_type));
        node_type_node.set_reported<zwaveplus_info_report_node_type_t>(node_type);

        auto installer_icon_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(zwaveplus_info_report_group_attributes_t::installer_icon_type));
        installer_icon_type_node.set_reported<zwaveplus_info_report_installer_icon_type_t>(installer_icon_type);

        auto user_icon_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(zwaveplus_info_report_group_attributes_t::user_icon_type));
        user_icon_type_node.set_reported<zwaveplus_info_report_user_icon_type_t>(user_icon_type);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class