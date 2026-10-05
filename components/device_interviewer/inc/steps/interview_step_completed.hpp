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

#ifndef INTERVIEW_STEP_COMPLETED_H
#define INTERVIEW_STEP_COMPLETED_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Terminal step reached when all interview steps have completed.
     *
     * On entry (handle_event with nullopt):
     * 1. Fires COMPONENT_CONNECTOR_INTERVIEW_DONE (synchronously, via
     *    fire_event_async + .get()) for the root and every discovered
     *    endpoint so that command classes can run their on_interview hooks
     *    and register required attributes before publish is allowed.
     * 2. Calls cc_interview_finish_if_complete on the component-connector worker.
     *    That publishes COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED OK for
     *    every endpoint when nothing is still required; otherwise the
     *    session stays in COMPLETED until chained Gets finish (or give-up).
     *
     * Handles no external events and never transitions out of this state.
     */
    class CompletedStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "Completed";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_COMPLETED_H
