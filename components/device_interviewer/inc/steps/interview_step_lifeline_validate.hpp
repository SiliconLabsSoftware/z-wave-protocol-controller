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

#ifndef INTERVIEW_STEP_LIFELINE_VALIDATE_H
#define INTERVIEW_STEP_LIFELINE_VALIDATE_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Validates the lifeline association (Group 1) by sending Get and waiting for Report.
     *
     * Runs after LifelineSetStep. Sends Association Get (or Multi Channel Association Get)
     * for group 1 depending on session.agi.agi_used_multi_channel, then waits for the
     * corresponding Report. On report for group 1, completes with DONE.
     */
    class LifelineValidateStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "LifelineValidate";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_LIFELINE_VALIDATE_H
