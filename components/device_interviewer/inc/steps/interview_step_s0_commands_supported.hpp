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

#ifndef INTERVIEW_STEP_S0_COMMANDS_SUPPORTED_H
#define INTERVIEW_STEP_S0_COMMANDS_SUPPORTED_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Queries the S0-encapsulated command class list for endpoint 0.
     *
     * Skipped if Security 0 CC (0x98) is absent from the NIF, or if the S0
     * key was not granted during inclusion. Otherwise fires
     * COMMAND_CLASS_S0_COMMANDS_SUPPORTED_GET and waits for
     * S0_COMMANDS_SUPPORTED_REPORT. On success, stores the reported CC list
     * in session.s0_supported_command_classes for later use by the Version
     * CC sequence.
     */
    class S0CommandsSupportedStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "S0CommandsSupported";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_S0_COMMANDS_SUPPORTED_H
