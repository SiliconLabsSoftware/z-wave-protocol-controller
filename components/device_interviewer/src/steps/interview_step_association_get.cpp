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

#include "interview_step_association_get.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "component_connector_types.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_types.hpp"
#include "log.h"
#include <future>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool AssociationGetStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::ASSOCIATION_REPORT_RECEIVED || event_type == device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED;
    }

    StepResult AssociationGetStep::on_enter(InterviewSession &session)
    {
        if (session.agi.agi_total_groups == 0) {
            sl_log_debug(LOG_TAG.data(), "Node %d has 0 association groups, skipping Association Get", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult AssociationGetStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            session.association_members.assoc_current_group_id = 1;

            component_connector connector;
            if (session.agi.agi_used_multi_channel) {
                component_connector_multi_channel_association_get_payload_t payload;
                payload.endpoint_node       = session.endpoint_node;
                payload.grouping_identifier = session.association_members.assoc_current_group_id;
                connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GET), payload);
            } else {
                component_connector_association_get_payload_t payload;
                payload.endpoint_node       = session.endpoint_node;
                payload.grouping_identifier = session.association_members.assoc_current_group_id;
                connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GET), payload);
            }
            return stay();
        }

        if (session.association_members.assoc_current_group_id >= session.agi.agi_total_groups) {
            sl_log_info(LOG_TAG.data(), "Node %d: all %d association groups read", session.node_id, session.agi.agi_total_groups);
            return done();
        }

        session.association_members.assoc_current_group_id++;

        component_connector connector;
        if (session.agi.agi_used_multi_channel) {
            component_connector_multi_channel_association_get_payload_t payload;
            payload.endpoint_node       = session.endpoint_node;
            payload.grouping_identifier = session.association_members.assoc_current_group_id;
            connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GET), payload);
        } else {
            component_connector_association_get_payload_t payload;
            payload.endpoint_node       = session.endpoint_node;
            payload.grouping_identifier = session.association_members.assoc_current_group_id;
            connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GET), payload);
        }

        return stay();
    }

}  // namespace zwave_command_class
