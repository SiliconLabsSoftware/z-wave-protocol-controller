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

#include "interview_step_multi_channel_association_supported_groupings_count.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "component_connector_types.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_types.hpp"
#include "log.h"
#include <any>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool MultiChannelAssociationSupportedGroupingsCountStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED;
    }

    StepResult MultiChannelAssociationSupportedGroupingsCountStep::on_enter(InterviewSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult MultiChannelAssociationSupportedGroupingsCountStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            component_connector_multi_channel_association_groupings_get_payload_t payload;
            payload.endpoint_node = session.endpoint_node;
            sl_log_debug(LOG_TAG.data(), "Node %d: requesting Multi Channel Association Supported Groupings (trigger group resolution)", session.node_id);
            connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET), payload);
            return stay();
        }

        const auto &payload = std::any_cast<const component_connector_multi_channel_association_groupings_get_payload_t &>(event->payload);

        session.agi.agi_used_multi_channel = true;
        session.agi.agi_total_groups       = payload.supported_groupings;

        sl_log_info(LOG_TAG.data(), "Node %d (%s): event %s - Multi Channel Association reports %d groups", session.node_id, name().c_str(), to_string(event->event), session.agi.agi_total_groups);
        return done();
    }

}  // namespace zwave_command_class
