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

#ifndef INTERVIEW_STEP_VERSION_REPORT_H
#define INTERVIEW_STEP_VERSION_REPORT_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Sends Version Get to query library type, protocol version,
     *        and application firmware version (Version Report).
     *
     * This step fires COMMAND_CLASS_VERSION_GET_INTERVIEW to the Version CC,
     * which creates the VERSION_GET_GROUP node and starts resolution. When the
     * Version Report is received and parsed, the Version CC fires
     * DEVICE_INTERVIEWER_VERSION_GET_DONE, which is mapped to the
     * VERSION_REPORT_RECEIVED external event.
     */
    class VersionReportStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "VersionReport";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_VERSION_REPORT_H
