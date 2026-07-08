
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
#include <string_view>

#include "log.h"

#include "device_interviewer.hpp"
#include "device_interviewer_events.hpp"
#include "device_interviewer_types.hpp"
#include "interview_state_machine.hpp"

#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"

#include "command_class_s2_events.hpp"
#include "command_class_s2_types.hpp"
#include "command_class_s0_events.hpp"
#include "command_class_s0_types.hpp"

#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store_network_helper.h"
#include "zwave_utils.h"
#include "device_interviewer_attribute_store.hpp"

#include "command_class_version_types.hpp"
#include "command_class_version_events.hpp"

#include "command_class_multi_channel_events.hpp"
#include "command_class_multi_channel_types.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_types.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "command_class_zwaveplus_info_events.hpp"
#include "command_class_zwaveplus_info_types.hpp"
#include "command_class_wake_up_events.hpp"
#include "command_class_wake_up_types.hpp"

#include "zwave_command_class_utils.hpp"

namespace zwave_command_class
{
    using namespace command_class_association_grp_info_types;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "device_interviewer";

    static bool nif_contains_command_class(const std::optional<std::vector<uint8_t>> &command_class_list, uint8_t command_class)
    {
        return command_class_list.has_value() && std::find(command_class_list->begin(), command_class_list->end(), command_class) != command_class_list->end();
    }

    static std::optional<std::vector<uint8_t>> read_reported_byte_array(const attribute_store::attribute &node)
    {
        if (node.is_valid() && node.reported_exists()) {
            return node.reported<std::vector<uint8_t>>();
        }
        return std::nullopt;
    }

    static std::optional<std::vector<uint8_t>> read_reported_normal_command_class_list(const attribute_store::attribute &node)
    {
        const auto wire_list = read_reported_byte_array(node);
        if (!wire_list.has_value()) {
            return std::nullopt;
        }
        return command_class_utils::get_normal_command_classes(wire_list.value());
    }

    // Static event queue definition
    ::threading::safe_queue<device_interviewer_external_event_data> device_interviewer::external_event_queue;

    device_interviewer::device_interviewer() : threading::threading("Device Interviewer")
    {
        // Initialize state machine
        state_machine = std::make_unique<InterviewStateMachine>();

        // Register attribute store types
        sl_status_t status = SL_STATUS_OK;
        for (attribute_schema_t const &a: attributes) {
            status = attribute_store_register_type(a.type, a.name, a.parent_type, a.storage_type);
            if (status != SL_STATUS_OK) {
                sl_log_critical(LOG_TAG.data(), "Failed to register user attribute %s for command class 0x%.2x", a.name);
            }
        }

        // Register all event handlers
        register_event_handlers();
    }

    /**
     * @brief Start (or skip) an interview for a node.
     *
     * Called directly from component_connector callbacks (not via external_event_queue), on the
     * publisher thread. Invokes the interview state machine's start path when appropriate.
     */
    sl_status_t device_interviewer::trigger_start_interview(const component_connector_node_added_payload_t &p)
    {
        // Do not interview nodes whose inclusion / security bootstrapping failed
        // (e.g. SmartStart S2 timeout before self-destruct).
        if ((p.status != SL_STATUS_OK) || (p.kex_fail_type != ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE)) {
            sl_log_info(LOG_TAG.data(), "Node %d: Skipping interview after add/security failure (status=%d, kex_fail=%d).", p.node_id, static_cast<int>(p.status), static_cast<int>(p.kex_fail_type));
            return SL_STATUS_OK;
        }

        attribute_store_node_t node_id_node = attribute_store_network_helper_get_zwave_node_id_node(p.node_id);
        if (node_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_warning(LOG_TAG.data(), "Node %d: Node not found in attribute store. Interview will not be started.", p.node_id);
            return SL_STATUS_OK;
        }

        attribute_store::attribute device_node(node_id_node);
        attribute_store_node_t endpoint_0_node = device_node.child_by_type(ATTRIBUTE_ENDPOINT_ID);
        if (endpoint_0_node == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_warning(LOG_TAG.data(), "Node %d: Endpoint 0 not found in attribute store. Interview will not be started.", p.node_id);
            return SL_STATUS_OK;
        }

        auto *existing_session = this->state_machine->get_session(p.node_id, 0);
        if (existing_session != nullptr && existing_session->current_state != InterviewState::IDLE && existing_session->current_state != InterviewState::COMPLETED) {
            if (existing_session->granted_keys != p.granted_keys) {
                sl_log_debug(LOG_TAG.data(), "Node %d: Interview already in progress, updating granted_keys from 0x%02X to 0x%02X", p.node_id, existing_session->granted_keys, p.granted_keys);
                existing_session->granted_keys = p.granted_keys;
            }
            return SL_STATUS_OK;
        }

        sl_log_info(LOG_TAG.data(), "Node %d: Starting interview (granted keys: 0x%02X, kex_fail: %d)", p.node_id, p.granted_keys, static_cast<int>(p.kex_fail_type));

        this->state_machine->start_interview(p.node_id, 0, device_node, attribute_store::attribute(endpoint_0_node), p.granted_keys);

        return SL_STATUS_OK;
    }

