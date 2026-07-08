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

#include "interview_step_version_cc_sequence.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_version_events.hpp"
#include "command_class_version_types.hpp"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool VersionCCSequenceStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return false;
    }

    StepResult VersionCCSequenceStep::on_enter(InterviewSession &session)
    {
        if (session.version_cc.command_classes_to_query.empty()) {
            sl_log_debug(LOG_TAG.data(), "Node %d: no command classes to query, skipping Version CC sequence", session.node_id);
            return skip();
        }

        if (session.version_cc.current_cc_it == session.version_cc.command_classes_to_query.end()) {
            sl_log_info(LOG_TAG.data(), "All command classes queried for node %d, endpoint %d", session.node_id, session.endpoint_id);
            return skip();
        }

        return stay();
    }

    StepResult VersionCCSequenceStep::handle_event(InterviewSession &session, [[maybe_unused]] std::optional<device_interviewer_external_event_data> event)
    {
        component_connector connector;
        command_class_version_types::command_class_version_cc_get_payload_t payload_map_version;
        payload_map_version.device_endpoint_node   = session.endpoint_node;
        payload_map_version.command_class          = *session.version_cc.current_cc_it;
        payload_map_version.retry_count            = 5;
        payload_map_version.is_first_command_class = (session.version_cc.current_cc_it == session.version_cc.command_classes_to_query.begin());
        connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CC_GET), payload_map_version);
        return done();
    }

}  // namespace zwave_command_class
