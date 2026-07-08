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
#include "attribute_resolver.h"
#include "attribute_timeouts.h"
#include "log.h"

#include <future>
#include <vector>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    namespace
    {
        // Time after a "subtree resolved" notification before we publish the
        // INTERVIEW_FULLY_RESOLVED event. The grace period lets in-flight report
        // parsers that synchronously chain into a follow-up start_group_resolution
        // (e.g. command_class_switch_color iterating colour components) re-create
        // a desired/reported mismatch before we re-check. Mirrors the value used
        // by command_class_wake_up's no-more-information deferral.
        constexpr clock_time_t INTERVIEW_FULLY_RESOLVED_DEFER_MS = 100;

        void on_device_resolution_done(attribute_store_node_t node_id_node);

        // Re-evaluated after the grace period: if the subtree picked up new
        // resolutions in the meantime (chained get/set rules from a CC's
        // on_*_report_parsed hooks), re-arm the listener and try again;
        // otherwise the device is genuinely settled and we can fire the
        // user-visible INTERVIEW_FULLY_RESOLVED event for every endpoint.
        void on_device_resolution_done_deferred(attribute_store_node_t node_id_node)
        {
            if (attribute_resolver_node_or_child_needs_resolution(node_id_node)) {
                attribute_resolver_set_resolution_listener(node_id_node, on_device_resolution_done);
                return;
            }

            attribute_store::attribute device(node_id_node);
            component_connector connector;

            for (const auto &endpoint_node: device.children(ATTRIBUTE_ENDPOINT_ID)) {
                if (!endpoint_node.is_valid()) {
                    continue;
                }
                component_connector_interview_done_payload_t payload;
                payload.endpoint_node = endpoint_node;
                payload.status        = SL_STATUS_OK;
                connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED), payload);
            }
        }

        // Listener invoked by the attribute resolver when the NodeID subtree
        // first appears fully resolved. Same shape as
        // command_class_wake_up::on_wake_up_no_more_information_resolution_listener:
        // clear the listener and defer the actual decision so chained
        // resolutions can settle.
        void on_device_resolution_done(attribute_store_node_t node_id_node)
        {
            attribute_resolver_clear_resolution_listener(node_id_node, on_device_resolution_done);
            attribute_timeout_set_callback(node_id_node, INTERVIEW_FULLY_RESOLVED_DEFER_MS, on_device_resolution_done_deferred);
        }
    }  // namespace

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

            // Fire INTERVIEW_DONE synchronously (fire_event_async + .get()) so every
            // command class on_interview hook runs — and queues its post-interview
            // attribute resolutions — before we install the resolution listener.
            // Otherwise the listener could fire prematurely on an empty subtree.
            std::vector<std::future<sl_status_t>> futures;

            component_connector_interview_done_payload_t root_payload;
            root_payload.endpoint_node = session.endpoint_node;
            root_payload.status        = SL_STATUS_OK;
            futures.push_back(connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), root_payload));

            for (const auto &ep_id: session.endpoints.endpoint_ids) {
                auto ep_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, ep_id);
                if (!ep_node.is_valid()) {
                    sl_log_warning(LOG_TAG.data(), "Node %d: endpoint %d node not found in attribute store, skipping interview done notification", session.node_id, ep_id);
                    continue;
                }

                component_connector_interview_done_payload_t ep_payload;
                ep_payload.endpoint_node = ep_node;
                ep_payload.status        = SL_STATUS_OK;
                futures.push_back(connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), ep_payload));
            }

            for (auto &f: futures) {
                f.get();
            }

            // Now install the listener on the NodeID node. It fires once the subtree
            // is fully resolved, at which point on_device_resolution_done publishes
            // the COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED event.
            attribute_resolver_set_resolution_listener(session.device_node, on_device_resolution_done);
        }

        return stay();
    }

}  // namespace zwave_command_class
