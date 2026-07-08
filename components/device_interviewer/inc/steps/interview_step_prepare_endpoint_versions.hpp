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

#ifndef INTERVIEW_STEP_PREPARE_ENDPOINT_VERSIONS_H
#define INTERVIEW_STEP_PREPARE_ENDPOINT_VERSIONS_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Prepares the session for endpoint Version CC queries.
     *
     * Compares endpoint_discovered_command_classes (accumulated by the
     * GET_ENDPOINT_S2/S0 steps) against command_classes_to_query (CCs
     * already versioned during the root interview). Sets up
     * command_classes_to_query + current_cc_it with only the new CCs so
     * that the reused VersionCCSequenceStep / VersionGetStep pair can
     * query their versions.
     *
     * Returns done() if new CCs need versioning, skip() otherwise.
     */
    class PrepareEndpointVersionsStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "PrepareEndpointVersions";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_PREPARE_ENDPOINT_VERSIONS_H
