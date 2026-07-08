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
     *    and queue any post-interview attribute resolutions before the
     *    listener is installed.
     * 2. Installs an attribute resolver listener on the device (NodeID)
     *    node. When the listener fires, the decision is deferred briefly
     *    via attribute_timeout_set_callback and re-checked: if any node in
     *    the subtree picked up a new resolution in the meantime (e.g. a
     *    CC's on_*_report_parsed hook chaining the next get like
     *    command_class_switch_color iterating colour components), the
     *    listener is re-armed and the device keeps interviewing. Only when
     *    the subtree is genuinely settled does the step fire
     *    COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED for the root and
     *    every endpoint. This is the signal that the device is truly
     *    ready for clients (e.g. the MQTT Interview/Report publisher).
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
