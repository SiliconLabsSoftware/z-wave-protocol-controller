/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef INTERVIEW_STEP_VERSION_CAPABILITIES_H
#define INTERVIEW_STEP_VERSION_CAPABILITIES_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Version Capabilities Get after initial Version Report (management
     *        mandatory interview CL:0086.01.21.01.2). Skipped when Version CC
     *        (0x86) is not present in merged S2/S0/NIF lists.
     */
    class VersionCapabilitiesInterviewStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "VersionCapabilitiesInterview";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_VERSION_CAPABILITIES_H
