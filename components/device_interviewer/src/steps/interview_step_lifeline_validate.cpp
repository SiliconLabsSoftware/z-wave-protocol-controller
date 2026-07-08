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

#include "interview_step_lifeline_validate.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_types.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    static constexpr uint8_t LIFELINE_GROUPING_IDENTIFIER = 1;

    bool LifelineValidateStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::ASSOCIATION_REPORT_RECEIVED || event_type == device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED;
    }

    StepResult LifelineValidateStep::on_enter(InterviewSession &session)
    {
        // TODO: This is a temporary workaround to ensure the SET is processed before the GET.
        // We should wait for the SET to be processed instead of sleeping.
        // To do that we need to refactor the resolver component
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return stay();
    }

    StepResult LifelineValidateStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;

            if (session.agi.agi_used_multi_channel) {
                component_connector_multi_channel_association_get_payload_t payload;
                payload.endpoint_node       = session.endpoint_node;
                payload.grouping_identifier = LIFELINE_GROUPING_IDENTIFIER;
                connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GET), payload);
                sl_log_info(LOG_TAG.data(), "Node %d: requesting Multi Channel Association Get (Group 1) to validate lifeline", session.node_id);
            } else {
                component_connector_association_get_payload_t payload;
                payload.endpoint_node       = session.endpoint_node;
                payload.grouping_identifier = LIFELINE_GROUPING_IDENTIFIER;
                connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GET), payload);
                sl_log_info(LOG_TAG.data(), "Node %d: requesting Association Get (Group 1) to validate lifeline", session.node_id);
            }
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::ASSOCIATION_REPORT_RECEIVED) {
            const auto &payload = std::any_cast<const component_connector_association_report_payload_t &>(event->payload);
            if (payload.endpoint_node != session.endpoint_node || payload.grouping_identifier != LIFELINE_GROUPING_IDENTIFIER) {
                sl_log_warning(LOG_TAG.data(), "Node %d: lifeline (Group 1) Association Report received but endpoint or grouping identifier does not match", session.node_id);
                return stay();
            }
            sl_log_info(LOG_TAG.data(), "Node %d: lifeline (Group 1) Association Report received", session.node_id);
            return done();
        }
        if (event->event == device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED) {
            const auto &payload = std::any_cast<const component_connector_multi_channel_association_report_payload_t &>(event->payload);
            if (payload.endpoint_node != session.endpoint_node || payload.grouping_identifier != LIFELINE_GROUPING_IDENTIFIER) {
                return stay();
            }
            sl_log_info(LOG_TAG.data(), "Node %d: lifeline (Group 1) Multi Channel Association Report received", session.node_id);
            return done();
        }
        sl_log_warning(LOG_TAG.data(), "Node %d: unexpected event received", session.node_id);
        return stay();
    }

}  // namespace zwave_command_class
