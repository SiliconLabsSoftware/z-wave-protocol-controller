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

#include "interview_step_version_get.hpp"
#include "interview_state_machine.hpp"
#include "command_class_version_types.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool VersionGetStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::VERSION_CC_GET_REQUESTED;
    }

    StepResult VersionGetStep::on_enter(InterviewSession &session)
    {
        return stay();
    }

    StepResult VersionGetStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_version_types::command_class_version_cc_get_payload_t>(event->payload);

            if (session.version_cc.current_cc_it == session.version_cc.command_classes_to_query.end() || payload.command_class != *session.version_cc.current_cc_it) {
                sl_log_debug(LOG_TAG.data(), "Unexpected version report for CC 0x%02X for node %d, endpoint %d. Ignoring.", payload.command_class, session.node_id, session.endpoint_id);
                return stay();
            }

            ++session.version_cc.current_cc_it;

            sl_log_info(LOG_TAG.data(), "Node %d received CC version report for CC 0x%02X", session.node_id, payload.command_class);
            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for VERSION_CC_GET_REQUESTED");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
