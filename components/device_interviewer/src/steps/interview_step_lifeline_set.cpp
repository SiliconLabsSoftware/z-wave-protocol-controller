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

#include "interview_step_lifeline_set.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_types.hpp"
#include "command_class_multi_channel_association_constants.hpp"
#include "zwave_network_management.h"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool LifelineSetStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;  // Association Set does not produce a report; validation is a separate step
    }

    StepResult LifelineSetStep::on_enter(InterviewSession &session)
    {
        if (session.agi.agi_total_groups < 1) {
            sl_log_info(LOG_TAG.data(), "Node %d has no association groups, skipping lifeline set", session.node_id);
            return skip();
        }

        if (zwave_network_management_get_node_id() == 0) {
            sl_log_warning(LOG_TAG.data(), "Cannot determine own NodeID, skipping lifeline set for node %d", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult LifelineSetStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        (void)event;
        // Only send the Set; validation is done in the separate LifelineValidateStep.
        // Use fire_event_async and wait so that the SET is processed (attribute store updated,
        // resolver queued) before we transition to LifelineValidate and fire GET. Otherwise
        // GET can be queued and processed before SET, so MULTI_CHANNEL_ASSOCIATION_SET_GROUP
        // never sends any frames.
        zwave_node_id_t controller_node_id = zwave_network_management_get_node_id();

        component_connector connector;
        if (session.agi.agi_used_multi_channel) {
            component_connector_multi_channel_association_set_payload_t set_payload;
            set_payload.endpoint_node       = session.endpoint_node;
            set_payload.grouping_identifier = command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER;
            set_payload.node_id             = controller_node_id;
            set_payload.endpoint_id         = 0;
            connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET), set_payload);
            sl_log_info(LOG_TAG.data(), "Node %d: requesting Multi Channel Association Set (Group %d, NodeID %d, EP 0)", session.node_id, command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER, controller_node_id);
        } else {
            component_connector_association_set_payload_t set_payload;
            set_payload.endpoint_node       = session.endpoint_node;
            set_payload.grouping_identifier = command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER;
            set_payload.node_id             = controller_node_id;
            set_payload.endpoint_id         = 0;
            connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SET), set_payload);
            sl_log_info(LOG_TAG.data(), "Node %d: requesting Association Set (Group %d, NodeID %d)", session.node_id, command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER, controller_node_id);
        }

        return done();
    }

}  // namespace zwave_command_class
