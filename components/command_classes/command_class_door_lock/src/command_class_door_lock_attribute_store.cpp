
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
#include "command_class_door_lock_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_door_lock_attribute_store";

    command_class_door_lock_attribute_store::command_class_door_lock_attribute_store() {}

    sl_status_t command_class_door_lock_attribute_store::on_door_lock_operation_report_received_store(attribute_store::attribute endpoint_node, command_class_door_lock_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::DOOR_LOCK_OPERATION_REPORT_GROUP));

        door_lock_operation_report_current_door_lock_mode_t current_door_lock_mode = 0;
        current_door_lock_mode                                                     = get_value_or_default(attribute_map, "current_door_lock_mode", current_door_lock_mode);
        auto current_door_lock_mode_node                                           = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::current_door_lock_mode));
        current_door_lock_mode_node.set_reported<door_lock_operation_report_current_door_lock_mode_t>(current_door_lock_mode);

        door_lock_operation_report_current_door_lock_mode_t inside_door_handles_mode = 0;
        inside_door_handles_mode                                                     = get_value_or_default(attribute_map, "inside_door_handles_mode", inside_door_handles_mode);
        auto inside_door_handles_mode_node                                           = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::inside_door_handles_mode));
        inside_door_handles_mode_node.set_reported<uint8_t>(inside_door_handles_mode);

        uint8_t outside_door_handles_mode   = 0;
        outside_door_handles_mode           = get_value_or_default(attribute_map, "outside_door_handles_mode", outside_door_handles_mode);
        auto outside_door_handles_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::outside_door_handles_mode));
        outside_door_handles_mode_node.set_reported<uint8_t>(outside_door_handles_mode);

        door_lock_operation_report_door_condition_t door_condition = 0;
        door_condition                                             = get_value_or_default(attribute_map, "door_condition", door_condition);
        auto door_condition_node                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::door_condition));
        door_condition_node.set_reported<door_lock_operation_report_door_condition_t>(door_condition);

        door_lock_operation_report_remaining_lock_time_minutes_t remaining_lock_time_minutes = 0;
        remaining_lock_time_minutes                                                          = get_value_or_default(attribute_map, "remaining_lock_time_minutes", remaining_lock_time_minutes);
        auto remaining_lock_time_minutes_node                                                = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::remaining_lock_time_minutes));
        remaining_lock_time_minutes_node.set_reported<door_lock_operation_report_remaining_lock_time_minutes_t>(remaining_lock_time_minutes);

        door_lock_operation_report_remaining_lock_time_seconds_t remaining_lock_time_seconds = 0;
        remaining_lock_time_seconds                                                          = get_value_or_default(attribute_map, "remaining_lock_time_seconds", remaining_lock_time_seconds);
        auto remaining_lock_time_seconds_node                                                = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::remaining_lock_time_seconds));
        remaining_lock_time_seconds_node.set_reported<door_lock_operation_report_remaining_lock_time_seconds_t>(remaining_lock_time_seconds);

        door_lock_operation_report_target_door_lock_mode_t target_door_lock_mode = 0;
        target_door_lock_mode                                                    = get_value_or_default(attribute_map, "target_door_lock_mode", target_door_lock_mode);
        auto target_door_lock_mode_node                                          = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::target_door_lock_mode));
        target_door_lock_mode_node.set_reported<door_lock_operation_report_target_door_lock_mode_t>(target_door_lock_mode);

        door_lock_operation_report_duration_t duration = 0;
        duration                                       = get_value_or_default(attribute_map, "duration", duration);
        auto duration_node                             = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_operation_report_group_attributes_t::duration));
        duration_node.set_reported<door_lock_operation_report_duration_t>(duration);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_door_lock_attribute_store::on_door_lock_configuration_report_received_store(attribute_store::attribute endpoint_node, command_class_door_lock_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::DOOR_LOCK_CONFIGURATION_REPORT_GROUP));

        door_lock_configuration_report_operation_type_t operation_type = 0;
        operation_type                                                 = get_value_or_default(attribute_map, "operation_type", operation_type);
        auto operation_type_node                                       = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::operation_type));
        operation_type_node.set_reported<door_lock_configuration_report_operation_type_t>(operation_type);

        uint8_t inside_door_handles_enabled   = 0;
        inside_door_handles_enabled           = get_value_or_default(attribute_map, "inside_door_handles_enabled", inside_door_handles_enabled);
        auto inside_door_handles_enabled_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::inside_door_handles_enabled));
        inside_door_handles_enabled_node.set_reported<uint8_t>(inside_door_handles_enabled);

        uint8_t outside_door_handles_enabled   = 0;
        outside_door_handles_enabled           = get_value_or_default(attribute_map, "outside_door_handles_enabled", outside_door_handles_enabled);
        auto outside_door_handles_enabled_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::outside_door_handles_enabled));
        outside_door_handles_enabled_node.set_reported<uint8_t>(outside_door_handles_enabled);

        door_lock_configuration_report_lock_timeout_minutes_t lock_timeout_minutes = 0;
        lock_timeout_minutes                                                       = get_value_or_default(attribute_map, "lock_timeout_minutes", lock_timeout_minutes);
        auto lock_timeout_minutes_node                                             = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::lock_timeout_minutes));
        lock_timeout_minutes_node.set_reported<door_lock_configuration_report_lock_timeout_minutes_t>(lock_timeout_minutes);

        door_lock_configuration_report_lock_timeout_seconds_t lock_timeout_seconds = 0;
        lock_timeout_seconds                                                       = get_value_or_default(attribute_map, "lock_timeout_seconds", lock_timeout_seconds);
        auto lock_timeout_seconds_node                                             = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::lock_timeout_seconds));
        lock_timeout_seconds_node.set_reported<door_lock_configuration_report_lock_timeout_seconds_t>(lock_timeout_seconds);

        door_lock_configuration_report_auto_relock_time_t auto_relock_time = 0;
        auto_relock_time                                                   = get_value_or_default(attribute_map, "auto_relock_time", auto_relock_time);
        auto auto_relock_time_node                                         = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::auto_relock_time));
        auto_relock_time_node.set_reported<door_lock_configuration_report_auto_relock_time_t>(auto_relock_time);

        door_lock_configuration_report_hold_and_release_time_t hold_and_release_time = 0;
        hold_and_release_time                                                        = get_value_or_default(attribute_map, "hold_and_release_time", hold_and_release_time);
        auto hold_and_release_time_node                                              = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::hold_and_release_time));
        hold_and_release_time_node.set_reported<door_lock_configuration_report_hold_and_release_time_t>(hold_and_release_time);

        uint8_t ta   = 0;
        ta           = get_value_or_default(attribute_map, "ta", ta);
        auto ta_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::ta));
        ta_node.set_reported<uint8_t>(ta);

        uint8_t btb   = 0;
        btb           = get_value_or_default(attribute_map, "btb", btb);
        auto btb_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_configuration_report_group_attributes_t::btb));
        btb_node.set_reported<uint8_t>(btb);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_door_lock_attribute_store::on_door_lock_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_door_lock_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::DOOR_LOCK_CAPABILITIES_REPORT_GROUP));

        uint8_t supported_operation_type_bit_mask_length   = 0;
        supported_operation_type_bit_mask_length           = get_value_or_default(attribute_map, "supported_operation_type_bit_mask_length", supported_operation_type_bit_mask_length);
        auto supported_operation_type_bit_mask_length_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::supported_operation_type_bit_mask_length));
        supported_operation_type_bit_mask_length_node.set_reported<uint8_t>(supported_operation_type_bit_mask_length);

        std::vector<uint8_t> supported_operation_type_bit_mask_default = {};
        auto supported_operation_type_bit_mask                         = get_value_or_default(attribute_map, "supported_operation_type_bit_mask", supported_operation_type_bit_mask_default);
        auto supported_operation_type_bit_mask_node                    = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::supported_operation_type_bit_mask));
        supported_operation_type_bit_mask_node.set_reported<std::vector<uint8_t>>(supported_operation_type_bit_mask);

        door_lock_capabilities_report_supported_door_lock_mode_list_length_t supported_door_lock_mode_list_length = 0;
        supported_door_lock_mode_list_length                                                                      = get_value_or_default(attribute_map, "supported_door_lock_mode_list_length", supported_door_lock_mode_list_length);
        auto supported_door_lock_mode_list_length_node                                                            = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::supported_door_lock_mode_list_length));
        supported_door_lock_mode_list_length_node.set_reported<door_lock_capabilities_report_supported_door_lock_mode_list_length_t>(supported_door_lock_mode_list_length);

        std::vector<uint8_t> supported_door_lock_mode_default = {};
        auto supported_door_lock_mode                         = get_value_or_default(attribute_map, "supported_door_lock_mode", supported_door_lock_mode_default);
        auto supported_door_lock_mode_node                    = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::supported_door_lock_mode));
        supported_door_lock_mode_node.set_reported<std::vector<uint8_t>>(supported_door_lock_mode);

        uint8_t supported_inside_handle_modes_bitmask   = 0;
        supported_inside_handle_modes_bitmask           = get_value_or_default(attribute_map, "supported_inside_handle_modes_bitmask", supported_inside_handle_modes_bitmask);
        auto supported_inside_handle_modes_bitmask_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::supported_inside_handle_modes_bitmask));
        supported_inside_handle_modes_bitmask_node.set_reported<uint8_t>(supported_inside_handle_modes_bitmask);

        uint8_t supported_outside_handle_modes_bitmask   = 0;
        supported_outside_handle_modes_bitmask           = get_value_or_default(attribute_map, "supported_outside_handle_modes_bitmask", supported_outside_handle_modes_bitmask);
        auto supported_outside_handle_modes_bitmask_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::supported_outside_handle_modes_bitmask));
        supported_outside_handle_modes_bitmask_node.set_reported<uint8_t>(supported_outside_handle_modes_bitmask);

        door_lock_capabilities_report_supported_door_components_t supported_door_components = 0;
        supported_door_components                                                           = get_value_or_default(attribute_map, "supported_door_components", supported_door_components);
        auto supported_door_components_node                                                 = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::supported_door_components));
        supported_door_components_node.set_reported<door_lock_capabilities_report_supported_door_components_t>(supported_door_components);

        uint8_t btbs   = 0;
        btbs           = get_value_or_default(attribute_map, "btbs", btbs);
        auto btbs_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::btbs));
        btbs_node.set_reported<uint8_t>(btbs);

        uint8_t tas   = 0;
        tas           = get_value_or_default(attribute_map, "tas", tas);
        auto tas_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::tas));
        tas_node.set_reported<uint8_t>(tas);

        uint8_t hrs   = 0;
        hrs           = get_value_or_default(attribute_map, "hrs", hrs);
        auto hrs_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::hrs));
        hrs_node.set_reported<uint8_t>(hrs);

        uint8_t ars   = 0;
        ars           = get_value_or_default(attribute_map, "ars", ars);
        auto ars_node = group_node.emplace_node(static_cast<attribute_store_type_t>(door_lock_capabilities_report_group_attributes_t::ars));
        ars_node.set_reported<uint8_t>(ars);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
