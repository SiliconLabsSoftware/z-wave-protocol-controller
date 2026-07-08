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

#ifndef INTERVIEW_STEP_LIFELINE_SET_H
#define INTERVIEW_STEP_LIFELINE_SET_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Sets the lifeline association (Group 1) to the controller's NodeID.
     *
     * Skipped if the device has no association groups or if the controller's
     * own NodeID cannot be determined. Sends only the Set
     * (COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET or
     * COMMAND_CLASS_ASSOCIATION_SET, chosen via session.agi.agi_used_multi_channel)
     * for Group 1 with endpoint 0 of the controller as the destination.
     * Validation (Get + Report) is done in the separate LifelineValidateStep.
     *
     * CL:0071.02.21.01.2: Controller MUST configure Lifeline (Group 1)
     * whenever the device supports Association CC.
     */
    class LifelineSetStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "LifelineSet";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_LIFELINE_SET_H
