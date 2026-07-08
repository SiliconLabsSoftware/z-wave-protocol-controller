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

#ifndef INTERVIEW_STEP_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT_H
#define INTERVIEW_STEP_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Triggers Multi Channel Association Supported Groupings resolution,
     *        waits for the report, then stores the count in session.agi.agi_total_groups.
     *
     * Runs after MultiChannelAssociationSupportedGroupingsStep (which gates on
     * MCA support). Fires COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET,
     * returns stay(); on MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED
     * reads the count from the attribute store and returns done().
     */
    class MultiChannelAssociationSupportedGroupingsCountStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "MultiChannelAssociationSupportedGroupingsCount";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT_H
