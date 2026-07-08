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

#ifndef INTERVIEW_STEP_PREPARE_VERSION_CC_LIST_H
#define INTERVIEW_STEP_PREPARE_VERSION_CC_LIST_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Prepares the root version-CC list once before the version loop.
     *
     * Merges S2, S0 and node info command class lists, deduplicates, removes
     * 0x00 and 0x20, and sets session.version_cc.command_classes_to_query and
     * session.version_cc.current_cc_it. Run once before VERSION_CC_SEQUENCE.
     *
     * on_enter: skip if Version CC (0x86) is not in any of the three source lists; else stay().
     * handle_event(nullopt): build merged list, set session state, return done().
     */
    class PrepareVersionCCListStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "PrepareVersionCCList";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_PREPARE_VERSION_CC_LIST_H
