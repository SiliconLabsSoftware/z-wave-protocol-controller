
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

#include "interview_state_machine.hpp"
#include "interview_steps.hpp"
#include "interview_step.hpp"
#include "device_interviewer.hpp"  // For device_interviewer_external_event_data definition
#include "log.h"
#include "zpc_attribute_store_network_helper.h"
#include "attribute_store.h"
#include "component_connector_types.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "command_class_s2_types.hpp"
#include "command_class_s0_types.hpp"
#include "zwave_utils.h"
#include "clock_platform.h"
#include "zpc_config.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_state_machine";

    namespace
    {
        constexpr clock_time_t INTERVIEW_STALL_TIMEOUT_MS = 60 * CLOCK_SECOND;
        /// Fallback when config is unavailable (matches zpc_config.c DEFAULT_WAKE_UP_INTERVAL).
        constexpr uint32_t INTERVIEW_STALL_NL_WAKE_UP_FALLBACK_S = 4200;
        /// Allow roughly two wake cycles before declaring an NL interview stalled.
        constexpr uint32_t INTERVIEW_STALL_NL_WAKE_UP_MULTIPLIER = 2;
        /// Floor so a tiny `default_wake_up_interval` (e.g. 5 or 30) cannot abort NL in seconds.
        constexpr uint32_t INTERVIEW_STALL_NL_MIN_S = 15 * 60;

        clock_time_t stall_timeout_ms_for_node(zwave_node_id_t node_id)
        {
            if (zwave_get_operating_mode(node_id) != OPERATING_MODE_NL) {
                return INTERVIEW_STALL_TIMEOUT_MS;
            }

            const zpc_config_t *cfg   = zpc_get_config();
            const uint32_t interval_s = (cfg != nullptr && cfg->default_wake_up_interval > 0) ? static_cast<uint32_t>(cfg->default_wake_up_interval) : INTERVIEW_STALL_NL_WAKE_UP_FALLBACK_S;
            const uint32_t from_wake  = INTERVIEW_STALL_NL_WAKE_UP_MULTIPLIER * interval_s;
            const uint32_t stall_s    = (from_wake > INTERVIEW_STALL_NL_MIN_S) ? from_wake : INTERVIEW_STALL_NL_MIN_S;
            return static_cast<clock_time_t>(stall_s) * CLOCK_SECOND;
        }
    }  // namespace

    void InterviewStateMachine::register_transitions()
    {
        using state_machine::StepResultCode;

        // Phase 1: Node discovery and security capabilities
        set_transition(InterviewState::NODE_INFORMATION, StepResultCode::DONE, InterviewState::S0_COMMANDS_SUPPORTED);
        set_transition(InterviewState::S0_COMMANDS_SUPPORTED, StepResultCode::DONE, InterviewState::S2_COMMANDS_SUPPORTED);
        set_transition(InterviewState::S0_COMMANDS_SUPPORTED, StepResultCode::SKIP, InterviewState::S2_COMMANDS_SUPPORTED);
        set_transition(InterviewState::S2_COMMANDS_SUPPORTED, StepResultCode::DONE, InterviewState::GET_VERSION_INFO);
        set_transition(InterviewState::S2_COMMANDS_SUPPORTED, StepResultCode::SKIP, InterviewState::GET_VERSION_INFO);

        // Phase 2: Version CC interview (spec step 1)
        set_transition(InterviewState::GET_VERSION_INFO, StepResultCode::DONE, InterviewState::PREPARE_VERSION_CC_LIST);
        set_transition(InterviewState::GET_VERSION_INFO, StepResultCode::SKIP, InterviewState::PREPARE_VERSION_CC_LIST);
        set_transition(InterviewState::PREPARE_VERSION_CC_LIST, StepResultCode::DONE, InterviewState::VERSION_CC_SEQUENCE);
        set_transition(InterviewState::PREPARE_VERSION_CC_LIST, StepResultCode::SKIP, InterviewState::GET_ZWAVEPLUS_INFO);
        set_transition(InterviewState::VERSION_CC_SEQUENCE, StepResultCode::DONE, InterviewState::GET_VERSION_REPORT);
        set_transition(InterviewState::VERSION_CC_SEQUENCE, StepResultCode::SKIP, InterviewState::GET_VERSION_CAPABILITIES);
        set_transition(InterviewState::GET_VERSION_REPORT, StepResultCode::DONE, InterviewState::VERSION_CC_SEQUENCE);
        set_transition(InterviewState::GET_VERSION_REPORT, StepResultCode::SKIP, InterviewState::GET_VERSION_CAPABILITIES);
        set_transition(InterviewState::GET_VERSION_CAPABILITIES, StepResultCode::DONE, InterviewState::GET_VERSION_ZWAVE_SOFTWARE);
        set_transition(InterviewState::GET_VERSION_CAPABILITIES, StepResultCode::SKIP, InterviewState::GET_VERSION_ZWAVE_SOFTWARE);
        set_transition(InterviewState::GET_VERSION_ZWAVE_SOFTWARE, StepResultCode::DONE, InterviewState::GET_ZWAVEPLUS_INFO);
        set_transition(InterviewState::GET_VERSION_ZWAVE_SOFTWARE, StepResultCode::SKIP, InterviewState::GET_ZWAVEPLUS_INFO);

        // Phase 3: Z-Wave Plus Info and Wake Up
        set_transition(InterviewState::GET_ZWAVEPLUS_INFO, StepResultCode::DONE, InterviewState::INTERVIEW_WAKE_UP);
        set_transition(InterviewState::GET_ZWAVEPLUS_INFO, StepResultCode::SKIP, InterviewState::INTERVIEW_WAKE_UP);
        set_transition(InterviewState::INTERVIEW_WAKE_UP, StepResultCode::DONE, InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS);
        set_transition(InterviewState::INTERVIEW_WAKE_UP, StepResultCode::SKIP, InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS);

        // Phase 4: Association / MCA and AGI
        set_transition(InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS, StepResultCode::DONE, InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT);
        set_transition(InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS, StepResultCode::SKIP, InterviewState::GET_ASSOCIATION_SUPPORTED_GROUPINGS);
        set_transition(InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT, StepResultCode::DONE, InterviewState::GET_ASSOCIATION_MEMBERS);
        set_transition(InterviewState::GET_ASSOCIATION_SUPPORTED_GROUPINGS, StepResultCode::DONE, InterviewState::GET_ASSOCIATION_MEMBERS);
        set_transition(InterviewState::GET_ASSOCIATION_SUPPORTED_GROUPINGS, StepResultCode::SKIP, InterviewState::GET_AGI_GROUP_COUNT);
        set_transition(InterviewState::GET_AGI_GROUP_COUNT, StepResultCode::DONE, InterviewState::GET_AGI_GROUP_NAME);
        set_transition(InterviewState::GET_AGI_GROUP_COUNT, StepResultCode::SKIP, InterviewState::GET_AGI_GROUP_NAME);
        set_transition(InterviewState::GET_ASSOCIATION_MEMBERS, StepResultCode::DONE, InterviewState::GET_AGI_GROUP_NAME);
        set_transition(InterviewState::GET_ASSOCIATION_MEMBERS, StepResultCode::SKIP, InterviewState::GET_AGI_GROUP_NAME);
        set_transition(InterviewState::GET_AGI_GROUP_NAME, StepResultCode::DONE, InterviewState::GET_AGI_GROUP_INFO);
        set_transition(InterviewState::GET_AGI_GROUP_NAME, StepResultCode::SKIP, InterviewState::SET_LIFELINE);
        set_transition(InterviewState::GET_AGI_GROUP_INFO, StepResultCode::DONE, InterviewState::GET_AGI_GROUP_COMMAND_LIST);
        set_transition(InterviewState::GET_AGI_GROUP_COMMAND_LIST, StepResultCode::DONE, InterviewState::GET_AGI_GROUP_NAME);
        set_transition(InterviewState::SET_LIFELINE, StepResultCode::DONE, InterviewState::VALIDATE_LIFELINE);
        set_transition(InterviewState::SET_LIFELINE, StepResultCode::SKIP, InterviewState::POST_VALIDATE_LIFELINE);
        set_transition(InterviewState::VALIDATE_LIFELINE, StepResultCode::DONE, InterviewState::POST_VALIDATE_LIFELINE);
        set_transition(InterviewState::VALIDATE_LIFELINE, StepResultCode::SKIP, InterviewState::POST_VALIDATE_LIFELINE);
        set_transition(InterviewState::POST_VALIDATE_LIFELINE, StepResultCode::DONE, InterviewState::CHECK_MULTI_CHANNEL_SUPPORT);
        set_transition(InterviewState::POST_VALIDATE_LIFELINE, StepResultCode::SKIP, InterviewState::ENDPOINT_ASSOCIATION_ITERATOR);

        // Phase 5: Multi Channel discovery
        set_transition(InterviewState::CHECK_MULTI_CHANNEL_SUPPORT, StepResultCode::DONE, InterviewState::MC_ENDPOINT_GET);
        set_transition(InterviewState::CHECK_MULTI_CHANNEL_SUPPORT, StepResultCode::SKIP, InterviewState::COMPLETED);
        set_transition(InterviewState::MC_ENDPOINT_GET, StepResultCode::DONE, InterviewState::GET_NUMBER_OF_ENDPOINTS);
        set_transition(InterviewState::MC_ENDPOINT_GET, StepResultCode::SKIP, InterviewState::COMPLETED);
        set_transition(InterviewState::GET_NUMBER_OF_ENDPOINTS, StepResultCode::DONE, InterviewState::GET_ENDPOINT_CAPABILITIES);
        set_transition(InterviewState::GET_NUMBER_OF_ENDPOINTS, StepResultCode::SKIP, InterviewState::GET_ENDPOINT_CAPABILITIES);
        set_transition(InterviewState::GET_ENDPOINT_CAPABILITIES, StepResultCode::DONE, InterviewState::GET_ENDPOINT_S2_CAPABILITIES);
        set_transition(InterviewState::GET_ENDPOINT_S2_CAPABILITIES, StepResultCode::DONE, InterviewState::GET_ENDPOINT_S0_CAPABILITIES);
        set_transition(InterviewState::GET_ENDPOINT_S2_CAPABILITIES, StepResultCode::SKIP, InterviewState::GET_ENDPOINT_S0_CAPABILITIES);
        set_transition(InterviewState::GET_ENDPOINT_S0_CAPABILITIES, StepResultCode::DONE, InterviewState::PREPARE_ENDPOINT_VERSIONS);
        set_transition(InterviewState::GET_ENDPOINT_S0_CAPABILITIES, StepResultCode::SKIP, InterviewState::PREPARE_ENDPOINT_VERSIONS);

        // Phase 6: Endpoint Version CC queries
        set_transition(InterviewState::PREPARE_ENDPOINT_VERSIONS, StepResultCode::DONE, InterviewState::ENDPOINT_VERSION_CC_SEQUENCE);
        set_transition(InterviewState::PREPARE_ENDPOINT_VERSIONS, StepResultCode::SKIP, InterviewState::ENDPOINT_ZWAVEPLUS_INFO);
        set_transition(InterviewState::ENDPOINT_VERSION_CC_SEQUENCE, StepResultCode::DONE, InterviewState::ENDPOINT_GET_VERSION_REPORT);
        set_transition(InterviewState::ENDPOINT_VERSION_CC_SEQUENCE, StepResultCode::SKIP, InterviewState::ENDPOINT_ZWAVEPLUS_INFO);
        set_transition(InterviewState::ENDPOINT_GET_VERSION_REPORT, StepResultCode::DONE, InterviewState::ENDPOINT_VERSION_CC_SEQUENCE);

        // Phase 7: Per-endpoint Z-Wave Plus Info and Association/AGI
        set_transition(InterviewState::ENDPOINT_ZWAVEPLUS_INFO, StepResultCode::DONE, InterviewState::ENDPOINT_ASSOCIATION_ITERATOR);
        set_transition(InterviewState::ENDPOINT_ZWAVEPLUS_INFO, StepResultCode::SKIP, InterviewState::ENDPOINT_ASSOCIATION_ITERATOR);
        set_transition(InterviewState::ENDPOINT_ASSOCIATION_ITERATOR, StepResultCode::DONE, InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS);
        set_transition(InterviewState::ENDPOINT_ASSOCIATION_ITERATOR, StepResultCode::SKIP, InterviewState::COMPLETED);
    }

    void InterviewStateMachine::register_steps()
    {
        // Phase 1: Node discovery and security capabilities
        register_step(InterviewState::NODE_INFORMATION, std::make_unique<NodeInformationStep>());
        register_step(InterviewState::S0_COMMANDS_SUPPORTED, std::make_unique<S0CommandsSupportedStep>());
        register_step(InterviewState::S2_COMMANDS_SUPPORTED, std::make_unique<S2CommandsSupportedStep>());

        // Phase 2: Version CC interview (spec step 1)
        register_step(InterviewState::GET_VERSION_INFO, std::make_unique<VersionReportStep>());
        register_step(InterviewState::GET_VERSION_CAPABILITIES, std::make_unique<VersionCapabilitiesInterviewStep>());
        register_step(InterviewState::GET_VERSION_ZWAVE_SOFTWARE, std::make_unique<VersionZwaveSoftwareInterviewStep>());
        register_step(InterviewState::PREPARE_VERSION_CC_LIST, std::make_unique<PrepareVersionCCListStep>());
        register_step(InterviewState::VERSION_CC_SEQUENCE, std::make_unique<VersionCCSequenceStep>());
        register_step(InterviewState::GET_VERSION_REPORT, std::make_unique<VersionGetStep>());

        // Phase 3: Z-Wave Plus Info and Wake Up
        register_step(InterviewState::GET_ZWAVEPLUS_INFO, std::make_unique<ZWavePlusInfoStep>());
        register_step(InterviewState::INTERVIEW_WAKE_UP, std::make_unique<WakeUpStep>());

        // Phase 4: Association / MCA and AGI
        register_step(InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS, std::make_unique<MultiChannelAssociationSupportedGroupingsStep>());
        register_step(InterviewState::GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT, std::make_unique<MultiChannelAssociationSupportedGroupingsCountStep>());
        register_step(InterviewState::GET_ASSOCIATION_SUPPORTED_GROUPINGS, std::make_unique<AssociationSupportedGroupingsStep>());
        register_step(InterviewState::GET_AGI_GROUP_COUNT, std::make_unique<GetAgiGroupCountStep>());
        register_step(InterviewState::GET_ASSOCIATION_MEMBERS, std::make_unique<AssociationGetStep>());
        register_step(InterviewState::SET_LIFELINE, std::make_unique<LifelineSetStep>());
        register_step(InterviewState::VALIDATE_LIFELINE, std::make_unique<LifelineValidateStep>());
        register_step(InterviewState::POST_VALIDATE_LIFELINE, std::make_unique<PostValidateLifelineStep>());
        register_step(InterviewState::GET_AGI_GROUP_NAME, std::make_unique<AgiGroupNameGetStep>());
        register_step(InterviewState::GET_AGI_GROUP_INFO, std::make_unique<AgiGroupInfoGetStep>());
        register_step(InterviewState::GET_AGI_GROUP_COMMAND_LIST, std::make_unique<AgiGroupCommandListGetStep>());

        // Phase 5: Multi Channel discovery
        register_step(InterviewState::CHECK_MULTI_CHANNEL_SUPPORT, std::make_unique<CheckMultiChannelSupportStep>());
        register_step(InterviewState::MC_ENDPOINT_GET, std::make_unique<McEndpointGetStep>());
        register_step(InterviewState::GET_NUMBER_OF_ENDPOINTS, std::make_unique<GetNumberOfEndpointsStep>());
        register_step(InterviewState::GET_ENDPOINT_CAPABILITIES, std::make_unique<GetEndpointCapabilitiesStep>());
        register_step(InterviewState::GET_ENDPOINT_S2_CAPABILITIES, std::make_unique<GetEndpointS2CapabilitiesStep>());
        register_step(InterviewState::GET_ENDPOINT_S0_CAPABILITIES, std::make_unique<GetEndpointS0CapabilitiesStep>());

        // Phase 6: Endpoint Version CC queries
        register_step(InterviewState::PREPARE_ENDPOINT_VERSIONS, std::make_unique<PrepareEndpointVersionsStep>());
        register_step(InterviewState::ENDPOINT_VERSION_CC_SEQUENCE, std::make_unique<VersionCCSequenceStep>());
        register_step(InterviewState::ENDPOINT_GET_VERSION_REPORT, std::make_unique<VersionGetStep>());

        // Phase 7: Per-endpoint Z-Wave Plus Info and Association/AGI
        register_step(InterviewState::ENDPOINT_ZWAVEPLUS_INFO, std::make_unique<GetEndpointZwavePlusInfoStep>());
        register_step(InterviewState::ENDPOINT_ASSOCIATION_ITERATOR, std::make_unique<EndpointAssociationIteratorStep>());

        // Interview complete
        register_step(InterviewState::COMPLETED, std::make_unique<CompletedStep>());
    }

    InterviewStateMachine::InterviewStateMachine() : state_machine::state_machine_base<InterviewState, InterviewSession, device_interviewer_external_event_data>(InterviewState::FAILED, LOG_TAG.data())
    {
        register_transitions();
        register_steps();
    }

    void InterviewStateMachine::start_interview(zwave_node_id_t node_id, uint8_t endpoint_id, attribute_store::attribute device_node, attribute_store::attribute endpoint_node, zwave_keyset_t granted_keys)
    {
        auto key = std::make_pair(node_id, endpoint_id);

        // Check if there's an existing session for this node/endpoint
        auto existing_it = sessions.find(key);
        if (existing_it != sessions.end()) {
            sl_log_info(LOG_TAG.data(), "Starting new interview for node %d, endpoint %d (replacing existing session in state %d)", node_id, endpoint_id, static_cast<int>(existing_it->second->current_state));
            sessions.erase(existing_it);
        }

        // Create new session; ctor sets current_state to IDLE until transition_to_state runs.
        auto session          = std::make_unique<InterviewSession>(node_id, endpoint_id, device_node, endpoint_node);
        session->granted_keys = granted_keys;

        sessions[key] = std::move(session);

        auto *session_ptr                = sessions[key].get();
        const InterviewState entry_point = InterviewState::NODE_INFORMATION;
        this->transition_to_state(*session_ptr, entry_point);
        session_ptr->last_progress_at = clock_time();

        sl_log_info(LOG_TAG.data(), "Started interview for node %d, endpoint %d", node_id, endpoint_id);
    }

    InterviewSession *InterviewStateMachine::get_session(zwave_node_id_t node_id, uint8_t endpoint_id)
    {
        auto key = std::make_pair(node_id, endpoint_id);
        auto it  = sessions.find(key);
        if (it != sessions.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool InterviewStateMachine::extract_node_info_from_endpoint(attribute_store::attribute endpoint_node, zwave_node_id_t &node_id, uint8_t &endpoint_id)
    {
        if (!endpoint_node.is_valid()) {
            return false;
        }

        sl_status_t status = attribute_store_network_helper_get_node_id_from_node(endpoint_node, &node_id);
        if (status != SL_STATUS_OK || node_id == 0) {
            return false;
        }

        endpoint_id = endpoint_node.reported<uint8_t>();
        return true;
    }

    InterviewSession *InterviewStateMachine::find_session_for_event(const device_interviewer_external_event_data &event)
    {
        // Try to extract node_id and endpoint_id from event payload
        zwave_node_id_t node_id = 0;
        uint8_t endpoint_id     = 0;

        try {
            switch (event.event) {
                case device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_REPORT: {
                    const auto &payload = std::any_cast<command_class_s2_types::s2_supported_report_payload_t>(event.payload);
                    node_id             = payload.connection_info.remote.node_id;
                    endpoint_id         = payload.connection_info.remote.endpoint_id;
                    break;
                }
                case device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_GET_TX_FAILED: {
                    const auto &payload = std::any_cast<command_class_s2_types::s2_supported_get_tx_failed_payload_t>(event.payload);
                    node_id             = payload.zwave_node_id;
                    endpoint_id         = payload.endpoint_id;
                    break;
                }
                case device_interviewer_external_event_t::S0_COMMANDS_SUPPORTED_REPORT: {
                    const auto &payload = std::any_cast<command_class_s0_types::s0_supported_report_payload_t>(event.payload);
                    node_id             = payload.connection_info.remote.node_id;
                    endpoint_id         = payload.connection_info.remote.endpoint_id;
                    break;
                }
                case device_interviewer_external_event_t::NODE_INFORMATION_RECEIVED: {
                    const auto &payload = std::any_cast<component_connector_node_information_received_payload_t>(event.payload);
                    node_id             = payload.node_id;
                    endpoint_id         = 0;
                    break;
                }
                default: {
                    if (!event.device_endpoint_node.has_value()) {
                        return nullptr;
                    }
                    if (!extract_node_info_from_endpoint(event.device_endpoint_node.value(), node_id, endpoint_id)) {
                        return nullptr;
                    }
                    break;
                }
            }
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Failed to extract node_id from event payload (event type: %d)", static_cast<int>(event.event));
            return nullptr;
        }

        if (node_id == 0) {
            return nullptr;
        }

        // Try exact match first
        auto *session = get_session(node_id, endpoint_id);
        if (session != nullptr) {
            return session;
        }

        // Try endpoint 0 if we didn't find exact match
        if (endpoint_id != 0) {
            session = get_session(node_id, 0);
            if (session != nullptr) {
                return session;
            }
        }

        return nullptr;
    }

    void InterviewStateMachine::publish_interview_failure(const InterviewSession &session)
    {
        // Publish failure without INTERVIEW_DONE so CC on_interview hooks do not run on incomplete data.
        if (!session.endpoint_node.is_valid()) {
            return;
        }

        component_connector connector;
        component_connector_interview_done_payload_t payload;
        payload.endpoint_node = session.endpoint_node;
        payload.status        = SL_STATUS_FAIL;
        connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED), payload);
    }

    void InterviewStateMachine::finalize_failed_session(zwave_node_id_t node_id, uint8_t endpoint_id)
    {
        auto key = std::make_pair(node_id, endpoint_id);
        auto it  = sessions.find(key);
        if (it == sessions.end()) {
            return;
        }

        publish_interview_failure(*it->second);
        sessions.erase(it);
    }

    sl_status_t InterviewStateMachine::process_event(const device_interviewer_external_event_data &event)
    {
        if (event.event == device_interviewer_external_event_t::NODE_DELETED) {
            try {
                const auto &payload = std::any_cast<component_connector_node_deleted_payload_t>(event.payload);
                std::vector<std::pair<zwave_node_id_t, uint8_t>> keys_to_erase;
                for (auto &[key, session]: sessions) {
                    if (key.first != payload.node_id) {
                        continue;
                    }
                    // Publish fail for in-progress interviews so MQTT clients unblock; then drop all sessions for the node.
                    if (session->current_state != InterviewState::COMPLETED && session->current_state != InterviewState::FAILED) {
                        sl_log_info(LOG_TAG.data(), "Node %d deleted, failing interview for endpoint %d", payload.node_id, session->endpoint_id);
                        publish_interview_failure(*session);
                    } else {
                        sl_log_debug(LOG_TAG.data(), "Node %d deleted, clearing interview for endpoint %d", payload.node_id, session->endpoint_id);
                    }
                    keys_to_erase.push_back(key);
                }
                for (const auto &key: keys_to_erase) {
                    sessions.erase(key);
                }
                return SL_STATUS_OK;
            } catch (const std::bad_any_cast &) {
                sl_log_error(LOG_TAG.data(), "Invalid payload type for NODE_DELETED event");
                return SL_STATUS_FAIL;
            }
        }

        if (event.event == device_interviewer_external_event_t::FACTORY_RESET) {
            sl_log_info(LOG_TAG.data(), "Factory reset: clearing all interview sessions");
            sessions.clear();
            return SL_STATUS_OK;
        }

        // Find the session for this event
        InterviewSession *session = find_session_for_event(event);
        if (session == nullptr) {
            // Event received for a node/endpoint without an active interview
            // This can happen if:
            // 1. Event arrives after interview completed
            // 2. Event arrives for a node that never started an interview
            // 3. External component fires event incorrectly
            sl_log_debug(LOG_TAG.data(), "Ignoring event %s - no active interview session found (event may be stale or for wrong node)", to_string(event.event));
            return SL_STATUS_NOT_FOUND;
        }

        auto *base_step = get_step(session->current_state);
        if (base_step == nullptr) {
            sl_log_error(LOG_TAG.data(), "No step registered for state %d (node %d, endpoint %d)", static_cast<int>(session->current_state), session->node_id, session->endpoint_id);
            return SL_STATUS_INVALID_STATE;
        }

        auto *step = static_cast<InterviewStep *>(base_step);

        {
            std::string state_name = step->name();
            sl_log_debug(LOG_TAG.data(), "Processing event %s for node %d, endpoint %d in state %s", to_string(event.event), session->node_id, session->endpoint_id, state_name.c_str());
        }

        // Check if this step handles this event type
        // This protects against events arriving in wrong state (e.g., VERSION_REPORT in S2_COMMANDS_SUPPORTED)
        if (!step->handles_external_event(event.event)) {
            return SL_STATUS_INVALID_STATE;
        }

        // Process the event
        const InterviewState state_before = session->current_state;
        const zwave_node_id_t node_id     = session->node_id;
        state_machine::StepResult result  = step->handle_event(*session, std::make_optional(event));

        this->apply_transition(*session, result);

        // Step fail() (e.g. S2 TX retries exhausted) — publish Interview/Report and drop the session.
        // Sessions are keyed by the start_interview endpoint (always 0); session.endpoint_id may differ mid-interview.
        if (session->current_state == InterviewState::FAILED) {
            finalize_failed_session(node_id, uint8_t {0});
            return result.status;
        }

        if (session->current_state != state_before) {
            session->last_progress_at = clock_time();
        }

        if (session->current_state == InterviewState::COMPLETED) {
            sl_log_debug(LOG_TAG.data(), "Clearing interview session for node %d, endpoint %d (terminal state %d)", session->node_id, session->endpoint_id, static_cast<int>(session->current_state));
            sessions.erase(std::make_pair(session->node_id, uint8_t {0}));
        }

        return result.status;
    }

    void InterviewStateMachine::abort_stale_sessions()
    {
        const clock_time_t now = clock_time();
        std::vector<std::pair<zwave_node_id_t, uint8_t>> stale_keys;

        for (auto &[key, session]: sessions) {
            if (session->current_state == InterviewState::IDLE || session->current_state == InterviewState::COMPLETED || session->current_state == InterviewState::FAILED) {
                continue;
            }

            if ((now - session->last_progress_at) <= stall_timeout_ms_for_node(session->node_id)) {
                continue;
            }

            auto *base_step             = get_step(session->current_state);
            const std::string step_name = (base_step != nullptr) ? base_step->name() : "UNKNOWN";
            sl_log_warning(LOG_TAG.data(), "Node %d: interview stalled in step %s for %lu ms — aborting", session->node_id, step_name.c_str(), static_cast<unsigned long>(now - session->last_progress_at));

            stale_keys.push_back(key);
        }

        for (const auto &key: stale_keys) {
            finalize_failed_session(key.first, key.second);
        }
    }

}  // namespace zwave_command_class
