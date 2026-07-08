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

#ifndef INTERVIEW_STEP_GET_AGI_GROUP_COUNT_H
#define INTERVIEW_STEP_GET_AGI_GROUP_COUNT_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Ensures session.agi.agi_total_groups (and agi_used_multi_channel) is set for AGI.
     *
     * Runs when the device supports Association Group Info (0x59) but we did not
     * get the group count from MCA or Association groupings (e.g. path:
     * GET_ASSOCIATION_SUPPORTED_GROUPINGS skip). Fires SUPPORTED_GROUPINGS_COUNT
     * (MCA or Association based on command_classes_to_query), waits synchronously,
     * sets session.agi.agi_total_groups and session.agi.agi_used_multi_channel, then done().
     *
     * on_enter: skip if no AGI (0x59), or if agi_total_groups > 0 already; else stay().
     */
    class GetAgiGroupCountStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "GetAgiGroupCount";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_GET_AGI_GROUP_COUNT_H
