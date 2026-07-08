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

#ifndef INTERVIEW_STEP_CHECK_MULTI_CHANNEL_SUPPORT_H
#define INTERVIEW_STEP_CHECK_MULTI_CHANNEL_SUPPORT_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Checks whether the node supports Multi Channel CC (0x60).
     *
     * Looks up Multi Channel in the session's command_classes_to_query list
     * in on_enter. Returns done() if Multi Channel is supported (triggering
     * McEndpointGetStep), or skip() to bypass the entire Multi Channel
     * endpoint discovery sub-sequence. Handles no external events.
     */
    class CheckMultiChannelSupportStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "CheckMultiChannelSupport";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_CHECK_MULTI_CHANNEL_SUPPORT_H
