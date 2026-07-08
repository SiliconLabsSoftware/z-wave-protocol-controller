
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

#include "command_class_notification.hpp"
#include "command_class_notification_attributes.hpp"
#include "command_class_notification_constants.hpp"

#include "component_connector.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "command_class_association_grp_info_constants.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"

#include "attribute.hpp"
#include "zwave_network_management.h"
#include "log.h"

namespace zwave_command_class
{
    using namespace command_class_notification_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_notification";

    command_class_notification::command_class_notification()
    {
        using namespace command_class_association_grp_info_types;
        component_connector connector;
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_check_command_list_result_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST_RESULT,
                                                                                                                                  [](const component_connector_agi_check_command_list_result_t &result) { return zwave_command_class::command_class_notification::on_check_command_list_result(result); });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_groupings_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT,
                                                                                                                          [](const component_connector_agi_groupings_payload_t &payload) { return zwave_command_class::command_class_notification::on_agi_group_command_list_report(payload); });
    }

    void command_class_notification::fire_check_command_in_group_list(attribute_store::attribute endpoint_node)
    {
        using namespace command_class_association_grp_info_types;
        constexpr uint8_t notification_report_cmd = static_cast<uint8_t>(command_class_notification_commands_t::COMMAND_CLASS_NOTIFICATION_NOTIFICATION_REPORT);

        component_connector_agi_check_command_list_payload_t payload;
        payload.device_endpoint_node = endpoint_node;
        payload.command_class_id     = cc_properties.command_class_id;
        payload.command_id           = notification_report_cmd;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST), payload);
    }

    void command_class_notification::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(notification_cc_attributes_t::NOTIFICATION_CC_GROUP));

        fire_check_command_in_group_list(endpoint_node);

        if (supported_version >= 2) {
            auto supported_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(notification_supported_get_group_attributes_t::NOTIFICATION_SUPPORTED_GET_GROUP));
            start_group_resolution(supported_get_node);
        }
    }

    sl_status_t command_class_notification::on_agi_group_command_list_report(const command_class_association_grp_info_types::component_connector_agi_groupings_payload_t &payload)
    {
        auto cc_group_node = payload.device_endpoint_node.child_by_type(static_cast<attribute_store_type_t>(notification_cc_attributes_t::NOTIFICATION_CC_GROUP));
        if (!cc_group_node.is_valid()) {
            return SL_STATUS_OK;
        }

        auto mode_node = cc_group_node.child_by_type(static_cast<attribute_store_type_t>(notification_cc_attributes_t::notification_mode));
        if (mode_node.is_valid() && mode_node.reported_exists() && mode_node.reported<uint8_t>() == NOTIFICATION_MODE_PUSH) {
            return SL_STATUS_OK;
        }

        fire_check_command_in_group_list(payload.device_endpoint_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification::on_check_command_list_result(const command_class_association_grp_info_types::component_connector_agi_check_command_list_result_t &result)
    {
        auto cc_group_node = result.device_endpoint_node.child_by_type(static_cast<attribute_store_type_t>(notification_cc_attributes_t::NOTIFICATION_CC_GROUP));
        if (!cc_group_node.is_valid()) {
            return SL_STATUS_FAIL;
        }

        auto mode_node              = cc_group_node.emplace_node(static_cast<attribute_store_type_t>(notification_cc_attributes_t::notification_mode));
        const bool already_push     = mode_node.reported_exists() && mode_node.reported<uint8_t>() == NOTIFICATION_MODE_PUSH;
        const uint8_t resolved_mode = result.found ? NOTIFICATION_MODE_PUSH : NOTIFICATION_MODE_PULL;

        // Avoid downgrading a confirmed PUSH mode back to PULL if AGI data is re-reported.
        if (already_push && resolved_mode == NOTIFICATION_MODE_PULL) {
            return SL_STATUS_OK;
        }

        mode_node.set_reported<uint8_t>(resolved_mode);
        sl_log_info(LOG_TAG.data(), "Notification mode set to %s", result.found ? "Push" : "Pull");

        // Only fire the association set when we transition into PUSH mode to avoid duplicates.
        if (resolved_mode == NOTIFICATION_MODE_PUSH && !already_push) {
            component_connector_association_set_payload_t set_payload;
            set_payload.endpoint_node       = result.device_endpoint_node;
            set_payload.grouping_identifier = command_class_association_grp_info_constants::LIFELINE_GROUP_ID;
            set_payload.node_id             = zwave_network_management_get_node_id();
            set_payload.endpoint_id         = 0;

            component_connector connector;
            connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SET), set_payload);
        }

        return SL_STATUS_OK;
    }

    uint8_t command_class_notification::get_notification_mode(attribute_store::attribute endpoint_node)
    {
        auto cc_group_node = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(notification_cc_attributes_t::NOTIFICATION_CC_GROUP));
        if (!cc_group_node.is_valid()) {
            return NOTIFICATION_MODE_PULL;
        }
        auto mode_node = cc_group_node.child_by_type(static_cast<attribute_store_type_t>(notification_cc_attributes_t::notification_mode));
        if (!mode_node.is_valid() || !mode_node.reported_exists()) {
            return NOTIFICATION_MODE_PULL;
        }
        return mode_node.reported<uint8_t>();
    }

    std::vector<uint8_t> command_class_notification::extract_supported_types(const std::vector<uint8_t> &bit_mask)
    {
        std::vector<uint8_t> types;
        for (size_t byte_idx = 0; byte_idx < bit_mask.size(); ++byte_idx) {
            for (uint8_t bit = 0; bit < 8; ++bit) {
                if ((bit_mask[byte_idx] & (1 << bit)) != 0) {
                    types.push_back(static_cast<uint8_t>((byte_idx * 8) + bit));
                }
            }
        }
        return types;
    }

    std::vector<uint8_t> command_class_notification::get_supported_types_from_store(attribute_store::attribute endpoint)
    {
        auto report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(notification_supported_report_group_attributes_t::NOTIFICATION_SUPPORTED_REPORT_GROUP));
        if (!report_group.is_valid()) {
            return {};
        }
        auto bit_mask_node = report_group.child_by_type(static_cast<attribute_store_type_t>(notification_supported_report_group_attributes_t::bit_mask));
        if (!bit_mask_node.is_valid() || !bit_mask_node.reported_exists()) {
            return {};
        }
        return extract_supported_types(bit_mask_node.reported<std::vector<uint8_t>>());
    }

    std::optional<uint8_t> command_class_notification::find_next_type(const std::vector<uint8_t> &types, uint8_t current_type)
    {
        auto it = std::find(types.begin(), types.end(), current_type);
        if (it != types.end()) {
            ++it;
        }
        if (it != types.end()) {
            return *it;
        }
        return std::nullopt;
    }

    void command_class_notification::start_event_supported_get(attribute_store::attribute endpoint, uint8_t notification_type)
    {
        auto event_get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(event_supported_get_group_attributes_t::EVENT_SUPPORTED_GET_GROUP));
        auto notif_type_node = event_get_group.emplace_node(static_cast<attribute_store_type_t>(event_supported_get_group_attributes_t::notification_type));
        notif_type_node.set_desired(notification_type);
        start_group_resolution(event_get_group);
    }

    void command_class_notification::start_notification_get(attribute_store::attribute endpoint, uint8_t notification_type)
    {
        uint8_t mode        = get_notification_mode(endpoint);
        uint8_t event_value = (mode == NOTIFICATION_MODE_PUSH) ? NOTIFICATION_GET_EVENT_TYPE_STATUS : NOTIFICATION_GET_EVENT_PULL;

        auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::NOTIFICATION_GET_GROUP));

        auto v1_alarm_type_node = get_group.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::v1_alarm_type));
        v1_alarm_type_node.set_desired<uint8_t>(0);

        auto notif_type_node = get_group.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::notification_type));
        notif_type_node.set_desired(notification_type);

        auto event_node = get_group.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::event));
        event_node.set_desired<uint8_t>(event_value);

        start_group_resolution(get_group);
    }

    sl_status_t command_class_notification::on_notification_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_notification_attribute_map_t payload)
    {
        (void)connection_info;

        auto bit_mask        = get_value_or_default(payload, "bit_mask", notification_supported_report_bit_mask_t {});
        auto supported_types = extract_supported_types(bit_mask);

        if (supported_types.empty()) {
            sl_log_debug(LOG_TAG.data(), "No supported notification types found");
            return SL_STATUS_OK;
        }

        uint8_t supported_version = endpoint_supported_version(endpoint);

        if (supported_version >= 3) {
            start_event_supported_get(endpoint, supported_types[0]);
        } else {
            start_notification_get(endpoint, supported_types[0]);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification::on_event_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_notification_attribute_map_t payload)
    {
        (void)connection_info;

        uint8_t reported_type = get_value_or_default(payload, "notification_type", static_cast<uint8_t>(0));

        // Make sure this report matches the notification type we actually asked for.
        // Otherwise an out-of-order or unsolicited report could cause us to skip the
        // rest of the event-supported-get chain and jump straight to notification-get.
        auto event_supported_get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(event_supported_get_group_attributes_t::EVENT_SUPPORTED_GET_GROUP));
        if (!event_supported_get_group.is_valid()) {
            return SL_STATUS_OK;
        }

        auto desired_type_node = event_supported_get_group.child_by_type(static_cast<attribute_store_type_t>(event_supported_get_group_attributes_t::notification_type));
        if (!desired_type_node.is_valid() || !desired_type_node.desired_exists()) {
            return SL_STATUS_OK;
        }
        if (desired_type_node.desired<uint8_t>() != reported_type) {
            return SL_STATUS_OK;
        }

        auto types = get_supported_types_from_store(endpoint);
        auto next  = find_next_type(types, reported_type);

        if (next.has_value()) {
            start_event_supported_get(endpoint, *next);
        } else if (!types.empty()) {
            start_notification_get(endpoint, types[0]);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification::on_notification_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_notification_attribute_map_t payload)
    {
        (void)connection_info;

        uint8_t reported_type = get_value_or_default(payload, "notification_type", static_cast<uint8_t>(0));

        auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::NOTIFICATION_GET_GROUP));
        if (!get_group.is_valid()) {
            return SL_STATUS_OK;
        }

        auto desired_type_node = get_group.child_by_type(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::notification_type));
        if (!desired_type_node.is_valid() || !desired_type_node.desired_exists()) {
            return SL_STATUS_OK;
        }
        if (desired_type_node.desired<uint8_t>() != reported_type) {
            return SL_STATUS_OK;
        }

        auto types = get_supported_types_from_store(endpoint);
        auto next  = find_next_type(types, reported_type);
        if (next.has_value()) {
            start_notification_get(endpoint, *next);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification::on_notification_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        const auto supported_version = endpoint_supported_version(group_node.parent());

        auto v1_alarm_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::v1_alarm_type));
        if (!v1_alarm_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(v1_alarm_type_node, DESIRED_ATTRIBUTE);

        auto notification_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::notification_type));
        if (!notification_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(notification_type_node, DESIRED_ATTRIBUTE);

        // Because of Alarm CC compatibility, we need to add the event node if the supported version is 3 or higher.
        if (supported_version >= 3) {
            auto event_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_get_group_attributes_t::event));
            if (!event_node.desired_exists()) {
                return SL_STATUS_NOT_READY;
            }
            frame_generator->add_value(event_node, DESIRED_ATTRIBUTE);
        }

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_notification::on_notification_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto notification_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_set_group_attributes_t::notification_type));
        if (!notification_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(notification_type_node, DESIRED_ATTRIBUTE);

        auto notification_status_node = group_node.emplace_node(static_cast<attribute_store_type_t>(notification_set_group_attributes_t::notification_status));
        if (!notification_status_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(notification_status_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_notification::on_event_supported_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto notification_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(event_supported_get_group_attributes_t::notification_type));
        if (!notification_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(notification_type_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
