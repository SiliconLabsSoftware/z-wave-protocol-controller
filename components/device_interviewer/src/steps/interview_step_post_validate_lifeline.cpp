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

#include "interview_step_post_validate_lifeline.hpp"
#include "interview_state_machine.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool PostValidateLifelineStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult PostValidateLifelineStep::on_enter(InterviewSession &session)
    {
        if (session.endpoint_id != 0) {
            sl_log_debug(LOG_TAG.data(), "Node %d: finished lifeline validate for endpoint %d, routing to endpoint association iterator", session.node_id, session.endpoint_id);
            return skip();
        }
        sl_log_debug(LOG_TAG.data(), "Node %d: root lifeline validated, routing to check multi channel support", session.node_id);
        return done();
    }

    StepResult PostValidateLifelineStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace zwave_command_class
