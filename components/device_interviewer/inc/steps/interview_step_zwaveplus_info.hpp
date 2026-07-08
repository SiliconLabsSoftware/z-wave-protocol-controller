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

#ifndef INTERVIEW_STEP_ZWAVEPLUS_INFO_H
#define INTERVIEW_STEP_ZWAVEPLUS_INFO_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Interviews the Z-Wave Plus Info Command Class (0x5E).
     *
     * Skipped if Z-Wave Plus Info CC (0x5E) is absent from the queried CC
     * list. Otherwise fires COMMAND_CLASS_ZWAVEPLUS_INFO_GET_INTERVIEW and
     * waits for ZWAVEPLUS_INFO_REPORT_RECEIVED, which carries the Z-Wave
     * Plus version, role type, node type, installer icon, and user icon.
     */
    class ZWavePlusInfoStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "ZWavePlusInfo";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_ZWAVEPLUS_INFO_H
