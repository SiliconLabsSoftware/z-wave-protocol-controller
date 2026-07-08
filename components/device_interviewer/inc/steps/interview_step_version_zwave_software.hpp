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

#ifndef INTERVIEW_STEP_VERSION_ZWAVE_SOFTWARE_H
#define INTERVIEW_STEP_VERSION_ZWAVE_SOFTWARE_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Version Z-Wave Software Get when Version Capabilities Report
     *        indicates Z-Wave Software support (CL:0086.01.21.02.1).
     */
    class VersionZwaveSoftwareInterviewStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "VersionZwaveSoftwareInterview";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_VERSION_ZWAVE_SOFTWARE_H
