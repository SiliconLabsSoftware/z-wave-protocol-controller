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

#ifndef INTERVIEW_STEP_VERSION_GET_H
#define INTERVIEW_STEP_VERSION_GET_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Waits for a Version Command Class Report as the second half of the
     *        ping-pong loop with VersionCCSequenceStep.
     *
     * When the CC version report arrives (VERSION_CC_GET_REQUESTED), this step
     * validates it against the current iterator position, advances the iterator
     * (skipping NOP and Basic), and returns done() to hand control back to
     * VersionCCSequenceStep for the next CC. The loop exits when
     * VersionCCSequenceStep detects the iterator has reached the end.
     */
    class VersionGetStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "VersionGet";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_VERSION_GET_H
