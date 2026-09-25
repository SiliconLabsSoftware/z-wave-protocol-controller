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
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store_network_helper.h"

// Component Connector
#include "component_connector.hpp"

// Version command class types and events
#include "command_class_version_types.hpp"
#include "command_class_version_events.hpp"

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
        connector.connect_typed<command_class_basic_events_t, attribute_store::attribute>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET, [](const attribute_store::attribute &endpoint_node) {
            zwave_command_class::command_class_basic::on_command_class_basic_get_event(endpoint_node);
            return SL_STATUS_OK;
        });

        // Promote Version Report 0 → 1 when a Basic Report already proved support
        // (CL:0020.01.21.02.2). Basic is never advertised, so Version often returns 0.
        attribute_store::register_callback_by_type_and_state(&on_basic_version_reported, ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC), REPORTED_ATTRIBUTE);
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

    attribute_store::attribute command_class_basic::basic_version_node(attribute_store::attribute endpoint_node)
    {
        // Version CC reports land on the root; keep the Basic version leaf there so
        // validate_command_version's ep0 fallback sees it.
        auto device_node = endpoint_node.parent();
        if (device_node.is_valid()) {
            auto ep0 = attribute_store::attribute(attribute_store_get_endpoint_0_node(device_node));
            if (ep0.is_valid()) {
                return ep0.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
            }
        }
        return endpoint_node.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
    }

    void command_class_basic::request_basic_version(attribute_store::attribute endpoint_node)
    {
        auto version_node = basic_version_node(endpoint_node);
        // CL:0020.01.21.02.2: a Basic Report means the CC is supported. Record v1
        // immediately so Set is not blocked while Version Get is in flight or fails.
        // Version Get below may refine this to v2; a Version Report of 0 is promoted
        // back to 1 in on_basic_version_reported.
        if (!version_node.reported_exists() || version_node.reported<uint8_t>() == 0) {
            version_node.set_reported<uint8_t>(1);
            sl_log_debug(LOG_TAG.data(), "Basic Report received; establishing Basic CC version 1");
        }

        command_class_version_types::command_class_version_cc_get_payload_t payload_map_version;
        payload_map_version.device_endpoint_node   = endpoint_node;
        payload_map_version.command_class          = COMMAND_CLASS_BASIC;
        payload_map_version.is_first_command_class = false;
        // Absorb the resolver race between tx-complete and report-dispatch when
        // the node returns version 0 (CC:0086.01.14.11.002).
        payload_map_version.retry_count = 2;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CC_GET), payload_map_version);
    }

    void command_class_basic::request_basic_version_if_needed(attribute_store::attribute endpoint_node)
    {
        auto version_node = basic_version_node(endpoint_node);
        if (version_node.reported_exists() && version_node.reported<uint8_t>() != 0) {
            return;
        }

        request_basic_version(endpoint_node);
    }

    void command_class_basic::ensure_support_if_report_present(attribute_store::attribute endpoint_node)
    {
        if (!has_basic_report(endpoint_node)) {
            return;
        }
        auto version_node = basic_version_node(endpoint_node);
        if (!version_node.reported_exists() || version_node.reported<uint8_t>() == 0) {
            sl_log_debug(LOG_TAG.data(), "Basic Report present with version 0; establishing version 1");
            version_node.set_reported<uint8_t>(1);
        }
    }

    void command_class_basic::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)supported_version;

        // Re-interview heal: a prior probe may have stored a report without a version.
        ensure_support_if_report_present(endpoint_node);

        // CL:0020.01.21.01.1: always probe with Basic Get first. Support is
        // established only if a Basic Report is returned (CL:0020.01.21.02.2).
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET), endpoint_node);

        // Basic is stripped from the Version CC list (0x20). First discovery sends
        // Version Get after the first report. Re-interview must send it again.
        if (has_basic_report(endpoint_node)) {
            request_basic_version(endpoint_node);
        }
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
            if (endpoint_node.is_valid()) {
                set_cc_interview_state(endpoint_node, COMMAND_CLASS_BASIC, cc_interview_state::done);
            }
            return;
        }

        // Devices that answer Basic Get often return Version 0 for 0x20 because
        // Basic MUST NOT be advertised. A report is the support signal; store v1.
        sl_log_debug(LOG_TAG.data(), "Basic Report present with Version 0; promoting to version 1");
        version_node.set_reported<uint8_t>(1);
    }

    void command_class_basic::on_command_class_basic_get_event(attribute_store::attribute endpoint_node)
    {
        // Query basic value during interview
        auto basic_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(basic_get_group_attributes_t::BASIC_GET_GROUP));
        // Use 2 to absorb the race between the resolver's tx-complete and report-dispatch
        // paths: with retry_count == 1 a valid Basic Report is mistaken for retry exhaustion
        // before stop_group_resolution clears needs_get. The second attempt is a cheap no-op
        // once the group is populated.
        start_group_resolution(basic_get_node, {.retry_count = 2});
    }

    sl_status_t command_class_basic::on_basic_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_basic_attribute_map_t payload)
    {
        (void)connection_info;

        basic_report_current_value_t current_value = 0;
        current_value                              = get_value_or_default(payload, "current_value", current_value);
        sl_log_debug(LOG_TAG.data(), "Basic current_value received: %d", current_value);
        set_cc_interview_state(endpoint, cc_properties.command_class_id, cc_interview_state::done);

        // Report is already stored. First discovery asks Version for 0x20 here.
        // Re-interview asks from on_interview because version is already non-zero.
        request_basic_version_if_needed(endpoint);

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
