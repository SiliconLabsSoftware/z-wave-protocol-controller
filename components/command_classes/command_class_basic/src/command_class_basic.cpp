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

// Base class
#include "command_class_basic.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "attribute_callbacks.hpp"
#include "attribute_resolver.h"
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store_network_helper.h"

// Component Connector
#include "component_connector.hpp"

// Basic command class types and events
#include "command_class_basic_types.hpp"
#include "command_class_basic_events.hpp"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_basic";

    command_class_basic::command_class_basic()
    {
        // Basic Command Class is exception for the interview process, because of the
        // CC:0020.01.00.21.003 and CC:0020.01.00.21.004.
        // The basic command class must not be advertised in the NIF and Security Supported Reports.
        force_interview_for_cc = true;

        component_connector connector;
        connector.connect_typed<command_class_basic_events_t, command_class_basic_types::basic_get_interview_payload_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET_INTERVIEW, [](const command_class_basic_types::basic_get_interview_payload_t &payload) {
            return zwave_command_class::command_class_basic::on_command_class_basic_get_interview_requested(payload);
        });

        // Promote Version Report 0 → 1 when a Basic Report already proved support
        // (CL:0020.01.21.02.2). Basic is never advertised, so Version often returns 0.
        attribute_store::register_callback_by_type_and_state(&on_basic_version_reported, ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC), REPORTED_ATTRIBUTE);
        attribute_resolver_set_resolution_give_up_listener(static_cast<attribute_store_type_t>(basic_get_group_attributes_t::BASIC_GET_GROUP), &command_class_basic::on_basic_get_resolution_give_up);
    }

    bool command_class_basic::has_basic_report(const attribute_store::attribute &endpoint_node)
    {
        const auto endpoint_has_report = [](const attribute_store::attribute &ep) {
            auto report_group = ep.child_by_type(static_cast<attribute_store_type_t>(basic_report_group_attributes_t::BASIC_REPORT_GROUP));
            if (!report_group.is_valid()) {
                return false;
            }
            auto current_value = report_group.child_by_type(static_cast<attribute_store_type_t>(basic_report_group_attributes_t::current_value));
            return current_value.is_valid() && current_value.reported_exists();
        };

        if (endpoint_has_report(endpoint_node)) {
            return true;
        }

        // Version CC reports are stored on the root; Basic Report may live on any
        // endpoint that answered the probe.
        auto device_node = endpoint_node.parent();
        if (!device_node.is_valid()) {
            return false;
        }
        const auto endpoints = device_node.children(ATTRIBUTE_ENDPOINT_ID);
        return std::ranges::any_of(endpoints, endpoint_has_report);
    }

    void command_class_basic::on_basic_version_reported(attribute_store_node_t version_node_id, attribute_store_change_t change)
    {
        if (change != ATTRIBUTE_UPDATED) {
            return;
        }

        attribute_store::attribute version_node(version_node_id);
        if (!version_node.reported_exists()) {
            return;
        }

        const uint8_t basic_version = version_node.reported<uint8_t>();
        if (basic_version != 0) {
            return;
        }

        auto endpoint_node = attribute_store::attribute(attribute_store_get_first_parent_with_type(version_node_id, ATTRIBUTE_ENDPOINT_ID));
        if (!endpoint_node.is_valid() || !has_basic_report(endpoint_node)) {
            sl_log_debug(LOG_TAG.data(), "Basic CC version is 0 and no Basic Report; leaving unsupported");
            return;
        }

        // Devices that answer Basic Get often return Version 0 for 0x20 because
        // Basic MUST NOT be advertised. A report is the support signal; store v1.
        sl_log_debug(LOG_TAG.data(), "Basic Report present with Version 0; promoting to version 1");
        version_node.set_reported<uint8_t>(1);
    }

    void command_class_basic::on_basic_get_resolution_give_up(attribute_store_node_t group_node_id)
    {
        auto endpoint_node = attribute_store::attribute(group_node_id).parent();
        if (!endpoint_node.is_valid()) {
            return;
        }

        sl_log_debug(LOG_TAG.data(), "Basic Get exhausted; Basic CC is unsupported");
        command_class_basic_types::basic_get_resolution_give_up_payload_t payload;
        payload.device_endpoint_node = endpoint_node;
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET_RESOLUTION_GIVE_UP), payload);
    }

    sl_status_t command_class_basic::on_command_class_basic_get_interview_requested(command_class_basic_types::basic_get_interview_payload_t payload)
    {
        auto basic_get_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(basic_get_group_attributes_t::BASIC_GET_GROUP));
        // Use 2 to absorb the race between the resolver's tx-complete and report-dispatch
        // paths: with retry_count == 1 a valid Basic Report is mistaken for retry exhaustion
        // before stop_group_resolution clears needs_get. The second attempt is a cheap no-op
        // once the group is populated.
        command_class_basic_core::start_group_resolution(basic_get_node, {.retry_count = 2});
        return SL_STATUS_OK;
    }

    sl_status_t command_class_basic::on_basic_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_basic_attribute_map_t payload)
    {
        (void)connection_info;
        (void)payload;

        command_class_basic_types::basic_report_received_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_REPORT_RECEIVED), callback_payload);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_basic::on_basic_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(basic_set_group_attributes_t::value));
        if (!value_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(value_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
