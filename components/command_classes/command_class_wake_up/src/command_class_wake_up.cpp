
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
#include "command_class_wake_up.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "attribute_resolver.h"
#include "attribute_store_helper.h"
#include "attribute_timeouts.h"
#include "component_connector.hpp"
#include "command_class_wake_up_events.hpp"
#include "log.h"
#include <cstdint>

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_wake_up";

    command_class_wake_up::command_class_wake_up()
    {
        component_connector connector;
        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_capabilities_get_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_CAPABILITIES_GET_INTERVIEW, [](const command_class_wake_up_types::wake_up_capabilities_get_payload_t &p) {
            return zwave_command_class::command_class_wake_up::on_wake_up_capabilities_get_interview_requested(p);
        });

        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_interval_get_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_GET_INTERVIEW, [](const command_class_wake_up_types::wake_up_interval_get_payload_t &p) {
            return zwave_command_class::command_class_wake_up::on_wake_up_interval_get_interview_requested(p);
        });

        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_interval_set_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_SET, [](const command_class_wake_up_types::wake_up_interval_set_payload_t &p) {
            return zwave_command_class::command_class_wake_up::on_wake_up_interval_set_interview_requested(p);
        });

        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_interval_requested_payload_t, uint32_t>(
          command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_REQUESTED,
          [](const command_class_wake_up_types::wake_up_interval_requested_payload_t &request, uint32_t &result) -> sl_status_t { return zwave_command_class::command_class_wake_up::on_wake_up_interval_requested(request, result); });

        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_arm_no_more_information_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_ARM_NO_MORE_INFORMATION, [](const command_class_wake_up_types::wake_up_arm_no_more_information_payload_t &p) {
            return zwave_command_class::command_class_wake_up::on_arm_no_more_information_requested(p);
        });
    }
    sl_status_t command_class_wake_up::on_wake_up_capabilities_get_interview_requested(command_class_wake_up_types::wake_up_capabilities_get_payload_t payload)
    {
        auto group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_get_group_attributes_t::WAKE_UP_INTERVAL_CAPABILITIES_GET_GROUP));
        command_class_wake_up_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up::on_wake_up_interval_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_wake_up_attribute_map_t payload)
    {
        command_class_wake_up_types::wake_up_capabilities_report_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_CAPABILITIES_REPORT_RECEIVED), callback_payload);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up::on_wake_up_interval_get_interview_requested(command_class_wake_up_types::wake_up_interval_get_payload_t payload)
    {
        auto group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_get_group_attributes_t::WAKE_UP_INTERVAL_GET_GROUP));
        command_class_wake_up_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up::on_wake_up_interval_set_interview_requested(command_class_wake_up_types::wake_up_interval_set_payload_t payload)
    {
        auto group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::WAKE_UP_INTERVAL_SET_GROUP));

        auto seconds_node = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::seconds));
        seconds_node.set_desired<wake_up_interval_set_seconds_t>(static_cast<wake_up_interval_set_seconds_t>(payload.interval));

        auto nodeid_node = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::nodeid));
        nodeid_node.set_desired<wake_up_interval_set_nodeid_t>(static_cast<wake_up_interval_set_nodeid_t>(payload.node_id));

        attribute_resolver_set_resolution_listener(group_node, command_class_wake_up::on_wake_up_interval_set_interview_resolution);
        command_class_wake_up_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up::on_wake_up_interval_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_wake_up_attribute_map_t payload)
    {
        command_class_wake_up_types::wake_up_interval_report_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;
        callback_payload.seconds              = get_value_or_default<wake_up_interval_report_seconds_t>(payload, "seconds", 0);

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_REPORT_RECEIVED), callback_payload);

        return SL_STATUS_OK;
    }

    void command_class_wake_up::arm_no_more_information_on_resolution_idle(attribute_store_node_t node_id_node)
    {
        // Cancel any prior idle timer so a fresh wake window cannot inherit a
        // pending WUNMI from the previous arm.
        attribute_timeout_cancel_callback(node_id_node, on_wake_up_no_more_information_deferred);
        attribute_resolver_set_resolution_listener(node_id_node, on_wake_up_no_more_information_resolution_listener);
        sl_log_debug(LOG_TAG.data(), "Armed no-more-info listener for node Attribute ID %d", node_id_node);
    }

    sl_status_t command_class_wake_up::on_arm_no_more_information_requested(const command_class_wake_up_types::wake_up_arm_no_more_information_payload_t &payload)
    {
        arm_no_more_information_on_resolution_idle(payload.device_node_id_node);
        return SL_STATUS_OK;
    }

    void command_class_wake_up::on_wake_up_no_more_information_resolution_listener(attribute_store_node_t node_id_node)
    {
        attribute_resolver_clear_resolution_listener(node_id_node, on_wake_up_no_more_information_resolution_listener);
        attribute_timeout_set_callback(node_id_node, 100, on_wake_up_no_more_information_deferred);
    }

    void command_class_wake_up::on_wake_up_no_more_information_deferred(attribute_store_node_t node_id_node)
    {
        attribute_resolver_clear_resolution_listener(node_id_node, on_wake_up_no_more_information_resolution_listener);

        // Exhausted Gets count as done for this wake window (retried on next WUN).
        if (attribute_resolver_node_or_child_needs_resolution(node_id_node)) {
            attribute_resolver_set_resolution_listener(node_id_node, on_wake_up_no_more_information_resolution_listener);
            return;
        }

        send_wake_up_no_more_information(node_id_node);
    }

    void command_class_wake_up::send_wake_up_no_more_information(attribute_store_node_t node_id_node)
    {
        sl_log_debug(LOG_TAG.data(), "Sending Wake Up No More Information for node %d", node_id_node);
        zwave_endpoint_id_t ep0           = 0;
        attribute_store_node_t endpoint_0 = attribute_store_get_node_child_by_value(node_id_node, ATTRIBUTE_ENDPOINT_ID, REPORTED_ATTRIBUTE, &ep0, sizeof(ep0), 0);
        auto endpoint_node                = attribute_store::attribute(endpoint_0);
        auto group_node                   = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_no_more_information_group_attributes_t::WAKE_UP_NO_MORE_INFORMATION_GROUP));
        attribute_resolver_set_resolution_listener(group_node, on_wake_up_no_more_information_sent_listener);
        // Skip supervision: the node goes back to sleep immediately after receiving
        // No More Information and cannot complete the supervision handshake.
        command_class_wake_up_core::start_group_resolution(group_node, {.skip_supervision = true});
    }

    void command_class_wake_up::on_wake_up_no_more_information_sent_listener(attribute_store_node_t wunmi_group_node)
    {
        attribute_resolver_clear_resolution_listener(wunmi_group_node, on_wake_up_no_more_information_sent_listener);

        attribute_store_node_t endpoint_node = attribute_store_get_first_parent_with_type(wunmi_group_node, ATTRIBUTE_ENDPOINT_ID);
        command_class_wake_up_types::wake_up_no_more_information_sent_payload_t callback_payload;
        callback_payload.device_endpoint_node = attribute_store::attribute(endpoint_node);

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_NO_MORE_INFORMATION_SENT), callback_payload);
    }

    sl_status_t command_class_wake_up::on_wake_up_notification_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_wake_up_attribute_map_t payload)
    {
        command_class_wake_up_types::wake_up_notification_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;

        component_connector connector;
        // fire_event is async: network_monitor resumes resolution and arms WUNMI
        // after restarting exhausted Gets. Arming here would race while still paused.
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_NOTIFICATION_RECEIVED), callback_payload);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up::on_wake_up_interval_requested(const command_class_wake_up_types::wake_up_interval_requested_payload_t &request, wake_up_interval_report_seconds_t &result)
    {
        attribute_store_node_t node_id_node   = request.device_endpoint_node;
        zwave_endpoint_id_t ep0               = 0;
        attribute_store_node_t endpoint_0     = attribute_store_get_node_child_by_value(node_id_node, ATTRIBUTE_ENDPOINT_ID, REPORTED_ATTRIBUTE, &ep0, sizeof(ep0), 0);
        attribute_store_node_t interval_group = attribute_store_get_node_child_by_type(endpoint_0, static_cast<attribute_store_type_t>(wake_up_interval_report_group_attributes_t::WAKE_UP_INTERVAL_REPORT_GROUP), 0);
        attribute_store_node_t seconds_node   = attribute_store_get_first_child_by_type(interval_group, static_cast<attribute_store_type_t>(wake_up_interval_report_group_attributes_t::seconds));

        result = 0;
        attribute_store_get_reported(seconds_node, &result, sizeof(result));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up::on_wake_up_interval_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        sl_log_debug(LOG_TAG.data(), "WakeUpIntervalSet command received");
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto seconds_node = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::seconds));
        auto nodeid_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::nodeid));
        if (!seconds_node.desired_exists() || !nodeid_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        auto seconds_value = seconds_node.desired<wake_up_interval_set_seconds_t>();
        auto nodeid_value  = nodeid_node.desired<wake_up_interval_set_nodeid_t>();

        // Seconds is uint32_t right now so convert it to an uint8_t array of 3 bytes because only 3 bytes are used for the wake up interval.
        uint8_t seconds_array[3];
        seconds_array[0] = (seconds_value >> 16) & 0xFF;
        seconds_array[1] = (seconds_value >> 8) & 0xFF;
        seconds_array[2] = (seconds_value) & 0xFF;

        frame_generator->add_raw_byte(seconds_array[0]);
        frame_generator->add_raw_byte(seconds_array[1]);
        frame_generator->add_raw_byte(seconds_array[2]);
        frame_generator->add_raw_byte(nodeid_value);

        return frame_generator->generate_frame();
    }

    void command_class_wake_up::on_wake_up_interval_set_interview_resolution(attribute_store_node_t set_group_node)
    {
        attribute_resolver_clear_resolution_listener(set_group_node, on_wake_up_interval_set_interview_resolution);

        auto endpoint_node = attribute_store::attribute(attribute_store_get_first_parent_with_type(set_group_node, ATTRIBUTE_ENDPOINT_ID));
        command_class_wake_up_types::wake_up_interval_set_interview_resolution_payload_t resolution_payload;
        resolution_payload.device_endpoint_node = endpoint_node;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_SET_INTERVIEW_RESOLUTION_COMPLETED), resolution_payload);

        command_class_wake_up_types::wake_up_interval_get_payload_t get_payload;
        get_payload.device_endpoint_node = endpoint_node;
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_GET_INTERVIEW), get_payload);
    }

    void command_class_wake_up::on_wake_up_interval_set_user_resolution(attribute_store_node_t set_group_node)
    {
        attribute_resolver_clear_resolution_listener(set_group_node, on_wake_up_interval_set_user_resolution);

        auto endpoint_node = attribute_store::attribute(attribute_store_get_first_parent_with_type(set_group_node, ATTRIBUTE_ENDPOINT_ID));

        component_connector connector;
        command_class_wake_up_types::wake_up_interval_get_payload_t get_payload;
        get_payload.device_endpoint_node = endpoint_node;
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_GET_INTERVIEW), get_payload);
    }

}  // namespace zwave_command_class
