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

#ifndef INTERVIEW_STEP_ENDPOINT_ASSOCIATION_ITERATOR_H
#define INTERVIEW_STEP_ENDPOINT_ASSOCIATION_ITERATOR_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Iterates over endpoints to run Association/MCA and AGI per endpoint.
     *
     * On first enter (from ENDPOINT_ZWAVEPLUS_INFO): sets session to first
     * endpoint and transitions to GET_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS.
     * When re-entered after VALIDATE_LIFELINE (session.endpoint_id != 0): advances
     * to next endpoint or restores root and transitions to COMPLETED.
     *
     * For each endpoint, resets AGI/association progress and rebuilds
     * version_cc.command_classes_to_query from that endpoint's Multi Channel
     * Capability Report (falling back to NIF / Secure NIF only if absent) so
     * MCA/Association/AGI are only interviewed when the endpoint advertises
     * those command classes.
     */
    class EndpointAssociationIteratorStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "EndpointAssociationIterator";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_ENDPOINT_ASSOCIATION_ITERATOR_H
