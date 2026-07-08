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

#include "interview_step_mc_endpoint_get.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_multi_channel_events.hpp"
#include "command_class_multi_channel_types.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool McEndpointGetStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::MULTI_CHANNEL_END_POINT_REPORT_RECEIVED;
    }

    StepResult McEndpointGetStep::on_enter(InterviewSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult McEndpointGetStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_multi_channel_types::command_class_multi_channel_end_point_get_payload_t payload;
            payload.device_endpoint_node = session.endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_GET_INTERVIEW), payload);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_multi_channel_types::command_class_multi_channel_end_point_report_payload_t>(event->payload);

            if (payload.individual_end_points == 0) {
                sl_log_info(LOG_TAG.data(), "Node %d has 0 individual endpoints", session.node_id);
                return skip();
            }

            session.multi_channel.mc_has_dynamic_endpoints = (payload.dynamic != 0);

            if (!session.multi_channel.mc_has_dynamic_endpoints) {
                for (uint8_t i = 1; i <= payload.individual_end_points; ++i) {
                    session.endpoints.endpoint_ids.push_back(i);
                }
                session.endpoints.current_endpoint_it = session.endpoints.endpoint_ids.begin();
                sl_log_info(LOG_TAG.data(), "Node %d: %d static endpoints (1..%d)", session.node_id, payload.individual_end_points, payload.individual_end_points);
            } else {
                sl_log_info(LOG_TAG.data(), "Node %d: dynamic endpoints detected, Endpoint Find required", session.node_id);
            }

            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for MULTI_CHANNEL_END_POINT_REPORT");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
