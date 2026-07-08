
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
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>
#include <vector>

// Base class
#include "command_class_s2.hpp"

#include "command_class_s2_attribute_store.hpp"
#include "command_class_s2_types.hpp"

namespace zwave_command_class
{
    // Log tag
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_s2_attribute_store";

    command_class_s2_attribute_store::command_class_s2_attribute_store()
    {
        register_attribute_types(attributes);
    }

    sl_status_t command_class_s2_attribute_store::on_s2_commands_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_s2_attribute_map_t attribute_map)
    {
        auto group_node         = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(s2_commands_supported_report_group_attributes_t::S2_COMMANDS_SUPPORTED_REPORT_GROUP));
        auto command_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(s2_commands_supported_report_group_attributes_t::command_class));

        s2_commands_supported_report_cc_list_t supported_cc_list = std::vector<uint8_t>();
        supported_cc_list                                        = get_value_or_default(attribute_map, "supported_cc_list", supported_cc_list);

        constexpr uint8_t command_class_mark = 0xEF;
        auto mark_iterator                   = std::find(supported_cc_list.begin(), supported_cc_list.end(), command_class_mark);
        supported_cc_list.erase(mark_iterator, supported_cc_list.end());

        command_class_node.set_reported<s2_commands_supported_report_cc_list_t>(supported_cc_list);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class