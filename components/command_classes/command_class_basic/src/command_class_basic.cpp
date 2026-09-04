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
#include "command_class_basic.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "attribute_callbacks.hpp"
#include "attribute_store_defined_attribute_types.h"

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
        // Constructor body - can be empty or contain initialization logic

        // Basic Command Class is exception for the interview process, because of the
        // CC:0020.01.00.21.003 and CC:0020.01.00.21.004.
        // The basic command class must not be advertised in the NIF and Security Supported Reports.
        force_interview_for_cc = true;

        component_connector connector;
        connector.connect_typed<command_class_basic_events_t, attribute_store::attribute>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET, [](const attribute_store::attribute &endpoint_node) {
            zwave_command_class::command_class_basic::on_command_class_basic_get_event(endpoint_node);
            return SL_STATUS_OK;
        });

        // Basic is not in the NIF; interview probes Version for 0x20 first. When that
        // version leaf is stored, start Basic Get only if the CC is actually supported.
        attribute_store::register_callback_by_type_and_state(&on_basic_version_reported, ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC), REPORTED_ATTRIBUTE);
    }

    void command_class_basic::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)supported_version;

        command_class_version_types::command_class_version_cc_get_payload_t payload_map_version;
        payload_map_version.device_endpoint_node   = endpoint_node;
        payload_map_version.command_class          = COMMAND_CLASS_BASIC;
        payload_map_version.is_first_command_class = false;
        // Use 2 to absorb the race between the resolver's tx-complete and report-dispatch
        // paths: with retry_count == 1 a valid version=0 report (CC unsupported) is mistaken
        // for retry exhaustion before stop_group_resolution clears needs_get. The second
        // attempt is a cheap no-op once the leaf is populated.
        payload_map_version.retry_count = 2;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CC_GET), payload_map_version);

        // Re-interview: the Basic version leaf is often already populated with the same
        // value. attribute_store then treats set_reported as touch-only (no ATTRIBUTE_UPDATED),
        // so on_basic_version_reported would never queue Basic Get. If we already know Basic
        // is supported, start Basic Get here; first interview still relies on the version
        // callback once the leaf is written for the first time.
        auto version_node = endpoint_node.child_by_type(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
        if (version_node.is_valid() && version_node.reported_exists() && version_node.reported<uint8_t>() != 0) {
            connector.fire_event(static_cast<uint32_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET), endpoint_node);
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
        if (basic_version == 0) {
            sl_log_debug(LOG_TAG.data(), "Basic CC version is 0; skipping Basic Get");
            return;
        }

        auto endpoint_node = attribute_store::attribute(attribute_store_get_first_parent_with_type(version_node_id, ATTRIBUTE_ENDPOINT_ID));
        if (!endpoint_node.is_valid()) {
            return;
        }

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET), endpoint_node);
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
        // Access parsed data
        basic_report_current_value_t current_value = 0;
        current_value                              = get_value_or_default(payload, "current_value", current_value);

        // Add custom logic here (e.g., logging, validation, notifications)
        sl_log_debug(LOG_TAG.data(), "Basic current_value received: %d", current_value);

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
