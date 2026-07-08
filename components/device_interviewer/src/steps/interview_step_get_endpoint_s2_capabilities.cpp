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

#include "interview_step_get_endpoint_s2_capabilities.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "command_class_s2_events.hpp"
#include "command_class_s2_types.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    namespace
    {
        constexpr uint8_t S2_COMMANDS_SUPPORTED_MAX_TX_RETRIES = 5;

        void fire_endpoint_s2_commands_supported_get(InterviewSession &session, uint8_t endpoint_id, attribute_store::attribute endpoint_node)
        {
            component_connector connector;
            command_class_s2_types::s2_supported_get_payload_t payload_map;
            payload_map.endpoint_id   = endpoint_id;
            payload_map.device_node   = session.device_node;
            payload_map.endpoint_node = endpoint_node;
            payload_map.zwave_node_id = session.node_id;
            payload_map.granted_keys  = session.granted_keys;
            connector.fire_event(static_cast<uint32_t>(command_class_s2_events_t::COMMAND_CLASS_S2_COMMANDS_SUPPORTED_GET), payload_map);
        }
    }  // namespace

    bool GetEndpointS2CapabilitiesStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_REPORT || event_type == device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_GET_TX_FAILED;
    }

    StepResult GetEndpointS2CapabilitiesStep::on_enter(InterviewSession &session)
    {
        if (session.endpoints.endpoint_ids.empty()) {
            sl_log_info(LOG_TAG.data(), "No endpoints to query S2 capabilities for node %d", session.node_id);
            return skip();
        }

        if ((session.granted_keys & ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY) == 0 && (session.granted_keys & ZWAVE_CONTROLLER_S2_ACCESS_KEY) == 0 && (session.granted_keys & ZWAVE_CONTROLLER_S2_UNAUTHENTICATED_KEY) == 0) {
            sl_log_info(LOG_TAG.data(), "No S2 keys granted to node %d, skipping endpoint S2 capabilities", session.node_id);
            return skip();
        }

        session.s2_commands_supported_tx_retries = 0;
        return stay();
    }

    StepResult GetEndpointS2CapabilitiesStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            session.endpoints.current_endpoint_it    = session.endpoints.endpoint_ids.begin();
            session.s2_commands_supported_tx_retries = 0;

            auto endpoint_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);
            fire_endpoint_s2_commands_supported_get(session, *session.endpoints.current_endpoint_it, endpoint_node);
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_GET_TX_FAILED) {
            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end()) {
                return stay();
            }

            ++session.s2_commands_supported_tx_retries;
            if (session.s2_commands_supported_tx_retries >= S2_COMMANDS_SUPPORTED_MAX_TX_RETRIES) {
                sl_log_error(LOG_TAG.data(), "Node %d endpoint %d: S2 Commands Supported Get failed after %u TX attempts, failing interview", session.node_id, *session.endpoints.current_endpoint_it, session.s2_commands_supported_tx_retries);
                return fail();
            }

            sl_log_warning(LOG_TAG.data(), "Node %d endpoint %d: S2 Commands Supported Get TX failed, retry %u/%u", session.node_id, *session.endpoints.current_endpoint_it, session.s2_commands_supported_tx_retries, S2_COMMANDS_SUPPORTED_MAX_TX_RETRIES);

            auto endpoint_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);
            fire_endpoint_s2_commands_supported_get(session, *session.endpoints.current_endpoint_it, endpoint_node);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_s2_types::s2_supported_report_payload_t>(event->payload);

            uint8_t reported_endpoint_id = payload.connection_info.remote.endpoint_id;
            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end() || reported_endpoint_id != *session.endpoints.current_endpoint_it) {
                sl_log_warning(LOG_TAG.data(), "Unexpected S2 report for endpoint %d for node %d. Ignoring.", reported_endpoint_id, session.node_id);
                return stay();
            }

            session.endpoints.endpoint_discovered_command_classes.insert(session.endpoints.endpoint_discovered_command_classes.end(), payload.supported_cc_list.begin(), payload.supported_cc_list.end());
            session.s2_commands_supported_tx_retries = 0;

            ++session.endpoints.current_endpoint_it;

            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end()) {
                sl_log_info(LOG_TAG.data(), "All endpoint S2 capabilities received for node %d", session.node_id);
                return done();
            }

            auto endpoint_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);
            fire_endpoint_s2_commands_supported_get(session, *session.endpoints.current_endpoint_it, endpoint_node);

            return stay();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for S2_COMMANDS_SUPPORTED_REPORT");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
