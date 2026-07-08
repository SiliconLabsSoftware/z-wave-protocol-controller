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

#include "interview_step_get_endpoint_s0_capabilities.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "command_class_s0_events.hpp"
#include "command_class_s0_types.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool GetEndpointS0CapabilitiesStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::S0_COMMANDS_SUPPORTED_REPORT;
    }

    StepResult GetEndpointS0CapabilitiesStep::on_enter(InterviewSession &session)
    {
        if (session.endpoints.endpoint_ids.empty()) {
            sl_log_info(LOG_TAG.data(), "No endpoints to query S0 capabilities for node %d", session.node_id);
            return skip();
        }

        if ((session.granted_keys & ZWAVE_CONTROLLER_S0_KEY) == 0) {
            sl_log_info(LOG_TAG.data(), "Node %d was not granted the S0 key, skipping S0 per-endpoint", session.node_id);
            return skip();
        }

        // S2 is higher than S0; endpoint secure CCs are discovered via S2 Commands Supported.
        if (((session.granted_keys & ZWAVE_CONTROLLER_S2_UNAUTHENTICATED_KEY) != 0) || ((session.granted_keys & ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY) != 0) || ((session.granted_keys & ZWAVE_CONTROLLER_S2_ACCESS_KEY) != 0)) {
            sl_log_info(LOG_TAG.data(), "Node %d was granted an S2 key, skipping S0 per-endpoint", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult GetEndpointS0CapabilitiesStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            session.endpoints.current_endpoint_it = session.endpoints.endpoint_ids.begin();

            auto endpoint_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);

            component_connector connector;
            command_class_s0_types::s0_supported_get_payload_t payload;
            payload.endpoint_id   = *session.endpoints.current_endpoint_it;
            payload.device_node   = session.device_node;
            payload.endpoint_node = endpoint_node;
            payload.zwave_node_id = session.node_id;
            payload.granted_keys  = session.granted_keys;
            connector.fire_event(static_cast<uint32_t>(command_class_s0_events_t::COMMAND_CLASS_S0_COMMANDS_SUPPORTED_GET), payload);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_s0_types::s0_supported_report_payload_t>(event->payload);

            uint8_t reported_endpoint_id = payload.connection_info.remote.endpoint_id;
            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end() || reported_endpoint_id != *session.endpoints.current_endpoint_it) {
                sl_log_warning(LOG_TAG.data(), "Unexpected S0 report for endpoint %d for node %d. Ignoring.", reported_endpoint_id, session.node_id);
                return stay();
            }

            session.endpoints.endpoint_discovered_command_classes.insert(session.endpoints.endpoint_discovered_command_classes.end(), payload.supported_cc_list.begin(), payload.supported_cc_list.end());

            ++session.endpoints.current_endpoint_it;

            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end()) {
                sl_log_info(LOG_TAG.data(), "All endpoint S0 capabilities received for node %d", session.node_id);
                return done();
            }

            auto endpoint_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);

            component_connector connector;
            command_class_s0_types::s0_supported_get_payload_t next_payload;
            next_payload.endpoint_id   = *session.endpoints.current_endpoint_it;
            next_payload.device_node   = session.device_node;
            next_payload.endpoint_node = endpoint_node;
            next_payload.zwave_node_id = session.node_id;
            next_payload.granted_keys  = session.granted_keys;

            connector.fire_event(static_cast<uint32_t>(command_class_s0_events_t::COMMAND_CLASS_S0_COMMANDS_SUPPORTED_GET), next_payload);

            return stay();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for S0_COMMANDS_SUPPORTED_REPORT");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
