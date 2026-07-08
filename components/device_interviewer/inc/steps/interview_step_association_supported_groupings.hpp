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

#ifndef INTERVIEW_STEP_ASSOCIATION_SUPPORTED_GROUPINGS_H
#define INTERVIEW_STEP_ASSOCIATION_SUPPORTED_GROUPINGS_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Queries Association Supported Groupings (0x85) as a fallback.
     *
     * Active only when Association CC (0x85) is present in the queried CC
     * list AND Multi Channel Association CC (0x8E) is absent (i.e. this
     * step is the fallback when MultiChannelAssociationSupportedGroupingsStep
     * was skipped). Fires COMMAND_CLASS_ASSOCIATION_GROUPINGS_GET and waits
     * for ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED. On success, sets
     * session.agi.agi_used_multi_channel = false so that subsequent Association
     * Get and AGI steps use plain Association commands.
     */
    class AssociationSupportedGroupingsStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "AssociationSupportedGroupings";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_ASSOCIATION_SUPPORTED_GROUPINGS_H
