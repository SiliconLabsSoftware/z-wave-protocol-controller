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

#ifndef INTERVIEW_STEP_POST_VALIDATE_LIFELINE_H
#define INTERVIEW_STEP_POST_VALIDATE_LIFELINE_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Router after VALIDATE_LIFELINE: root path vs per-endpoint path.
     *
     * If session.endpoint_id != 0 we just finished lifeline validation for an
     * endpoint → return skip() → ENDPOINT_ASSOCIATION_ITERATOR (next endpoint
     * or COMPLETED). If session.endpoint_id == 0 we finished root lifeline →
     * return done() → CHECK_MULTI_CHANNEL_SUPPORT.
     */
    class PostValidateLifelineStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "PostValidateLifeline";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_POST_VALIDATE_LIFELINE_H
