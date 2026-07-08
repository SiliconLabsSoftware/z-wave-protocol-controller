
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
    }

    void command_class_basic::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        command_class_version_types::command_class_version_cc_get_payload_t payload_map_version;
        payload_map_version.device_endpoint_node   = endpoint_node;
        payload_map_version.command_class          = COMMAND_CLASS_BASIC;
        payload_map_version.is_first_command_class = false;
        // Use 2 to absorb the race between the resolver's tx-complete and report-dispatch
        // paths: with retry_count == 1 a valid version=0 report (CC unsupported) is mistaken
        // for retry exhaustion before stop_group_resolution clears needs_get. The second
        // attempt is a cheap no-op once the leaf is populated.
        payload_map_version.retry_count = 2;

        // Ask the version command class to get the version of the basic command class to be able to parse the report.
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CC_GET), payload_map_version);

        // Ask the basic command class to get the current value of the basic command class.
        // Note: Events will be queued and processed in the order they are received.
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