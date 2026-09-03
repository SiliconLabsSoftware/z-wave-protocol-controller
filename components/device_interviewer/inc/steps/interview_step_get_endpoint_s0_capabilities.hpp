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

#ifndef INTERVIEW_STEP_GET_ENDPOINT_S0_CAPABILITIES_H
#define INTERVIEW_STEP_GET_ENDPOINT_S0_CAPABILITIES_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Queries S0 Commands Supported for every discovered endpoint.
     *
     * Skipped if session.endpoints.endpoint_ids is empty, if S0 was not granted,
     * or if an S2 key was granted (S0 is not the highest Security Class).
     * Resets session.endpoints.current_endpoint_it to the
     * beginning on initial entry, then fires
     * COMMAND_CLASS_S0_COMMANDS_SUPPORTED_GET for each endpoint in turn and
     * waits for S0_COMMANDS_SUPPORTED_REPORT. Mis-matched endpoint reports
     * (e.g. root EP0 reply to an endpoint Get) skip the current endpoint so
     * the interview cannot stall. Advances the iterator on each handled
     * report and completes once all endpoints have been queried.
     */
    class GetEndpointS0CapabilitiesStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "GetEndpointS0Capabilities";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_GET_ENDPOINT_S0_CAPABILITIES_H
