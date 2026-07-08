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

#include "interview_step_endpoint_association_iterator.hpp"
#include "interview_state_machine.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool EndpointAssociationIteratorStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult EndpointAssociationIteratorStep::on_enter(InterviewSession &session)
    {
        if (session.endpoints.endpoint_ids.empty()) {
            sl_log_info(LOG_TAG.data(), "Node %d: no endpoints, skipping per-endpoint association/AGI", session.node_id);
            return skip();
        }

        if (session.endpoint_id != 0) {
            // We just finished association/AGI/lifeline for an endpoint (came from POST_VALIDATE_LIFELINE)
            ++session.endpoints.current_endpoint_it;
            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end()) {
                session.endpoint_node = session.root_endpoint_node;
                session.endpoint_id   = 0;
                sl_log_info(LOG_TAG.data(), "Node %d: per-endpoint association/AGI completed for all endpoints", session.node_id);
                return skip();
            }
        } else {
            // First time (from ENDPOINT_ZWAVEPLUS_INFO): start with first endpoint
            session.endpoints.current_endpoint_it = session.endpoints.endpoint_ids.begin();
        }

        session.endpoint_node                              = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);
        session.endpoint_id                                = *session.endpoints.current_endpoint_it;
        session.agi.agi_current_group_id                   = 0;
        session.association_members.assoc_current_group_id = 0;

        sl_log_info(LOG_TAG.data(), "Node %d: starting association/AGI interview for endpoint %d", session.node_id, session.endpoint_id);
        return done();
    }

    StepResult EndpointAssociationIteratorStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace zwave_command_class
