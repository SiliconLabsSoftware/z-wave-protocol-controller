
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
#include "command_class_battery.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_battery";

    command_class_battery::command_class_battery()
    {
        // Constructor body - can be empty or contain initialization logic
    }

    void command_class_battery::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(battery_get_group_attributes_t::BATTERY_GET_GROUP));
        command_class_battery_core::start_group_resolution(group_node);
    }

}  // namespace zwave_command_class