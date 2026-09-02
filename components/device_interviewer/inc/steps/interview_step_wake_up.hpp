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

#ifndef INTERVIEW_STEP_WAKE_UP_H
#define INTERVIEW_STEP_WAKE_UP_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Interviews the Wake Up Command Class (0x84).
     *
     * Skipped if Wake Up CC (0x84) is absent from the node's NIF and S0/S2
     * Commands Supported lists.
     *
     * Wake Up CC version 2+: Interval Capabilities Get → Report, then Interval Set
     * (`zpc.default_wake_up_interval`, controller node id). After Set resolution,
     * the Wake Up CC queues Interval Get → Interval Report; the step ignores
     * Interval Reports until `WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED` is received
     * so stale reports cannot complete the step early.
     *
     * Wake Up CC version 1: Interval Set (same), same resolution gate, then Get → Report.
     */
    class WakeUpStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "WakeUp";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_WAKE_UP_H
