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

#ifndef INTERVIEW_STEP_MC_ENDPOINT_GET_H
#define INTERVIEW_STEP_MC_ENDPOINT_GET_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Fires Multi Channel Endpoint Get and parses the Endpoint Report.
     *
     * Fires COMMAND_CLASS_MULTI_CHANNEL_END_POINT_GET_INTERVIEW and waits
     * for MULTI_CHANNEL_END_POINT_REPORT_RECEIVED. Skips if the reported
     * individual endpoint count is zero. Stores the dynamic-endpoints flag in
     * session.multi_channel.mc_has_dynamic_endpoints. For static devices (dynamic=false),
     * pre-fills session.endpoints.endpoint_ids with IDs 1..N so that
     * GetNumberOfEndpointsStep can skip its Endpoint Find request.
     */
    class McEndpointGetStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "McEndpointGet";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_MC_ENDPOINT_GET_H
