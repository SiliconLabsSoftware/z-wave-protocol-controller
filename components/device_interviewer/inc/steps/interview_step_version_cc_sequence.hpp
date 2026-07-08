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

#ifndef INTERVIEW_STEP_VERSION_CC_SEQUENCE_H
#define INTERVIEW_STEP_VERSION_CC_SEQUENCE_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Sends a Version Command Class Get for the next CC in the interview list.
     *
     * This step is one half of a ping-pong loop with VersionGetStep. The list
     * (session.version_cc.command_classes_to_query and current_cc_it) is prepared by
     * PrepareVersionCCListStep (root) or PrepareEndpointVersionsStep (endpoints).
     *
     * on_enter: skip if list is empty or current_cc_it is at end; else stay().
     * handle_event(nullopt): fires COMMAND_CLASS_VERSION_CC_GET for current CC, returns done().
     */
    class VersionCCSequenceStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "VersionCCSequence";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_VERSION_CC_SEQUENCE_H
