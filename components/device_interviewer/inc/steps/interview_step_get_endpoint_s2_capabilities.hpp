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

#ifndef INTERVIEW_STEP_GET_ENDPOINT_S2_CAPABILITIES_H
#define INTERVIEW_STEP_GET_ENDPOINT_S2_CAPABILITIES_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Queries S2 Commands Supported for every discovered endpoint.
     *
     * Skipped if session.endpoints.endpoint_ids is empty (no Multi Channel endpoints).
     * Resets session.endpoints.current_endpoint_it to the beginning on initial entry,
     * then fires COMMAND_CLASS_S2_COMMANDS_SUPPORTED_GET for each endpoint
     * in turn and waits for S2_COMMANDS_SUPPORTED_REPORT. Mis-matched
     * endpoint reports are silently ignored. Advances the iterator on each
     * valid report and completes once all endpoints have been queried.
     */
    class GetEndpointS2CapabilitiesStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "GetEndpointS2Capabilities";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_GET_ENDPOINT_S2_CAPABILITIES_H
