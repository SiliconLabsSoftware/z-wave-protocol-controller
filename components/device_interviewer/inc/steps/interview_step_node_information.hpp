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

#ifndef INTERVIEW_STEP_NODE_INFORMATION_H
#define INTERVIEW_STEP_NODE_INFORMATION_H

#include "interview_step.hpp"

namespace zwave_command_class
{
    /**
     * @brief Requests and stores the Node Information Frame (NIF).
     *
     * Fires COMMAND_CLASS_PROTOCOL_COMMANDS_REQUEST_NODE_INFO on initial entry.
     * On receiving NODE_INFORMATION_RECEIVED, persists the CC list, device
     * classes (basic/generic/specific), and listening/optional protocol flags
     * into the attribute store, and caches the CC list in
     * session.node_information_command_class_list. Also resolves and stores
     * session.device_node and session.endpoint_node (endpoint 0).
     * This step never skips; it always issues the request.
     */
    class NodeInformationStep : public InterviewStep
    {
        public:
            std::string name() const override
            {
                return "NodeInformation";
            }

            bool handles_external_event(device_interviewer_external_event_t event_type) const override;

            StepResult handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event) override;

            StepResult on_enter(InterviewSession &session) override;
    };

}  // namespace zwave_command_class

#endif  // INTERVIEW_STEP_NODE_INFORMATION_H
