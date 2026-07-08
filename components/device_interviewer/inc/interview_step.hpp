
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

#ifndef INTERVIEW_STEP_H
#define INTERVIEW_STEP_H

#include "sl_status.h"
#include "device_interviewer_events.hpp"
#include "device_interviewer_external_event_types.hpp"  // For device_interviewer_external_event_t and device_interviewer_external_event_data
#include "interview_state_machine.hpp"                  // InterviewSession (complete type for step_base; no cycle — state machine header does not include interview_step.hpp)
#include <optional>
#include <string>

namespace zwave_command_class
{
    using StepResult     = state_machine::StepResult;
    using StepResultCode = state_machine::StepResultCode;

    /**
     * @brief Base class for interview steps
     *
     * Each step represents one phase of the device interview process.
     * Steps handle events and return semantic result codes; the engine resolves
     * the next state via the transition table.
     */
    class InterviewStep : public state_machine::step_base<InterviewSession, device_interviewer_external_event_data>
    {
        public:
            ~InterviewStep() override = default;

            /**
             * @brief Check if this step can handle the given event type
             * @param event_type The event type
             * @return True if this step handles this event
             */
            virtual bool handles_external_event(device_interviewer_external_event_t event_type) const = 0;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_H
