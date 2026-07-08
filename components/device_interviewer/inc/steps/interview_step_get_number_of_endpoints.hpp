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

#ifndef INTERVIEW_STEP_GET_NUMBER_OF_ENDPOINTS_H
#define INTERVIEW_STEP_GET_NUMBER_OF_ENDPOINTS_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Discovers endpoint IDs via Multi Channel Endpoint Find for dynamic devices.
     *
     * Skipped immediately (done()) if session.endpoints.endpoint_ids was already
     * populated by McEndpointGetStep (i.e. the device has static endpoints).
     * For dynamic devices, fires COMMAND_CLASS_MULTI_CHANNEL_END_POINT_FIND
     * and waits for MULTI_CHANNEL_END_POINT_FIND_REPORT_RECEIVED. On
     * receiving the report, populates session.endpoints.endpoint_ids with the
     * discovered endpoint IDs and initialises session.endpoints.current_endpoint_it.
     * Returns skip() if the report contains no endpoints.
     */
    class GetNumberOfEndpointsStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "GetNumberOfEndpoints";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_GET_NUMBER_OF_ENDPOINTS_H
