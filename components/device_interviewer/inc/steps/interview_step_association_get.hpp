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

#ifndef INTERVIEW_STEP_ASSOCIATION_GET_H
#define INTERVIEW_STEP_ASSOCIATION_GET_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Reads the current association members for every supported group.
     *
     * Skipped if session.agi.agi_total_groups is 0. For each group ID from 1 to
     * agi_total_groups, fires either COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GET
     * or COMMAND_CLASS_ASSOCIATION_GET depending on session.agi.agi_used_multi_channel,
     * then waits for MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED or
     * ASSOCIATION_REPORT_RECEIVED respectively. Advances the group counter on
     * each valid report and completes after the last group has been read.
     */
    class AssociationGetStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "AssociationGet";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_ASSOCIATION_GET_H
