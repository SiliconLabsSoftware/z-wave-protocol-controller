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

#include "interview_step_agi_group_name_get.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "component_connector_types.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    using namespace command_class_association_grp_info_types;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool AgiGroupNameGetStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT_RECEIVED;
    }

    StepResult AgiGroupNameGetStep::on_enter(InterviewSession &session)
    {
        const bool has_agi = std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x59) != session.version_cc.command_classes_to_query.end();

        if (!has_agi) {
            sl_log_debug(LOG_TAG.data(), "Node %d does not support Association Group Info (0x59), skipping AGI group name step", session.node_id);
            return skip();
        }

        if (session.agi.agi_total_groups == 0) {
            sl_log_debug(LOG_TAG.data(), "Node %d has 0 association groups, skipping AGI group name step", session.node_id);
            return skip();
        }

        if (session.agi.agi_current_group_id == 0) {
            session.agi.agi_current_group_id = 1;
        }

        if (session.agi.agi_current_group_id > session.agi.agi_total_groups) {
            sl_log_info(LOG_TAG.data(), "Node %d: finished querying AGI for all %d groups", session.node_id, session.agi.agi_total_groups);
            return skip();
        }

        return stay();
    }

    StepResult AgiGroupNameGetStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            sl_log_info(LOG_TAG.data(), "Node %d: querying Association Group Name for group %d", session.node_id, session.agi.agi_current_group_id);

            component_connector connector;
            component_connector_agi_group_name_get_payload_t p {};
            p.device_endpoint_node = session.endpoint_node;
            p.grouping_identifier  = session.agi.agi_current_group_id;

            connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_NAME_GET), p);
            return stay();
        }

        (void)event;
        sl_log_debug(LOG_TAG.data(), "Node %d: Association Group Name report received for group %d", session.node_id, session.agi.agi_current_group_id);
        return done();
    }

}  // namespace zwave_command_class
