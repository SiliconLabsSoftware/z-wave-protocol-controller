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

#include "interview_step_association_supported_groupings.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "component_connector_types.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "log.h"
#include <algorithm>
#include <future>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool AssociationSupportedGroupingsStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED;
    }

    StepResult AssociationSupportedGroupingsStep::on_enter(InterviewSession &session)
    {
        const bool has_multi_channel_association = std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x8E) != session.version_cc.command_classes_to_query.end();
        const bool has_association               = std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x85) != session.version_cc.command_classes_to_query.end();

        sl_log_info(LOG_TAG.data(), "AssociationSupportedGroupings step for node %d: has_association=%d, has_multi_channel_association=%d", session.node_id, has_association, has_multi_channel_association);

        if (!has_association || has_multi_channel_association) {
            return skip();
        }

        return stay();
    }

    StepResult AssociationSupportedGroupingsStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            component_connector_association_groupings_get_payload_t ag_payload;
            ag_payload.endpoint_node = session.endpoint_node;
            sl_log_debug(LOG_TAG.data(), "Node %d supports Association (no Multi Channel Association), requesting Supported Groupings", session.node_id);
            connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GROUPINGS_GET), ag_payload);
            return stay();
        }

        session.agi.agi_used_multi_channel = false;

        component_connector connector;
        component_connector_association_groupings_get_payload_t count_payload;
        count_payload.endpoint_node  = session.endpoint_node;
        auto future                  = connector.fire_event_async<component_connector_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), count_payload);
        auto [status, result]        = future.get();
        session.agi.agi_total_groups = (status == SL_STATUS_OK) ? result : 0;

        sl_log_info(LOG_TAG.data(), "Node %d: Association reports %d groups", session.node_id, session.agi.agi_total_groups);
        return done();
    }

}  // namespace zwave_command_class
