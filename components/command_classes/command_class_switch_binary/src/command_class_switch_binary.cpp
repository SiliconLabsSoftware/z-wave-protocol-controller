
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
#include "command_class_switch_binary.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_binary";

    command_class_switch_binary::command_class_switch_binary()
    {
        // Constructor body - can be empty or contain initialization logic
    }

    void command_class_switch_binary::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_get_group_attributes_t::SWITCH_BINARY_GET_GROUP));
        start_group_resolution(group_node);
    }

    sl_status_t command_class_switch_binary::on_switch_binary_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto target_value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::target_value));
        if (!target_value_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(target_value_node, DESIRED_ATTRIBUTE);

        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::duration));
        if (!duration_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(duration_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class