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

#include "interview_step_basic.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_basic_events.hpp"
#include "command_class_basic_types.hpp"
#include "command_class_version_events.hpp"
#include "command_class_version_types.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store_network_helper.h"
#include "ZW_classcmd.h"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    namespace
    {
        bool endpoint_has_basic_report(const attribute_store::attribute &endpoint_node)
        {
            auto report_group = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(command_class_basic_types::basic_report_group_attributes_t::BASIC_REPORT_GROUP));
            if (!report_group.is_valid()) {
                return false;
            }
            auto current_value = report_group.child_by_type(static_cast<attribute_store_type_t>(command_class_basic_types::basic_report_group_attributes_t::current_value));
            return current_value.is_valid() && current_value.reported_exists();
        }

        attribute_store::attribute basic_version_node(attribute_store::attribute endpoint_node)
        {
            auto device_node = endpoint_node.parent();
            if (device_node.is_valid()) {
                auto ep0 = attribute_store::attribute(attribute_store_get_endpoint_0_node(device_node));
                if (ep0.is_valid()) {
                    return ep0.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
                }
            }
            return endpoint_node.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
        }

        attribute_store::attribute current_basic_endpoint(InterviewSession &session)
        {
            const uint8_t endpoint_id = *session.basic.current_endpoint_it;
            return session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, endpoint_id);
        }

        void heal_basic_version_if_report_present(attribute_store::attribute endpoint_node)
        {
            if (!endpoint_has_basic_report(endpoint_node)) {
                return;
            }
            auto version_node = basic_version_node(endpoint_node);
            if (!version_node.reported_exists() || version_node.reported<uint8_t>() == 0) {
                sl_log_debug(LOG_TAG.data(), "Basic Report present with version 0; establishing version 1");
                version_node.set_reported<uint8_t>(1);
            }
        }

        void fire_basic_get(attribute_store::attribute endpoint_node)
        {
            component_connector connector;
            command_class_basic_types::basic_get_interview_payload_t payload;
            payload.device_endpoint_node = endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET_INTERVIEW), payload);
        }

        void fire_basic_version_get(attribute_store::attribute endpoint_node)
        {
            auto version_node = basic_version_node(endpoint_node);
            if (!version_node.reported_exists() || version_node.reported<uint8_t>() == 0) {
                version_node.set_reported<uint8_t>(1);
                sl_log_debug(LOG_TAG.data(), "Basic Report received; establishing Basic CC version 1");
            }

            command_class_version_types::command_class_version_cc_get_payload_t payload_map_version;
            payload_map_version.device_endpoint_node   = endpoint_node;
            payload_map_version.command_class          = COMMAND_CLASS_BASIC;
            payload_map_version.is_first_command_class = false;
            payload_map_version.retry_count            = 2;

            component_connector connector;
            connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CC_GET), payload_map_version);
        }

    }  // namespace

    bool BasicInterviewStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::BASIC_REPORT_RECEIVED || event_type == device_interviewer_external_event_t::BASIC_GET_RESOLUTION_GIVE_UP || event_type == device_interviewer_external_event_t::VERSION_CC_GET_REQUESTED;
    }

    StepResult BasicInterviewStep::on_enter(InterviewSession &session)
    {
        session.basic.endpoint_ids.clear();
        session.basic.endpoint_ids.push_back(0);
        for (const auto endpoint_id: session.endpoints.endpoint_ids) {
            if (endpoint_id != 0) {
                session.basic.endpoint_ids.push_back(endpoint_id);
            }
        }
        session.basic.current_endpoint_it = session.basic.endpoint_ids.begin();
        session.basic.phase               = BasicProgress::Phase::PendingKick;
        return stay();
    }

    StepResult BasicInterviewStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            if (session.basic.phase != BasicProgress::Phase::PendingKick) {
                return stay();
            }
            if (session.basic.current_endpoint_it == session.basic.endpoint_ids.end()) {
                return done();
            }
            auto endpoint_node = current_basic_endpoint(session);
            heal_basic_version_if_report_present(endpoint_node);
            fire_basic_get(endpoint_node);
            session.basic.phase = BasicProgress::Phase::AwaitingReport;
            sl_log_info(LOG_TAG.data(), "Node %d: Basic Get probe on endpoint %u", session.node_id, static_cast<unsigned>(*session.basic.current_endpoint_it));
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::BASIC_REPORT_RECEIVED) {
            if (session.basic.phase != BasicProgress::Phase::AwaitingReport) {
                return stay();
            }
            try {
                const auto &payload = std::any_cast<command_class_basic_types::basic_report_received_payload_t>(event->payload);
                if (payload.device_endpoint_node != current_basic_endpoint(session)) {
                    return stay();
                }
            } catch (const std::bad_any_cast &) {
                sl_log_error(LOG_TAG.data(), "Invalid payload type for BASIC_REPORT_RECEIVED");
                return stay(SL_STATUS_FAIL);
            }

            fire_basic_version_get(current_basic_endpoint(session));
            session.basic.phase = BasicProgress::Phase::AwaitingVersion;
            sl_log_info(LOG_TAG.data(), "Node %d: Basic Report on endpoint %u; Version Get for 0x20", session.node_id, static_cast<unsigned>(*session.basic.current_endpoint_it));
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::BASIC_GET_RESOLUTION_GIVE_UP) {
            if (session.basic.phase != BasicProgress::Phase::AwaitingReport) {
                return stay();
            }
            try {
                const auto &payload = std::any_cast<command_class_basic_types::basic_get_resolution_give_up_payload_t>(event->payload);
                if (payload.device_endpoint_node != current_basic_endpoint(session)) {
                    return stay();
                }
            } catch (const std::bad_any_cast &) {
                sl_log_error(LOG_TAG.data(), "Invalid payload type for BASIC_GET_RESOLUTION_GIVE_UP");
                return stay(SL_STATUS_FAIL);
            }

            auto endpoint_node = current_basic_endpoint(session);
            endpoint_node.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC)).set_reported<uint8_t>(0);
            sl_log_info(LOG_TAG.data(), "Node %d: Basic Get exhausted on endpoint %u; leaving unsupported", session.node_id, static_cast<unsigned>(*session.basic.current_endpoint_it));

            ++session.basic.current_endpoint_it;
            if (session.basic.current_endpoint_it == session.basic.endpoint_ids.end()) {
                return done();
            }
            session.basic.phase = BasicProgress::Phase::PendingKick;
            heal_basic_version_if_report_present(current_basic_endpoint(session));
            fire_basic_get(current_basic_endpoint(session));
            session.basic.phase = BasicProgress::Phase::AwaitingReport;
            sl_log_info(LOG_TAG.data(), "Node %d: Basic Get probe on endpoint %u", session.node_id, static_cast<unsigned>(*session.basic.current_endpoint_it));
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::VERSION_CC_GET_REQUESTED) {
            if (session.basic.phase != BasicProgress::Phase::AwaitingVersion) {
                return stay();
            }
            try {
                const auto &payload = std::any_cast<command_class_version_types::command_class_version_cc_get_payload_t>(event->payload);
                if (payload.command_class != COMMAND_CLASS_BASIC) {
                    return stay();
                }
            } catch (const std::bad_any_cast &) {
                sl_log_error(LOG_TAG.data(), "Invalid payload type for VERSION_CC_GET_REQUESTED during Basic interview");
                return stay(SL_STATUS_FAIL);
            }

            sl_log_info(LOG_TAG.data(), "Node %d: Basic Version Report for endpoint %u", session.node_id, static_cast<unsigned>(*session.basic.current_endpoint_it));
            ++session.basic.current_endpoint_it;
            if (session.basic.current_endpoint_it == session.basic.endpoint_ids.end()) {
                return done();
            }
            session.basic.phase = BasicProgress::Phase::PendingKick;
            heal_basic_version_if_report_present(current_basic_endpoint(session));
            fire_basic_get(current_basic_endpoint(session));
            session.basic.phase = BasicProgress::Phase::AwaitingReport;
            sl_log_info(LOG_TAG.data(), "Node %d: Basic Get probe on endpoint %u", session.node_id, static_cast<unsigned>(*session.basic.current_endpoint_it));
            return stay();
        }

        return stay();
    }

}  // namespace zwave_command_class
