
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

#ifndef INTERVIEW_STATE_MACHINE_H
#define INTERVIEW_STATE_MACHINE_H

#include "sl_status.h"
#include "device_interviewer_types.hpp"
#include "device_interviewer_external_event_types.hpp"  // For device_interviewer_external_event_t and device_interviewer_external_event_data
#include "attribute.hpp"
#include "zwave_generic_types.h"
#include "state_machine_base.hpp"
#include "clock_platform.h"
#include <memory>
#include <map>
#include <vector>
#include <any>
#include <string>
#include <utility>

namespace zwave_command_class
{

    /**
     * @brief Enumeration of interview states
     */
    enum class InterviewState {
        IDLE,
        S2_COMMANDS_SUPPORTED,
        S0_COMMANDS_SUPPORTED,
        NODE_INFORMATION,
        GET_VERSION_INFO,
        GET_VERSION_CAPABILITIES,
        GET_VERSION_ZWAVE_SOFTWARE,
        PREPARE_VERSION_CC_LIST,
        GET_VERSION_REPORT,
        VERSION_CC_SEQUENCE,
        GET_ZWAVEPLUS_INFO,
        INTERVIEW_WAKE_UP,
        GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS,
        GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT,
        GET_ASSOCIATION_SUPPORTED_GROUPINGS,
        GET_AGI_GROUP_COUNT,
        GET_ASSOCIATION_MEMBERS,
        SET_LIFELINE,
        VALIDATE_LIFELINE,
        POST_VALIDATE_LIFELINE,
        GET_AGI_GROUP_NAME,
        GET_AGI_GROUP_INFO,
        GET_AGI_GROUP_COMMAND_LIST,
        CHECK_MULTI_CHANNEL_SUPPORT,
        MC_ENDPOINT_GET,
        GET_NUMBER_OF_ENDPOINTS,
        GET_ENDPOINT_CAPABILITIES,
        GET_ENDPOINT_S2_CAPABILITIES,
        GET_ENDPOINT_S0_CAPABILITIES,
        PREPARE_ENDPOINT_VERSIONS,
        ENDPOINT_VERSION_CC_SEQUENCE,
        ENDPOINT_GET_VERSION_REPORT,
        ENDPOINT_ZWAVEPLUS_INFO,
        ENDPOINT_ASSOCIATION_ITERATOR,
        COMPLETED,
        FAILED
    };

    /**
     * @brief Root + endpoint version CC ping-pong (PREPARE_VERSION_CC_LIST, VERSION_CC_SEQUENCE, GET_VERSION_REPORT;
     *        same pattern for endpoint flow in PREPARE_ENDPOINT_VERSIONS / ENDPOINT_*).
     */
    struct VersionCcProgress {
            std::vector<uint8_t> command_classes_to_query;
            std::vector<uint8_t>::iterator current_cc_it;
    };

    /**
     * @brief Multi-endpoint iteration (GET_NUMBER_OF_ENDPOINTS, GET_ENDPOINT_*, ENDPOINT_ZWAVEPLUS_INFO, COMPLETED, etc.).
     */
    struct EndpointProgress {
            std::vector<uint8_t> endpoint_ids;
            std::vector<uint8_t>::iterator current_endpoint_it;
            std::vector<uint8_t> endpoint_discovered_command_classes;
    };

    /**
     * @brief WakeUpStep sub-phases.
     *
     * v2: PendingKick → AwaitingCapabilitiesReport (Capabilities Get/Report) →
     *     AwaitingIntervalSetResolution (Interval Set) → AwaitingPostSetIntervalReport
     *     (after set resolution + Interval Get queued) → Interval Report → done.
     * v1: PendingKick → AwaitingIntervalSetResolution (Set) → AwaitingPostSetIntervalReport → Report → done.
     */
    struct WakeUpProgress {
            enum class Phase { PendingKick, AwaitingCapabilitiesReport, AwaitingIntervalSetResolution, AwaitingPostSetIntervalReport };
            Phase phase = Phase::PendingKick;
            /// Wake Up CC version from Version CC (0 = treat as 1).
            uint8_t command_class_version = 1;
    };

    /**
     * @brief McEndpointGetStep dynamic endpoint flag.
     */
    struct MultiChannelProgress {
            bool mc_has_dynamic_endpoints = false;
    };

    /**
     * @brief AssociationGetStep group iteration.
     */
    struct AssociationMembersProgress {
            uint8_t assoc_current_group_id = 0;
    };

    /**
     * @brief AGI name/info/command-list steps and lifeline helpers.
     */
    struct AgiProgress {
            bool agi_used_multi_channel  = false;
            uint8_t agi_total_groups     = 0;
            uint8_t agi_current_group_id = 0;
    };

