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

#ifndef INTERVIEW_STEP_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_H
#define INTERVIEW_STEP_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Gates on Multi Channel Association (0x8E) support.
     *
     * Skipped if Multi Channel Association CC (0x8E) is absent from the
     * queried CC list. Otherwise returns done() to transition to
     * MultiChannelAssociationSupportedGroupingsCountStep, which triggers
     * the groupings resolution and waits for the report.
     */
    class MultiChannelAssociationSupportedGroupingsStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "MultiChannelAssociationSupportedGroupings";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_H
