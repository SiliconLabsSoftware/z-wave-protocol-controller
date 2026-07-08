
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
#include <cstdint>
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_multi_channel.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "command_class_multi_channel_events.hpp"
#include "command_class_s2_events.hpp"
#include "command_class_s2_types.hpp"
#include "command_class_multi_channel_types.hpp"
#include "zpc_attribute_store_network_helper.h"
#include "command_class_multi_channel_constants.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_multi_channel";

    command_class_multi_channel::command_class_multi_channel()
    {
        // Constructor body - can be empty or contain initialization logic
        component_connector connector;
        connector.connect_typed<command_class_multi_channel_events_t, command_class_multi_channel_types::command_class_multi_channel_end_point_find_payload_t>(
          command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_FIND,
          [](const command_class_multi_channel_types::command_class_multi_channel_end_point_find_payload_t &p) { return zwave_command_class::command_class_multi_channel::on_multi_channel_end_point_find_requested(p); });

        connector.connect_typed<command_class_multi_channel_events_t, command_class_multi_channel_types::command_class_multi_channel_commands_capability_get_payload_t>(
          command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_GET,
          [](const command_class_multi_channel_types::command_class_multi_channel_commands_capability_get_payload_t &p) { return zwave_command_class::command_class_multi_channel::on_multi_channel_commands_capability_get_requested(p); });

        connector.connect_typed<command_class_multi_channel_events_t, command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t, command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t>(
          command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_GET_LIST_OF_ENDPOINTS,
          [](const command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t &p, command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t &r) {
              return zwave_command_class::command_class_multi_channel::on_multi_channel_get_list_of_endpoints_requested(p, r);
          });

        connector.connect_typed<command_class_multi_channel_events_t, command_class_multi_channel_types::command_class_multi_channel_end_point_get_payload_t>(
          command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_GET_INTERVIEW,
          [](const command_class_multi_channel_types::command_class_multi_channel_end_point_get_payload_t &p) { return zwave_command_class::command_class_multi_channel::on_multi_channel_end_point_get_interview_requested(p); });
    }

    void command_class_multi_channel::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) {}

    sl_status_t command_class_multi_channel::on_multi_channel_get_list_of_endpoints_requested(const command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t &payload,
                                                                                              command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t &result)
    {
        auto device_node                            = payload.device_endpoint_node.parent();
        auto endpoint_node_0                        = device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID);
        auto multi_channel_endpoint_find_group_node = endpoint_node_0.emplace_node(static_cast<attribute_store_type_t>(command_class_multi_channel_types::multi_channel_end_point_find_group_attributes_t::MULTI_CHANNEL_END_POINT_FIND_GROUP));

        auto generic_device_class_node = multi_channel_endpoint_find_group_node.emplace_node(static_cast<attribute_store_type_t>(command_class_multi_channel_types::multi_channel_end_point_find_group_attributes_t::generic_device_class));
        generic_device_class_node.set_desired<uint8_t>(payload.endpoints[0].properties1.value);
        auto specific_device_class_node = multi_channel_endpoint_find_group_node.emplace_node(static_cast<attribute_store_type_t>(command_class_multi_channel_types::multi_channel_end_point_find_group_attributes_t::specific_device_class));
        specific_device_class_node.set_desired<uint8_t>(payload.endpoints[0].properties1.value);
        command_class_multi_channel_core::start_group_resolution(multi_channel_endpoint_find_group_node);

        // Copy endpoints to result
        result.device_endpoint_node = payload.device_endpoint_node;
        result.endpoints            = payload.endpoints;

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_end_point_get_interview_requested(command_class_multi_channel_types::command_class_multi_channel_end_point_get_payload_t payload)
    {
        auto multi_channel_endpoint_get_group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_get_group_attributes_t::MULTI_CHANNEL_END_POINT_GET_GROUP));
        command_class_multi_channel_core::start_group_resolution(multi_channel_endpoint_get_group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_commands_capability_get_requested(command_class_multi_channel_types::command_class_multi_channel_commands_capability_get_payload_t payload)
    {
        auto endpoint_node                                = payload.device_endpoint_node.parent().emplace_node(ATTRIBUTE_ENDPOINT_ID, payload.endpoint_id);
        auto multi_channel_commands_capability_group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_get_group_attributes_t::MULTI_CHANNEL_CAPABILITY_GET_GROUP));
        auto end_point_node                               = multi_channel_commands_capability_group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_get_group_attributes_t::end_point));
        end_point_node.set_desired<uint8_t>(payload.endpoint_id);
        command_class_multi_channel_core::start_group_resolution(multi_channel_commands_capability_group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_end_point_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_attribute_map_t payload)
    {
        command_class_multi_channel_types::command_class_multi_channel_end_point_report_payload_t payload_struct;
        payload_struct.device_endpoint_node = endpoint;

        uint8_t individual_end_points = 0;
        uint8_t aggregated_end_points = 0;
        uint8_t dynamic               = 0;
        uint8_t identical             = 0;

        payload_struct.individual_end_points = get_value_or_default(payload, "individual_end_points", individual_end_points);
        payload_struct.aggregated_end_points = get_value_or_default(payload, "aggregated_end_points", aggregated_end_points);
        payload_struct.dynamic               = get_value_or_default(payload, "dynamic", dynamic);
        payload_struct.identical             = get_value_or_default(payload, "identical", identical);

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_REPORT_RECEIVED), payload_struct);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_capability_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_attribute_map_t payload)
    {
        command_class_multi_channel_types::command_class_multi_channel_commands_capability_report_payload_t payload_struct;
        payload_struct.device_endpoint_node = endpoint;
        uint8_t endpoint_id                 = 0;
        payload_struct.endpoint_id          = get_value_or_default(payload, "end_point", endpoint_id);

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT), payload_struct);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_end_point_find_requested(command_class_multi_channel_types::command_class_multi_channel_end_point_find_payload_t payload)
    {
        auto multi_channel_end_point_find_group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::MULTI_CHANNEL_END_POINT_FIND_GROUP));
        auto generic_device_class_node               = multi_channel_end_point_find_group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::generic_device_class));
        generic_device_class_node.set_desired<uint8_t>(command_class_multi_channel_constants::DEVICE_CLASS_GENERIC_TYPE_NON_INTEROPERABLE);

        auto specific_device_class_node = multi_channel_end_point_find_group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::specific_device_class));
        specific_device_class_node.set_desired<uint8_t>(command_class_multi_channel_constants::DEVICE_CLASS_GENERIC_TYPE_NON_INTEROPERABLE);
        command_class_multi_channel_core::start_group_resolution(multi_channel_end_point_find_group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_end_point_find_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_attribute_map_t payload)
    {
        // We received the list of endpoints, starting to poll capabilities for the first one
        command_class_multi_channel_types::command_class_multi_channel_end_point_find_report_payload_t payload_struct;
        payload_struct.device_endpoint_node = endpoint;

        multi_channel_end_point_find_report_vg_t vg;
        vg                       = get_value_or_default(payload, "vg", vg);
        payload_struct.endpoints = vg;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_FIND_REPORT), payload_struct);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_capability_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        sl_status_t status    = SL_STATUS_NOT_READY;
        auto group_node       = args.node;
        auto *frame_generator = args.get_frame_generator;

        auto end_point_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_get_group_attributes_t::end_point));

        if (end_point_node.desired_exists()) {
            frame_generator->add_value(end_point_node, DESIRED_ATTRIBUTE);
            status = frame_generator->generate_frame();
        }

        return status;
    }

    sl_status_t command_class_multi_channel::on_multi_channel_end_point_find_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto *frame_generator = args.get_frame_generator;
        auto group_node       = args.node;

        auto generic_device_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::generic_device_class));
        if (!generic_device_class_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(generic_device_class_node, DESIRED_ATTRIBUTE);

        auto specific_device_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::specific_device_class));
        if (!specific_device_class_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(specific_device_class_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_multi_channel::on_multi_channel_aggregated_members_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto *frame_generator = args.get_frame_generator;
        auto group_node       = args.node;

        auto aggregated_end_point_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_aggregated_members_get_group_attributes_t::aggregated_end_point));
        if (!aggregated_end_point_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(aggregated_end_point_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class