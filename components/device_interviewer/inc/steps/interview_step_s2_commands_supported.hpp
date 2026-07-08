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

#ifndef INTERVIEW_STEP_S2_COMMANDS_SUPPORTED_H
#define INTERVIEW_STEP_S2_COMMANDS_SUPPORTED_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Queries the S2-encapsulated command class list for endpoint 0.
     *
     * Skipped if Security 2 CC (0x9F) is absent from the NIF, or if no S2
     * key (Unauthenticated / Authenticated / Access) was granted during
     * inclusion. Otherwise fires COMMAND_CLASS_S2_COMMANDS_SUPPORTED_GET and
     * waits for S2_COMMANDS_SUPPORTED_REPORT. On TX failure, retries up to 5
     * times then fails the interview. On success, stores the reported
     * CC list in session.s2_supported_command_classes for later use by the
     * Version CC sequence.
     */
    class S2CommandsSupportedStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "S2CommandsSupported";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_S2_COMMANDS_SUPPORTED_H
