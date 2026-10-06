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

#ifndef INTERVIEW_STEP_BASIC_H
#define INTERVIEW_STEP_BASIC_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Probes Basic CC (0x20) on root and every discovered endpoint.
     *
     * Basic MUST NOT be advertised (CC:0020.01.00.21.003/004). Support is
     * established only if a Basic Report is returned (CL:0020.01.21.02.2)
     * after Basic Get (CL:0020.01.21.01.1). A report stores version 1
     * immediately (CL:0020.01.21.02.2). Version Get for 0x20 follows only
     * when Version CC (0x86) is advertised; otherwise version 1 is kept.
     * Give-up stores version 0 and continues; this step never fails the interview.
     */
    class BasicInterviewStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "BasicInterview";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_BASIC_H
