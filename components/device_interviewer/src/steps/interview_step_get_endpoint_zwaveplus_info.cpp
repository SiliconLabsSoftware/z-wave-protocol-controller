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

#include "interview_step_get_endpoint_zwaveplus_info.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_zwaveplus_info_events.hpp"
#include "command_class_zwaveplus_info_types.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool GetEndpointZwavePlusInfoStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::ZWAVEPLUS_INFO_REPORT_RECEIVED;
    }

    StepResult GetEndpointZwavePlusInfoStep::on_enter(InterviewSession &session)
    {
        if (session.endpoints.endpoint_ids.empty()) {
            sl_log_info(LOG_TAG.data(), "Node %d: no endpoints, skipping per-endpoint Z-Wave Plus Info", session.node_id);
            return skip();
        }
        if (std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x5E) == session.version_cc.command_classes_to_query.end()) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support Z-Wave Plus Info CC (0x5E), skipping per-endpoint", session.node_id);
            return skip();
        }
        return stay();
    }

    StepResult GetEndpointZwavePlusInfoStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            session.endpoints.current_endpoint_it = session.endpoints.endpoint_ids.begin();
            auto endpoint_node                    = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);

            component_connector connector;
            command_class_zwaveplus_info_types::zwaveplus_info_get_payload_t payload;
            payload.device_endpoint_node = endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_zwaveplus_info_events_t::COMMAND_CLASS_ZWAVEPLUS_INFO_GET_INTERVIEW), payload);
            sl_log_info(LOG_TAG.data(), "Node %d: requesting Z-Wave Plus Info for endpoint %d", session.node_id, *session.endpoints.current_endpoint_it);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_zwaveplus_info_types::zwaveplus_info_report_payload_t>(event->payload);

            if (!payload.device_endpoint_node.is_valid()) {
                sl_log_warning(LOG_TAG.data(), "Node %d: Z-Wave Plus Info report with invalid endpoint node", session.node_id);
                return stay();
            }

            uint8_t report_endpoint_id = 0;
            try {
                report_endpoint_id = payload.device_endpoint_node.reported<uint8_t>();
            } catch (...) {
                sl_log_warning(LOG_TAG.data(), "Node %d: could not read endpoint id from report node", session.node_id);
                return stay();
            }

            if (report_endpoint_id != *session.endpoints.current_endpoint_it) {
                sl_log_debug(LOG_TAG.data(), "Node %d: ignoring Z-Wave Plus Info report for endpoint %d (waiting for %d)", session.node_id, report_endpoint_id, *session.endpoints.current_endpoint_it);
                return stay();
            }

            sl_log_info(LOG_TAG.data(), "Node %d: received Z-Wave Plus Info Report for endpoint %d", session.node_id, *session.endpoints.current_endpoint_it);
            ++session.endpoints.current_endpoint_it;

            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end()) {
                sl_log_info(LOG_TAG.data(), "Node %d: all per-endpoint Z-Wave Plus Info received", session.node_id);
                return done();
            }

            auto endpoint_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);
            component_connector connector;
            command_class_zwaveplus_info_types::zwaveplus_info_get_payload_t payload_get;
            payload_get.device_endpoint_node = endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_zwaveplus_info_events_t::COMMAND_CLASS_ZWAVEPLUS_INFO_GET_INTERVIEW), payload_get);
            sl_log_info(LOG_TAG.data(), "Node %d: requesting Z-Wave Plus Info for endpoint %d", session.node_id, *session.endpoints.current_endpoint_it);
            return stay();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for ZWAVEPLUS_INFO_REPORT_RECEIVED");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
