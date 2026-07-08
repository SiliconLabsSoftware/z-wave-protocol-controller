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

#ifndef INTERVIEW_STEP_GET_ENDPOINT_CAPABILITIES_H
#define INTERVIEW_STEP_GET_ENDPOINT_CAPABILITIES_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Queries Multi Channel Commands Capability for every discovered endpoint.
     *
     * Iterates over session.endpoints.endpoint_ids using session.endpoints.current_endpoint_it.
     * For each endpoint, fires COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_GET
     * and waits for MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT_RECEIVED. Stale
     * or mis-matched reports (wrong endpoint ID) are silently ignored.
     * Advances the iterator on each valid report and completes once all
     * endpoints have been queried.
     */
    class GetEndpointCapabilitiesStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "GetEndpointCapabilities";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_GET_ENDPOINT_CAPABILITIES_H
