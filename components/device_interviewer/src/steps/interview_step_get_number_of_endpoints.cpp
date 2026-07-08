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

#include "interview_step_get_number_of_endpoints.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_multi_channel_types.hpp"
#include "command_class_multi_channel_events.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool GetNumberOfEndpointsStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::MULTI_CHANNEL_END_POINT_FIND_REPORT_RECEIVED;
    }

    StepResult GetNumberOfEndpointsStep::on_enter(InterviewSession &session)
    {
        if (!session.endpoints.endpoint_ids.empty()) {
            sl_log_info(LOG_TAG.data(), "Node %d: endpoint IDs already populated (%zu endpoints), skipping Endpoint Find", session.node_id, session.endpoints.endpoint_ids.size());
            return skip();
        }
        return stay();
    }

    StepResult GetNumberOfEndpointsStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_multi_channel_types::command_class_multi_channel_end_point_find_payload_t payload_map;
            payload_map.device_endpoint_node = session.endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_END_POINT_FIND), payload_map);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_multi_channel_types::command_class_multi_channel_end_point_find_report_payload_t>(event->payload);

            if (payload.endpoints.empty()) {
                sl_log_info(LOG_TAG.data(), "No endpoints found for node %d", session.node_id, session.endpoint_id);
                return skip();
            }
            sl_log_info(LOG_TAG.data(), "%d endpoints found for node %d", payload.endpoints.size(), session.node_id, session.endpoint_id);
            sl_log_info(LOG_TAG.data(), "Endpoints:");
            for (const auto &endpoint: payload.endpoints) {
                sl_log_info(LOG_TAG.data(), "\t- Endpoint ID: %d", endpoint.properties1.value);
                session.endpoints.endpoint_ids.push_back(endpoint.properties1.value);
            }
            session.endpoints.current_endpoint_it = session.endpoints.endpoint_ids.begin();
            return done();

        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for MULTI_CHANNEL_END_POINT_FIND_REPORT");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