    /**
     * @brief Register handlers for events from other components.
     *
     * Most handlers call queue_event() so the device_interviewer thread (run()) can process
     * them through the state machine.
     *
     * Event categories:
     * - Node lifecycle:
     *   - NODE_ADDED: starts the interview synchronously on the callback thread.
     *   - NODE_INTERVIEW_REQUESTED: starts the interview synchronously on the callback thread.
     *     Fired by command_class_inclusion_controller (no-handoff fallback) and OTA (post-update
     *     re-interview); deferral / handoff arbitration lives in those producers, not here.
     *   - NODE_DELETED: queued for the state machine.
     * - Command class and connector reports: queued for state machine processing
     * - Synchronous handlers: simple requests that do not use the state machine (e.g. GET_NODE_INFORMATION)
     */
    void device_interviewer::register_event_handlers()
    {
        component_connector connector;

        // ============================================================================
        // Node lifecycle events
        // ============================================================================

        // Node deletion: Fail the interview when a node is being excluded/deleted
        connector.connect_typed<component_connector_common_events_t, component_connector_node_deleted_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_DELETED, [](const component_connector_node_deleted_payload_t &p) {
            queue_event(device_interviewer_external_event_t::NODE_DELETED, p);
            return SL_STATUS_OK;
        });

        connector.connect_typed<component_connector_common_events_t, component_connector_factory_reset_complete_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_FACTORY_RESET_COMPLETE, [](const component_connector_factory_reset_complete_payload_t &p) {
            queue_event(device_interviewer_external_event_t::FACTORY_RESET, p);
            return SL_STATUS_OK;
        });

        // Node added: local inclusion after security bootstrapping completes
        connector.connect_typed<component_connector_common_events_t, component_connector_node_added_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_ADDED, [this](const component_connector_node_added_payload_t &p) { return this->trigger_start_interview(p); });

        // Interview requested directly (no security handoff context). Synthesize a NODE_ADDED-shaped
        // payload from the attribute store and dispatch the same way as a real NODE_ADDED.
        connector.connect_typed<component_connector_common_events_t, component_connector_node_interview_requested_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_INTERVIEW_REQUESTED, [this](const component_connector_node_interview_requested_payload_t &p) {
            component_connector_node_added_payload_t synthesized {};
            synthesized.status             = SL_STATUS_OK;
            synthesized.node_id            = p.node_id;
            synthesized.inclusion_protocol = zwave_get_inclusion_protocol(p.node_id);
            if (zwave_get_node_granted_keys(p.node_id, &synthesized.granted_keys) != SL_STATUS_OK) {
                synthesized.granted_keys = 0;
            }
            return this->trigger_start_interview(synthesized);
        });

        // ============================================================================
        // Command class report events (queued for state machine processing)
        //
        // Note: These events are validated in process_event() to ensure they match
        // an active interview session and the current state. Events for nodes without
        // active interviews or in wrong states are safely ignored with appropriate logging.
        // This protects against:
        // - Stale events (queued before cancellation, processed after)
        // - External triggers (events fired incorrectly from other components)
        // - Race conditions (events arriving out of order)
        // ============================================================================

        // S2 Commands Supported Report
        connector.connect_typed<command_class_s2_events_t, command_class_s2_types::s2_supported_report_payload_t>(command_class_s2_events_t::COMMAND_CLASS_S2_COMMANDS_SUPPORTED_REPORT, [](const command_class_s2_types::s2_supported_report_payload_t &p) {
            queue_event(device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_REPORT, p);
            return SL_STATUS_OK;
        });

        // S2 Commands Supported Get TX failed (enqueue or air failure)
        connector.connect_typed<command_class_s2_events_t, command_class_s2_types::s2_supported_get_tx_failed_payload_t>(command_class_s2_events_t::COMMAND_CLASS_S2_COMMANDS_SUPPORTED_GET_TX_FAILED, [](const command_class_s2_types::s2_supported_get_tx_failed_payload_t &p) {
            queue_event(device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_GET_TX_FAILED, p);
            return SL_STATUS_OK;
        });

        // S0 Commands Supported Report
        connector.connect_typed<command_class_s0_events_t, command_class_s0_types::s0_supported_report_payload_t>(command_class_s0_events_t::COMMAND_CLASS_S0_COMMANDS_SUPPORTED_REPORT, [](const command_class_s0_types::s0_supported_report_payload_t &p) {
            queue_event(device_interviewer_external_event_t::S0_COMMANDS_SUPPORTED_REPORT, p);
            return SL_STATUS_OK;
        });

        // Node Information Received
        connector.connect_typed<component_connector_common_events_t, component_connector_node_information_received_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_INFORMATION_RECEIVED, [](const component_connector_node_information_received_payload_t &p) {
            queue_event(device_interviewer_external_event_t::NODE_INFORMATION_RECEIVED, p);
            return SL_STATUS_OK;
        });

        // Version CC Get Requested (internal event from version command class)
        connector.connect_typed<device_interviewer_events_t, command_class_version_types::command_class_version_cc_get_payload_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_CC_GET, [](const command_class_version_types::command_class_version_cc_get_payload_t &p) {
            queue_event(device_interviewer_external_event_t::VERSION_CC_GET_REQUESTED, p, p.device_endpoint_node);
            return SL_STATUS_OK;
        });

        // Multi Channel End Point Find Report
        connector.connect_typed<command_class_multi_channel_events_t, command_class_multi_channel_types::command_class_multi_channel_end_point_find_report_payload_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_FIND_REPORT,
                                                                                                                                                                      [](const command_class_multi_channel_types::command_class_multi_channel_end_point_find_report_payload_t &p) {
                                                                                                                                                                          queue_event(device_interviewer_external_event_t::MULTI_CHANNEL_END_POINT_FIND_REPORT_RECEIVED, p, p.device_endpoint_node);
                                                                                                                                                                          return SL_STATUS_OK;
                                                                                                                                                                      });

        // Multi Channel Commands Capability Report
        connector.connect_typed<command_class_multi_channel_events_t, command_class_multi_channel_types::command_class_multi_channel_commands_capability_report_payload_t>(
          command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT,
          [](const command_class_multi_channel_types::command_class_multi_channel_commands_capability_report_payload_t &p) {
              queue_event(device_interviewer_external_event_t::MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT_RECEIVED, p, p.device_endpoint_node);
              return SL_STATUS_OK;
          });

        // Multi Channel Association Groupings Report
        connector.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_groupings_get_payload_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT,
                                                                                                                                                         [](const component_connector_multi_channel_association_groupings_get_payload_t &p) {
                                                                                                                                                             queue_event(device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED, p, p.endpoint_node);
                                                                                                                                                             return SL_STATUS_OK;
                                                                                                                                                         });

        // Association Groupings Report
        connector.connect_typed<command_class_association_events_t, component_connector_association_groupings_get_payload_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GROUPINGS_REPORT, [](const component_connector_association_groupings_get_payload_t &p) {
            queue_event(device_interviewer_external_event_t::ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED, p, p.endpoint_node);
            return SL_STATUS_OK;
        });

        // Version Report (from Version CC - overall firmware/protocol version)
        connector.connect_typed<device_interviewer_events_t, command_class_version_types::command_class_version_report_callback_payload_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_GET_DONE, [](const command_class_version_types::command_class_version_report_callback_payload_t &p) {
            queue_event(device_interviewer_external_event_t::VERSION_REPORT_RECEIVED, p, p.device_endpoint_node);
            return SL_STATUS_OK;
        });

        connector.connect_typed<device_interviewer_events_t, command_class_version_types::command_class_version_capabilities_report_callback_payload_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_CAPABILITIES_DONE,
                                                                                                                                                        [](const command_class_version_types::command_class_version_capabilities_report_callback_payload_t &p) {
                                                                                                                                                            queue_event(device_interviewer_external_event_t::VERSION_CAPABILITIES_REPORT_RECEIVED, p, p.device_endpoint_node);
                                                                                                                                                            return SL_STATUS_OK;
                                                                                                                                                        });

        connector.connect_typed<device_interviewer_events_t, command_class_version_types::command_class_version_report_callback_payload_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_ZWAVE_SOFTWARE_DONE,
                                                                                                                                           [](const command_class_version_types::command_class_version_report_callback_payload_t &p) {
                                                                                                                                               queue_event(device_interviewer_external_event_t::VERSION_ZWAVE_SOFTWARE_REPORT_RECEIVED, p, p.device_endpoint_node);
                                                                                                                                               return SL_STATUS_OK;
                                                                                                                                           });

        // Multi Channel End Point Report
        connector.connect_typed<command_class_multi_channel_events_t, command_class_multi_channel_types::command_class_multi_channel_end_point_report_payload_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_REPORT_RECEIVED,
                                                                                                                                                                 [](const command_class_multi_channel_types::command_class_multi_channel_end_point_report_payload_t &p) {
                                                                                                                                                                     queue_event(device_interviewer_external_event_t::MULTI_CHANNEL_END_POINT_REPORT_RECEIVED, p, p.device_endpoint_node);
                                                                                                                                                                     return SL_STATUS_OK;
                                                                                                                                                                 });

        // Z-Wave Plus Info Report
        connector.connect_typed<command_class_zwaveplus_info_events_t, command_class_zwaveplus_info_types::zwaveplus_info_report_payload_t>(command_class_zwaveplus_info_events_t::COMMAND_CLASS_ZWAVEPLUS_INFO_REPORT_RECEIVED,
                                                                                                                                            [](const command_class_zwaveplus_info_types::zwaveplus_info_report_payload_t &p) {
                                                                                                                                                queue_event(device_interviewer_external_event_t::ZWAVEPLUS_INFO_REPORT_RECEIVED, p, p.device_endpoint_node);
                                                                                                                                                return SL_STATUS_OK;
                                                                                                                                            });

        // Wake Up Capabilities Report
        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_capabilities_report_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_CAPABILITIES_REPORT_RECEIVED, [](const command_class_wake_up_types::wake_up_capabilities_report_payload_t &p) {
            queue_event(device_interviewer_external_event_t::WAKE_UP_CAPABILITIES_REPORT_RECEIVED, p, p.device_endpoint_node);
            return SL_STATUS_OK;
        });

        // Wake Up Interval Set (interview) resolution completed — unblocks WakeUpStep before Interval Get / Report
        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_interval_set_interview_resolution_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_SET_INTERVIEW_RESOLUTION_COMPLETED,
                                                                                                                                                  [](const command_class_wake_up_types::wake_up_interval_set_interview_resolution_payload_t &p) {
                                                                                                                                                      queue_event(device_interviewer_external_event_t::WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED, p, p.device_endpoint_node);
                                                                                                                                                      return SL_STATUS_OK;
                                                                                                                                                  });

        // Wake Up Interval Report
        connector.connect_typed<command_class_wake_up_events_t, command_class_wake_up_types::wake_up_interval_report_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_REPORT_RECEIVED, [](const command_class_wake_up_types::wake_up_interval_report_payload_t &p) {
            queue_event(device_interviewer_external_event_t::WAKE_UP_INTERVAL_REPORT_RECEIVED, p, p.device_endpoint_node);
            return SL_STATUS_OK;
        });

        // Association Report (per-group association members)
        connector.connect_typed<command_class_association_events_t, component_connector_association_report_payload_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_REPORT_RECEIVED, [](const component_connector_association_report_payload_t &p) {
            queue_event(device_interviewer_external_event_t::ASSOCIATION_REPORT_RECEIVED, p, p.endpoint_node);
            return SL_STATUS_OK;
        });

        // Multi Channel Association Report (per-group association members)
        connector.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_report_payload_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED,
                                                                                                                                                  [](const component_connector_multi_channel_association_report_payload_t &p) {
                                                                                                                                                      queue_event(device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED, p, p.endpoint_node);
                                                                                                                                                      return SL_STATUS_OK;
                                                                                                                                                  });

        // Association Group Info reports (from Association Group Info CC)
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_groupings_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT, [](const component_connector_agi_groupings_payload_t &p) {
            queue_event(device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT_RECEIVED, p, p.device_endpoint_node);
            return SL_STATUS_OK;
        });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_groupings_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_INFO_REPORT, [](const component_connector_agi_groupings_payload_t &p) {
            queue_event(device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_INFO_REPORT_RECEIVED, p, p.device_endpoint_node);
            return SL_STATUS_OK;
        });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_groupings_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT, [](const component_connector_agi_groupings_payload_t &p) {
            queue_event(device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT_RECEIVED, p, p.device_endpoint_node);
            return SL_STATUS_OK;
        });

        // ============================================================================
        // Synchronous event handlers (don't need state machine)
        // ============================================================================

        // GET_NODE_INFORMATION: Simple read operation, handled synchronously
        connector.connect_typed<device_interviewer_events_t, device_interviewer_get_node_information_payload_t, device_interviewer_get_node_information_payload_t>(
          device_interviewer_events_t::DEVICE_INTERVIEWER_GET_NODE_INFORMATION,
          [](const device_interviewer_get_node_information_payload_t &p, device_interviewer_get_node_information_payload_t &r) { return zwave_command_class::device_interviewer::on_get_node_information(p, r); });
    }

    sl_status_t device_interviewer::on_get_node_information(const device_interviewer_get_node_information_payload_t &payload_struct, device_interviewer_get_node_information_payload_t &result_struct)
    {
        result_struct.device_node = payload_struct.device_node;

        auto endpoint_0_node = payload_struct.device_node.child_by_type(ATTRIBUTE_ENDPOINT_ID);

        auto node_information_group_node = endpoint_0_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP));
        auto listening_protocol_node     = node_information_group_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::listening_protocol));
        auto optional_protocol_node      = node_information_group_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::optional_protocol));
        auto basic_device_class_node     = node_information_group_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::basic_device_class));
        auto generic_device_class_node   = node_information_group_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::generic_device_class));
        auto specific_device_class_node  = node_information_group_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::specific_device_class));
        auto command_class_list_node     = node_information_group_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::command_class_list));

        result_struct.listening_protocol    = listening_protocol_node.reported_exists() ? std::optional<uint8_t>(listening_protocol_node.reported<uint8_t>()) : std::nullopt;
        result_struct.optional_protocol     = optional_protocol_node.reported_exists() ? std::optional<uint8_t>(optional_protocol_node.reported<uint8_t>()) : std::nullopt;
        result_struct.basic_device_class    = basic_device_class_node.reported_exists() ? std::optional<uint8_t>(basic_device_class_node.reported<uint8_t>()) : std::nullopt;
        result_struct.generic_device_class  = generic_device_class_node.reported_exists() ? std::optional<uint8_t>(generic_device_class_node.reported<uint8_t>()) : std::nullopt;
        result_struct.specific_device_class = specific_device_class_node.reported_exists() ? std::optional<uint8_t>(specific_device_class_node.reported<uint8_t>()) : std::nullopt;
        result_struct.command_class_list    = read_reported_normal_command_class_list(command_class_list_node);

        // S2 command-class list when Security 2 (0x9F) is in the NIF; otherwise omitted.
        constexpr uint8_t security_2_cc = 0x9F;

        if (nif_contains_command_class(result_struct.command_class_list, security_2_cc)) {
            auto s2_group_node                  = endpoint_0_node.child_by_type(static_cast<attribute_store_type_t>(command_class_s2_types::s2_commands_supported_report_group_attributes_t::S2_COMMANDS_SUPPORTED_REPORT_GROUP));
            auto s2_command_class_node          = s2_group_node.child_by_type(static_cast<attribute_store_type_t>(command_class_s2_types::s2_commands_supported_report_group_attributes_t::command_class));
            result_struct.s2_command_class_list = read_reported_normal_command_class_list(s2_command_class_node);
        }

        // S0 command-class list when Security 0 (0x98) is in the NIF; otherwise omitted.
        constexpr uint8_t security_0_cc = 0x98;

        if (nif_contains_command_class(result_struct.command_class_list, security_0_cc)) {
            auto s0_group_node                  = endpoint_0_node.child_by_type(static_cast<attribute_store_type_t>(command_class_s0_types::s0_commands_supported_report_group_attributes_t::S0_COMMANDS_SUPPORTED_REPORT_GROUP));
            auto s0_command_class_node          = s0_group_node.child_by_type(static_cast<attribute_store_type_t>(command_class_s0_types::s0_commands_supported_report_group_attributes_t::command_class));
            result_struct.s0_command_class_list = read_reported_normal_command_class_list(s0_command_class_node);
        }

        auto inclusion_protocol_node = payload_struct.device_node.child_by_type(ATTRIBUTE_ZWAVE_INCLUSION_PROTOCOL);
        if (inclusion_protocol_node.is_valid() && inclusion_protocol_node.reported_exists()) {
            result_struct.inclusion_protocol = inclusion_protocol_node.reported<uint32_t>();
        }

        auto granted_keys_node = payload_struct.device_node.child_by_type(ATTRIBUTE_GRANTED_SECURITY_KEYS);
        if (granted_keys_node.is_valid() && granted_keys_node.reported_exists()) {
            result_struct.granted_keys = granted_keys_node.reported<uint8_t>();
        }

        return SL_STATUS_OK;
    }

    // Initializable interface
    sl_status_t device_interviewer::initialize()
    {
        device_interviewer_mqtt_api.setup_mqtt_api();
        return SL_STATUS_OK;
    }

    int device_interviewer::shutdown()
    {
        stop();
        return 0;
    }

    std::string device_interviewer::name() const
    {
        return "Device Interviewer";
    }

    /**
     * @brief Main event processing loop (runs on device_interviewer thread)
     *
     * This method processes events from the queue on a dedicated thread. Events are
     * queued by external event handlers (registered in register_event_handlers()) and
     * processed here to ensure thread safety.
     *
     * Flow:
     * 1. Pop event from queue (with timeout to reduce CPU usage)
     * 2. If event available: route to state machine for processing
     */
    void device_interviewer::run()
    {
        constexpr uint32_t timeout_ms                            = 50;  // Timeout to reduce CPU usage
        std::optional<device_interviewer_external_event_data> ev = external_event_queue.pop(timeout_ms);

        if (ev.has_value()) {
            // Process event through state machine
            state_machine->process_event(ev.value());
        }

        state_machine->abort_stale_sessions();
    }

}  // namespace zwave_command_class