    /**
     * @brief Context for tracking interview progress for a device/endpoint
     */
    struct InterviewSession {
            zwave_node_id_t node_id;
            uint8_t endpoint_id;
            InterviewState current_state;
            attribute_store::attribute device_node;
            attribute_store::attribute endpoint_node;
            attribute_store::attribute root_endpoint_node;  // Restored after per-endpoint association phase

            // S2 capabilities (stored by S2CommandsSupportedStep when report received)
            std::vector<uint8_t> s2_supported_command_classes;
            /// TX failures for S2 Commands Supported Get (root or current endpoint).
            uint8_t s2_commands_supported_tx_retries = 0;

            // S0 capabilities (stored by S0CommandsSupportedStep when report received)
            std::vector<uint8_t> s0_supported_command_classes;

            // Node information command class list (stored by NodeInformationStep when report received)
            std::vector<uint8_t> node_information_command_class_list;

            VersionCcProgress version_cc;
            EndpointProgress endpoints;
            WakeUpProgress wake_up;
            MultiChannelProgress multi_channel;
            AssociationMembersProgress association_members;
            AgiProgress agi;

            /// Set from Version Capabilities Report during GET_VERSION_CAPABILITIES (Z-Wave Software bit).
            bool version_zwave_software_supported = false;

            // Interview metadata
            zwave_keyset_t granted_keys;

            /// Last time the interview made a state-machine transition (stall detection).
            clock_time_t last_progress_at = 0;

            InterviewSession(zwave_node_id_t nid, uint8_t eid, attribute_store::attribute dev_node, attribute_store::attribute ep_node) : node_id(nid), endpoint_id(eid), current_state(InterviewState::IDLE), device_node(dev_node), endpoint_node(ep_node), root_endpoint_node(ep_node), granted_keys(0)
            {}
    };

    /**
     * @brief State machine for managing device interview process
     */
    class InterviewStateMachine : public state_machine::state_machine_base<InterviewState, InterviewSession, device_interviewer_external_event_data>
    {
        public:
            InterviewStateMachine();
            ~InterviewStateMachine() override = default;

            /**
             * @brief Process an event through the state machine
             * @param event The event to process
             * @return Status of processing
             */
            sl_status_t process_event(const device_interviewer_external_event_data &event);

            /**
             * @brief Start an interview for a node
             * @param node_id The node to interview
             * @param endpoint_id The endpoint to start with (usually 0)
             * @param device_node The device node in attribute store
             * @param endpoint_node The endpoint node in attribute store
             * @param granted_keys Security keys granted
             */
            void start_interview(zwave_node_id_t node_id, uint8_t endpoint_id, attribute_store::attribute device_node, attribute_store::attribute endpoint_node, zwave_keyset_t granted_keys);

            /**
             * @brief Get session for a node/endpoint
             * @param node_id Node ID
             * @param endpoint_id Endpoint ID
             * @return Pointer to session or nullptr if not found
             */
            InterviewSession *get_session(zwave_node_id_t node_id, uint8_t endpoint_id);

            /**
             * @brief Abort interviews that have made no state progress for too long.
             *
             * AL/FL: 60 s. NL: max(2 × zpc.default_wake_up_interval, 15 min).
             * Fires INTERVIEW_FULLY_RESOLVED with fail status and erases the session.
             */
            void abort_stale_sessions();

        private:
            // Map: (node_id, endpoint_id) -> session
            std::map<std::pair<zwave_node_id_t, uint8_t>, std::unique_ptr<InterviewSession>> sessions;

            /**
             * @brief Find or create session for an event
             */
            InterviewSession *find_session_for_event(const device_interviewer_external_event_data &event);

            void register_transitions();

            /**
             * @brief Register all interview steps
             */
            void register_steps();

            /**
             * @brief Publish INTERVIEW_FULLY_RESOLVED (FAIL) for a session's endpoint.
             *
             * Used by step fail(), NODE_DELETED, and stall abort so MQTT clients
             * always get Interview/Report.
             */
            static void publish_interview_failure(const InterviewSession &session);

            /**
             * @brief Publish interview failure and erase the session for (node_id, endpoint_id).
             */
            void finalize_failed_session(zwave_node_id_t node_id, uint8_t endpoint_id);

            /**
             * @brief Extract node_id and endpoint_id from an attribute store endpoint node
             * @param endpoint_node The endpoint node from attribute store
             * @param node_id Output parameter for node_id
             * @param endpoint_id Output parameter for endpoint_id
             * @return true if extraction succeeded, false otherwise
             */
            static bool extract_node_info_from_endpoint(attribute_store::attribute endpoint_node, zwave_node_id_t &node_id, uint8_t &endpoint_id);
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STATE_MACHINE_H
