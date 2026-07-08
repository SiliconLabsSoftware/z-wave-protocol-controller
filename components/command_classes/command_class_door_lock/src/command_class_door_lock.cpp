
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

#include "command_class_door_lock.hpp"

#include "ZW_classcmd.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_door_lock";

    command_class_door_lock::command_class_door_lock() {}

    void command_class_door_lock::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        if (supported_version >= 4) {
            auto capabilities_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_get_group_attributes_t::DOOR_LOCK_CAPABILITIES_GET_GROUP));
            start_group_resolution(capabilities_get_node);
        }
        auto operation_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_get_group_attributes_t::DOOR_LOCK_OPERATION_GET_GROUP));
        start_group_resolution(operation_get_node);

        auto configuration_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_get_group_attributes_t::DOOR_LOCK_CONFIGURATION_GET_GROUP));
        start_group_resolution(configuration_get_node);
    }

    sl_status_t command_class_door_lock::on_door_lock_operation_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto door_lock_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_set_group_attributes_t::door_lock_mode));
        if (!door_lock_mode_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(door_lock_mode_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_door_lock::on_door_lock_configuration_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto operation_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::operation_type));
        if (!operation_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(operation_type_node, DESIRED_ATTRIBUTE);

        auto inside_door_handles_enabled_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::inside_door_handles_enabled));
        auto outside_door_handles_enabled_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::outside_door_handles_enabled));
        if (!inside_door_handles_enabled_node.desired_exists() || !outside_door_handles_enabled_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = inside_door_handles_enabled_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 4, .node = outside_door_handles_enabled_node, .node_value_state = DESIRED_ATTRIBUTE},
        });

        auto lock_timeout_minutes_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::lock_timeout_minutes));
        auto lock_timeout_seconds_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::lock_timeout_seconds));
        if (!lock_timeout_minutes_node.desired_exists() || !lock_timeout_seconds_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(lock_timeout_minutes_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(lock_timeout_seconds_node, DESIRED_ATTRIBUTE);

        auto auto_relock_time_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::auto_relock_time));
        auto hold_and_release_time_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::hold_and_release_time));
        if (!auto_relock_time_node.desired_exists() || !hold_and_release_time_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(auto_relock_time_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(hold_and_release_time_node, DESIRED_ATTRIBUTE);

        auto ta_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::ta));
        auto btb_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_set_group_attributes_t::btb));
        if (!ta_node.desired_exists() || !btb_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = ta_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 1, .node = btb_node, .node_value_state = DESIRED_ATTRIBUTE},
        });

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
