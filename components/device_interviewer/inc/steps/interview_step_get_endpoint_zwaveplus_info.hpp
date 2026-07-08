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

#ifndef INTERVIEW_STEP_GET_ENDPOINT_ZWAVEPLUS_INFO_H
#define INTERVIEW_STEP_GET_ENDPOINT_ZWAVEPLUS_INFO_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Queries Z-Wave Plus Info (CC 0x5E) for every discovered endpoint.
     *
     * Per Z-Wave Plus Info v2 spec: each End Point must be interviewed to
     * advertise individual icons. Iterates over session.endpoints.endpoint_ids, sends
     * ZWAVEPLUS_INFO_GET per endpoint, waits for ZWAVEPLUS_INFO_REPORT_RECEIVED.
     */
    class GetEndpointZwavePlusInfoStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "GetEndpointZwavePlusInfo";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_GET_ENDPOINT_ZWAVEPLUS_INFO_H
