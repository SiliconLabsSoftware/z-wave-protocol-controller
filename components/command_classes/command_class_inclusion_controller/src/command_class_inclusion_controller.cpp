
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

#include <array>
#include <string_view>
#include <variant>

#include "command_class_inclusion_controller.hpp"

#include "ZW_classcmd.h"

#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "log.h"
#include "zwapi_protocol_controller.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_controller_internal.h"
#include "zwave_network_management.h"
#include "zwave_tx_scheme_selector.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_inclusion_controller";

    namespace
    {
        bool nif_advertises_command_class(const zwave_node_info_t &nif, zwave_command_class_t command_class)
        {
            for (uint8_t i = 0; i < nif.command_class_list_length; ++i) {
                if (nif.command_class_list[i] == command_class) {
                    return true;
                }
            }
            return false;
        }
    }  // namespace

    struct timer_handle_t command_class_inclusion_controller::timer                           = {nullptr};
    command_class_inclusion_controller::session_t command_class_inclusion_controller::session = {
      session_state_t::idle,
      0,
      0,
      0,
    };

    struct timer_handle_t command_class_inclusion_controller::deferred_interview_timer                              = {nullptr};
    command_class_inclusion_controller::deferred_interview_t command_class_inclusion_controller::deferred_interview = {false, 0};

    command_class_inclusion_controller::command_class_inclusion_controller()
    {
        component_connector connector;

        connector.connect_typed<component_connector_common_events_t, component_connector_node_information_received_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_INFORMATION_RECEIVED,
                                                                                                                              [](const component_connector_node_information_received_payload_t &payload) { return on_node_information_received(payload); });

        connector.connect_typed<component_connector_common_events_t, component_connector_node_added_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_ADDED, [](const component_connector_node_added_payload_t &payload) { return on_node_added(payload); });

        // Arbitrate handoff vs. fallback for nodes assigned by another controller.
        connector.connect_typed<component_connector_common_events_t, component_connector_node_id_assigned_by_other_controller_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_ID_ASSIGNED_BY_OTHER_CONTROLLER,
                                                                                                                                         [](const component_connector_node_id_assigned_by_other_controller_payload_t &payload) { return on_node_id_assigned_by_other_controller(payload); });

        // Drop a pending fallback if the node is excluded before the grace expires.
        connector.connect_typed<component_connector_common_events_t, component_connector_node_deleted_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_DELETED, [](const component_connector_node_deleted_payload_t &payload) { return on_node_deleted(payload); });
    }

    sl_status_t command_class_inclusion_controller::on_initiate_parsed(const zwave_controller_connection_info_t *connection_info, [[maybe_unused]] attribute_store::attribute endpoint, command_class_inclusion_controller_attribute_map_t payload)
    {
        // CC:0074.01.01.11.002: ignore multicast.
        if (connection_info == nullptr || connection_info->local.is_multicast) {
            sl_log_debug(LOG_TAG.data(), "Ignoring INITIATE received via multicast addressing.");
            return SL_STATUS_OK;
        }

        const zwave_node_id_t inclusion_controller_node_id = connection_info->remote.node_id;
        const auto step_id                                 = static_cast<uint8_t>(std::get<uint8_t>(payload.at("step_id")));
        const auto included_node_id                        = static_cast<zwave_node_id_t>(std::get<uint8_t>(payload.at("node_id")));

        // The SIS only ever receives INITIATE_PROXY_INCLUSION / INITIATE_PROXY_INCLUSION_REPLACE.
        // Reserved values are silently ignored per CC:0074.01.01.11.004's spec table.
        if (step_id != INITIATE_PROXY_INCLUSION && step_id != INITIATE_PROXY_INCLUSION_REPLACE) {
            sl_log_debug(LOG_TAG.data(), "Ignoring INITIATE with unsupported Step ID 0x%02X from NodeID %u", step_id, inclusion_controller_node_id);
            return SL_STATUS_OK;
        }

        if (session.state != session_state_t::idle) {
            sl_log_warning(LOG_TAG.data(), "INITIATE received from NodeID %u while another inclusion-controller session is in flight. Replying STEP_FAILED.", inclusion_controller_node_id);
            send_complete(inclusion_controller_node_id, step_id, COMPLETE_STEP_FAILED);
            return SL_STATUS_OK;
        }

        if (zwave_network_management_is_busy()) {
            sl_log_warning(LOG_TAG.data(), "INITIATE received from NodeID %u while network management is busy. Replying STEP_FAILED.", inclusion_controller_node_id);
            send_complete(inclusion_controller_node_id, step_id, COMPLETE_STEP_FAILED);
            return SL_STATUS_OK;
        }

        sl_log_info(LOG_TAG.data(), "INITIATE from NodeID %u for new NodeID %u, step 0x%02X. Requesting NIF.", inclusion_controller_node_id, included_node_id, step_id);

        // The IC is handing this node off to us; the proxy/S0 path will produce a real NODE_ADDED.
        // Drop any pending fallback for the same node so we don't double-trigger an interview.
        cancel_deferred_interview(included_node_id);

        session.inclusion_controller_node_id = inclusion_controller_node_id;
        session.included_node_id             = included_node_id;
        session.step_id                      = step_id;
        session.state                        = session_state_t::waiting_nif;

        timer_set(&timer, SESSION_TIMEOUT_MS, on_session_timeout, nullptr);

        if (zwapi_request_node_info(included_node_id) != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG.data(), "zwapi_request_node_info failed for NodeID %u. Replying STEP_FAILED.", included_node_id);
            finish_session_failed();
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_inclusion_controller::on_complete_parsed(const zwave_controller_connection_info_t *connection_info, [[maybe_unused]] attribute_store::attribute endpoint, command_class_inclusion_controller_attribute_map_t payload)
    {
        // CC:0074.01.02.11.002: ignore multicast.
        if (connection_info == nullptr || connection_info->local.is_multicast) {
            sl_log_debug(LOG_TAG.data(), "Ignoring COMPLETE received via multicast addressing.");
            return SL_STATUS_OK;
        }

        // The only COMPLETE we expect is the inclusion controller's reply to our delegated
        // INITIATE (Step ID = S0_INCLUSION). Anything else is unsolicited and ignored.
        if (session.state != session_state_t::waiting_ic_complete) {
            sl_log_debug(LOG_TAG.data(), "Ignoring unsolicited COMPLETE from NodeID %u (no delegation in flight).", connection_info->remote.node_id);
            return SL_STATUS_OK;
        }

        if (connection_info->remote.node_id != session.inclusion_controller_node_id) {
            sl_log_debug(LOG_TAG.data(), "Ignoring COMPLETE from NodeID %u; expected the inclusion controller (NodeID %u).", connection_info->remote.node_id, session.inclusion_controller_node_id);
            return SL_STATUS_OK;
        }

        const auto received_step_id = static_cast<uint8_t>(std::get<uint8_t>(payload.at("step_id")));
        const auto received_status  = static_cast<uint8_t>(std::get<uint8_t>(payload.at("status")));

        if (received_step_id != COMPLETE_S0_INCLUSION) {
            sl_log_debug(LOG_TAG.data(), "Ignoring COMPLETE with Step ID 0x%02X while waiting for COMPLETE_S0_INCLUSION.", received_step_id);
            return SL_STATUS_OK;
        }

        // Forward the inclusion controller's outcome to the original INITIATE (proxy or replace).
        // Any non-OK status from the controller maps to STEP_FAILED in our reply.
        const uint8_t outgoing_status = (received_status == COMPLETE_STEP_OK) ? COMPLETE_STEP_OK : COMPLETE_STEP_FAILED;
        sl_log_info(LOG_TAG.data(), "S0 delegation done by NodeID %u (status=0x%02X). Replying COMPLETE step 0x%02X with status 0x%02X.", connection_info->remote.node_id, received_status, session.step_id, outgoing_status);

        const zwave_node_id_t target     = session.inclusion_controller_node_id;
        const uint8_t step_id            = session.step_id;
        const zwave_node_id_t s0_node_id = session.included_node_id;
        zwave_node_info_t s0_nif         = session.node_info;
        clear_session();
        send_complete(target, step_id, outgoing_status);

        if (outgoing_status == COMPLETE_STEP_OK) {
            zwapi_node_info_header_t ni = {};
            if (zwapi_get_protocol_info(s0_node_id, &ni) == SL_STATUS_OK) {
                s0_nif.listening_protocol = ni.capability;
                s0_nif.optional_protocol  = ni.security;
            }
            zwave_dsk_t empty_dsk = {};
            zwave_controller_on_node_added(SL_STATUS_OK, &s0_nif, s0_node_id, empty_dsk, ZWAVE_CONTROLLER_S0_KEY, ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE, PROTOCOL_ZWAVE);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_inclusion_controller::on_node_information_received(const component_connector_node_information_received_payload_t &payload)
    {
        if (session.state != session_state_t::waiting_nif || payload.node_id != session.included_node_id) {
            return SL_STATUS_OK;
        }

        // CC:0074.01.01.11.005: branch on the joining node's security capability.
        // - S2 (0x9F): ZPC drives proxy inclusion (S2 bootstrapping) itself.
        // - S0 (0x98) only: ZPC delegates S0 bootstrapping back to the inclusion
        //   controller via INITIATE_S0_INCLUSION; ZPC must NOT run S0 itself.
        // - Neither: nothing to bootstrap, acknowledge immediately.
        if (nif_advertises_command_class(payload.node_info, COMMAND_CLASS_SECURITY_2)) {
            sl_log_info(LOG_TAG.data(), "NIF received for NodeID %u (S2-capable), starting proxy inclusion (step 0x%02X).", payload.node_id, session.step_id);

            const sl_status_t status = zwave_network_management_start_proxy_inclusion(payload.node_id, payload.node_info, session.step_id);
            if (status != SL_STATUS_OK) {
                sl_log_warning(LOG_TAG.data(), "zwave_network_management_start_proxy_inclusion failed (status=0x%04X). Replying STEP_FAILED.", status);
                finish_session_failed();
                return SL_STATUS_OK;
            }

            session.state = session_state_t::waiting_node_added;
            timer_set(&timer, SESSION_TIMEOUT_MS, on_session_timeout, nullptr);
            return SL_STATUS_OK;
        }

        if (nif_advertises_command_class(payload.node_info, COMMAND_CLASS_SECURITY)) {
            sl_log_info(LOG_TAG.data(), "NIF received for NodeID %u (S0-only), delegating S0 inclusion to NodeID %u.", payload.node_id, session.inclusion_controller_node_id);

            session.node_info = payload.node_info;
            session.state     = session_state_t::waiting_ic_complete;
            timer_set(&timer, SESSION_TIMEOUT_MS, on_session_timeout, nullptr);
            send_initiate(session.inclusion_controller_node_id, session.included_node_id, INITIATE_S0_INCLUSION);
            return SL_STATUS_OK;
        }

        sl_log_info(LOG_TAG.data(), "NIF received for NodeID %u (no security), replying COMPLETE STEP_OK to NodeID %u.", payload.node_id, session.inclusion_controller_node_id);
        const zwave_node_id_t target = session.inclusion_controller_node_id;
        const uint8_t step_id        = session.step_id;
        clear_session();
        send_complete(target, step_id, COMPLETE_STEP_OK);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_inclusion_controller::on_node_added(const component_connector_node_added_payload_t &payload)
    {
        if (session.state != session_state_t::waiting_node_added || payload.node_id != session.included_node_id) {
            return SL_STATUS_OK;
        }

        if (payload.status != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG.data(), "NODE_ADDED for NodeID %u reported failure (0x%04X). Replying STEP_FAILED.", payload.node_id, payload.status);
            finish_session_failed();
            return SL_STATUS_OK;
        }

        // CC:0074.01.02.11.004: the SIS's "device probe" requirement is met by the NIF
        // request that the proxy-inclusion path already performed. The deeper attribute-store
        // interview corresponds to spec step 10 ("SHOULD perform any probing"), runs in
        // parallel via the device_interviewer, and must not delay the COMPLETE: the
        // inclusion controller has its own timeout for receiving COMPLETE after S2
        // bootstrapping finishes (CC:0074.01.02.11.001).
        sl_log_info(LOG_TAG.data(), "NodeID %u added through proxy inclusion. Sending COMPLETE STEP_OK to NodeID %u.", payload.node_id, session.inclusion_controller_node_id);
        const zwave_node_id_t target = session.inclusion_controller_node_id;
        const uint8_t step_id        = session.step_id;
        clear_session();
        send_complete(target, step_id, COMPLETE_STEP_OK);
        return SL_STATUS_OK;
    }

    void command_class_inclusion_controller::on_session_timeout([[maybe_unused]] void *data)
    {
        if (session.state == session_state_t::idle) {
            return;
        }
        sl_log_warning(LOG_TAG.data(), "Inclusion-controller session for NodeID %u timed out in state %u. Replying STEP_FAILED.", session.included_node_id, static_cast<unsigned>(session.state));
        finish_session_failed();
    }

    void command_class_inclusion_controller::send_complete(zwave_node_id_t inclusion_controller_node_id, uint8_t step_id, uint8_t status)
    {
        zwave_controller_connection_info_t connection = {};
        zwave_tx_scheme_get_node_connection_info(inclusion_controller_node_id, 0, &connection);

        const std::array<uint8_t, 4> frame = {COMMAND_CLASS_INCLUSION_CONTROLLER, COMPLETE, step_id, status};
        const sl_status_t tx_status        = command_class_utils::send_report(&connection, static_cast<uint16_t>(frame.size()), frame.data());
        if (tx_status != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG.data(), "Failed to send Inclusion Controller COMPLETE to NodeID %u (status=0x%04X).", inclusion_controller_node_id, tx_status);
        }
    }

    void command_class_inclusion_controller::send_initiate(zwave_node_id_t inclusion_controller_node_id, zwave_node_id_t included_node_id, uint8_t step_id)
    {
        zwave_controller_connection_info_t connection = {};
        zwave_tx_scheme_get_node_connection_info(inclusion_controller_node_id, 0, &connection);

        const std::array<uint8_t, 4> frame = {COMMAND_CLASS_INCLUSION_CONTROLLER, INITIATE, static_cast<uint8_t>(included_node_id), step_id};
        const sl_status_t tx_status        = command_class_utils::send_report(&connection, static_cast<uint16_t>(frame.size()), frame.data());
        if (tx_status != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG.data(), "Failed to send Inclusion Controller INITIATE to NodeID %u (status=0x%04X).", inclusion_controller_node_id, tx_status);
        }
    }

    void command_class_inclusion_controller::finish_session_failed()
    {
        const zwave_node_id_t target = session.inclusion_controller_node_id;
        const uint8_t step_id        = session.step_id;
        clear_session();
        send_complete(target, step_id, COMPLETE_STEP_FAILED);
    }

    void command_class_inclusion_controller::clear_session()
    {
        timer_stop(&timer);
        session = {session_state_t::idle, 0, 0, 0};
    }

    sl_status_t command_class_inclusion_controller::on_node_id_assigned_by_other_controller(const component_connector_node_id_assigned_by_other_controller_payload_t &payload)
    {
        // We don't yet know if the originating controller will hand off via INITIATE.
        // Arm a grace timer; INITIATE cancels it, otherwise we fall back to interview-request.
        arm_deferred_interview(payload.node_id);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_inclusion_controller::on_node_deleted(const component_connector_node_deleted_payload_t &payload)
    {
        cancel_deferred_interview(payload.node_id);
        return SL_STATUS_OK;
    }

    void command_class_inclusion_controller::arm_deferred_interview(zwave_node_id_t node_id)
    {
        deferred_interview = {true, node_id};
        timer_set(&deferred_interview_timer, HANDOFF_GRACE_MS, on_deferred_interview_timeout, nullptr);
        sl_log_debug(LOG_TAG.data(), "Node %u: arming inclusion-handoff grace timer (%u ms).", node_id, static_cast<unsigned>(HANDOFF_GRACE_MS));
    }

    void command_class_inclusion_controller::cancel_deferred_interview(zwave_node_id_t node_id)
    {
        if (!deferred_interview.armed || deferred_interview.node_id != node_id) {
            return;
        }
        sl_log_debug(LOG_TAG.data(), "Node %u: cancelling inclusion-handoff grace timer (real lifecycle event arrived).", node_id);
        timer_stop(&deferred_interview_timer);
        deferred_interview = {false, 0};
    }

    void command_class_inclusion_controller::on_deferred_interview_timeout([[maybe_unused]] void *data)
    {
        if (!deferred_interview.armed) {
            return;
        }
        const zwave_node_id_t node_id = deferred_interview.node_id;
        deferred_interview            = {false, 0};
        sl_log_info(LOG_TAG.data(), "Node %u: no INITIATE within grace window, requesting interview.", node_id);
        fire_node_interview_requested(node_id);
    }

    void command_class_inclusion_controller::fire_node_interview_requested(zwave_node_id_t node_id)
    {
        component_connector_node_interview_requested_payload_t payload;
        payload.node_id = node_id;
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_INTERVIEW_REQUESTED), payload);
    }

}  // namespace zwave_command_class
