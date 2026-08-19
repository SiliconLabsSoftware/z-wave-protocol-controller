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

#include "interview_step_wake_up.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_wake_up_events.hpp"
#include "command_class_wake_up_types.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "zpc_config.h"
#include "zwave_network_management.h"
#include "zwave_command_class_utils.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    static uint8_t read_wake_up_command_class_version(const attribute_store::attribute &endpoint_node)
    {
        auto version_node = endpoint_node.child_by_type(ATTRIBUTE_COMMAND_CLASS_WAKE_UP_VERSION);
        if (version_node.is_valid() && version_node.reported_exists()) {
            const uint8_t v = version_node.reported<uint8_t>();
            return (v > 0) ? v : 1;
        }
        return 1;
    }

    static uint32_t wake_up_interval_seconds_from_zpc_config()
    {
        const zpc_config_t *cfg = zpc_get_config();
        if (cfg == nullptr || cfg->default_wake_up_interval < 0) {
            return 0;
        }
        return static_cast<uint32_t>(cfg->default_wake_up_interval);
    }

    static void fire_wake_up_interval_set(InterviewSession &session)
    {
        component_connector connector;
        command_class_wake_up_types::wake_up_interval_set_payload_t set_payload;
        set_payload.device_endpoint_node = session.endpoint_node;
        set_payload.interval             = wake_up_interval_seconds_from_zpc_config();
        set_payload.node_id              = zwave_network_management_get_node_id();
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_SET), set_payload);
    }

    bool WakeUpStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::WAKE_UP_CAPABILITIES_REPORT_RECEIVED || event_type == device_interviewer_external_event_t::WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED || event_type == device_interviewer_external_event_t::WAKE_UP_INTERVAL_REPORT_RECEIVED;
    }

    StepResult WakeUpStep::on_enter(InterviewSession &session)
    {
        if (!command_class_utils::is_command_class_in_s2_s0_nif_lists(0x84, session.s2_supported_command_classes, session.s0_supported_command_classes, session.node_information_command_class_list)) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support Wake Up CC (0x84), skipping", session.node_id);
            return skip();
        }

        session.wake_up.phase                 = WakeUpProgress::Phase::PendingKick;
        session.wake_up.command_class_version = read_wake_up_command_class_version(session.endpoint_node);
        return stay();
    }

    StepResult WakeUpStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            if (session.wake_up.phase != WakeUpProgress::Phase::PendingKick) {
                return stay();
            }

            if (session.wake_up.command_class_version >= 2) {
                component_connector connector;
                command_class_wake_up_types::wake_up_capabilities_get_payload_t payload;
                payload.device_endpoint_node = session.endpoint_node;
                connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_CAPABILITIES_GET_INTERVIEW), payload);
                session.wake_up.phase = WakeUpProgress::Phase::AwaitingCapabilitiesReport;
                sl_log_info(LOG_TAG.data(), "Node %d: Wake Up v%u — Interval Capabilities Get", session.node_id, static_cast<unsigned>(session.wake_up.command_class_version));
                return stay();
            }

            fire_wake_up_interval_set(session);
            session.wake_up.phase = WakeUpProgress::Phase::AwaitingIntervalSetResolution;
            sl_log_info(LOG_TAG.data(), "Node %d: Wake Up v1 — Interval Set (default_wake_up_interval=%u s)", session.node_id, static_cast<unsigned>(wake_up_interval_seconds_from_zpc_config()));
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::WAKE_UP_CAPABILITIES_REPORT_RECEIVED) {
            if (session.wake_up.phase != WakeUpProgress::Phase::AwaitingCapabilitiesReport) {
                return stay();
            }
            try {
                std::any_cast<command_class_wake_up_types::wake_up_capabilities_report_payload_t>(event->payload);
            } catch (const std::bad_any_cast &) {
                sl_log_error(LOG_TAG.data(), "Invalid payload type for WAKE_UP_CAPABILITIES_REPORT_RECEIVED");
                return stay(SL_STATUS_FAIL);
            }

            fire_wake_up_interval_set(session);
            session.wake_up.phase = WakeUpProgress::Phase::AwaitingIntervalSetResolution;
            sl_log_info(LOG_TAG.data(), "Node %d: Wake Up Capabilities Report — Interval Set (default_wake_up_interval=%u s)", session.node_id, static_cast<unsigned>(wake_up_interval_seconds_from_zpc_config()));
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED) {
            if (session.wake_up.phase != WakeUpProgress::Phase::AwaitingIntervalSetResolution) {
                return stay();
            }
            if (!event->device_endpoint_node.has_value() || event->device_endpoint_node.value() != session.endpoint_node) {
                return stay();
            }
            try {
                std::any_cast<command_class_wake_up_types::wake_up_interval_set_interview_resolution_payload_t>(event->payload);
            } catch (const std::bad_any_cast &) {
                sl_log_error(LOG_TAG.data(), "Invalid payload type for WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED");
                return stay(SL_STATUS_FAIL);
            }
            session.wake_up.phase = WakeUpProgress::Phase::AwaitingPostSetIntervalReport;
            sl_log_info(LOG_TAG.data(), "Node %d: Wake Up Interval Set resolution done; awaiting Interval Report after Get", session.node_id);
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::WAKE_UP_INTERVAL_REPORT_RECEIVED) {
            if (session.wake_up.phase != WakeUpProgress::Phase::AwaitingPostSetIntervalReport) {
                return stay();
            }
            sl_log_info(LOG_TAG.data(), "Node %d: Wake Up Interval Report received, interview step complete", session.node_id);
            return done();
        }

        return stay();
    }

}  // namespace zwave_command_class
