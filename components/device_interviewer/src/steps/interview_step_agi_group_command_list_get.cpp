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

#include "interview_step_agi_group_command_list_get.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "component_connector_types.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "log.h"

namespace zwave_command_class
{
    using namespace command_class_association_grp_info_types;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool AgiGroupCommandListGetStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT_RECEIVED;
    }

    StepResult AgiGroupCommandListGetStep::on_enter(InterviewSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult AgiGroupCommandListGetStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            sl_log_info(LOG_TAG.data(), "Node %d: querying Association Group Command List for group %d", session.node_id, session.agi.agi_current_group_id);

            component_connector connector;
            component_connector_agi_group_command_list_get_payload_t p {};
            p.device_endpoint_node = session.endpoint_node;
            p.grouping_identifier  = session.agi.agi_current_group_id;
            p.allow_cache          = 1;
            connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_GET), p);
            return stay();
        }

        sl_log_debug(LOG_TAG.data(), "Node %d: Association Group Command List report received for group %d", session.node_id, session.agi.agi_current_group_id);
        session.agi.agi_current_group_id++;
        return done();
    }

}  // namespace zwave_command_class
