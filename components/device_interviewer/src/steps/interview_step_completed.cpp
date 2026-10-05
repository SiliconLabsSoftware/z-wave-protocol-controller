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

#include "interview_step_completed.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "log.h"

#include <future>
#include <vector>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool CompletedStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult CompletedStep::on_enter(InterviewSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult CompletedStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            sl_log_info(LOG_TAG.data(), "Interview process completed successfully for node %d, endpoint %d", session.node_id, session.endpoint_id);

            component_connector connector;

            // Fire INTERVIEW_DONE synchronously so every command class on_interview
            // runs — and registers required attributes — before we allow publish.
            std::vector<std::future<sl_status_t>> futures;
            std::vector<attribute_store::attribute> endpoint_nodes;

            if (session.endpoint_node.is_valid()) {
                endpoint_nodes.push_back(session.endpoint_node);
            }

            for (const auto &ep_id: session.endpoints.endpoint_ids) {
                if (ep_id == 0) {
                    continue;
                }
                auto ep_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, ep_id);
                if (!ep_node.is_valid()) {
                    sl_log_warning(LOG_TAG.data(), "Node %d: endpoint %d node not found in attribute store, skipping interview done notification", session.node_id, ep_id);
                    continue;
                }
                endpoint_nodes.push_back(ep_node);
            }

            for (const auto &ep_node: endpoint_nodes) {
                component_connector_interview_done_payload_t ep_payload {.endpoint_node = ep_node, .status = SL_STATUS_OK};
                futures.push_back(connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), ep_payload));
            }

            for (auto &f: futures) {
                static_cast<void>(f.get());
            }

            if (!session.endpoint_node.is_valid()) {
                sl_log_error(LOG_TAG.data(), "Node %d: no root endpoint for cc_interview_finish_if_complete", session.node_id);
                return fail();
            }

            component_connector_cc_interview_action_payload_t finish_payload {
              .endpoint_node = session.endpoint_node,
              .action        = component_connector_cc_interview_action_t::finish_if_complete,
            };
            if (connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_CC_INTERVIEW_ACTION_REQUESTED), finish_payload).get() != SL_STATUS_OK) {
                sl_log_error(LOG_TAG.data(), "Node %d: failed to cc_interview_finish_if_complete", session.node_id);
                return fail();
            }
        }

        return stay();
    }

}  // namespace zwave_command_class
