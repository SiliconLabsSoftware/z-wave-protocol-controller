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

#include "interview_step_get_endpoint_capabilities.hpp"
#include "interview_state_machine.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "component_connector.hpp"
#include "command_class_multi_channel_types.hpp"
#include "command_class_multi_channel_events.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool GetEndpointCapabilitiesStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT_RECEIVED;
    }

    StepResult GetEndpointCapabilitiesStep::on_enter(InterviewSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult GetEndpointCapabilitiesStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_multi_channel_types::command_class_multi_channel_commands_capability_get_payload_t payload_map;
            payload_map.device_endpoint_node = session.endpoint_node;
            payload_map.endpoint_id          = *session.endpoints.current_endpoint_it;
            connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_GET), payload_map);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_multi_channel_types::command_class_multi_channel_commands_capability_report_payload_t>(event->payload);

            if (payload.endpoint_id != *session.endpoints.current_endpoint_it) {
                sl_log_warning(LOG_TAG.data(), "Unexpected capability report for endpoint %d for node %d (current_endpoint_id=%d)", payload.endpoint_id, session.node_id, *session.endpoints.current_endpoint_it);
                return stay();
            }

            using mc_t   = command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t;
            auto ep_node = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, payload.endpoint_id);
            auto grp     = ep_node.child_by_type(static_cast<attribute_store_type_t>(mc_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP));
            if (grp.is_valid()) {
                auto cc_node = grp.child_by_type(static_cast<attribute_store_type_t>(mc_t::command_class));
                if (cc_node.is_valid() && cc_node.reported_exists()) {
                    auto cc_list = cc_node.reported<command_class_multi_channel_types::multi_channel_capability_report_command_class_t>();
                    session.endpoints.endpoint_discovered_command_classes.insert(session.endpoints.endpoint_discovered_command_classes.end(), cc_list.begin(), cc_list.end());
                }
            }

            ++session.endpoints.current_endpoint_it;

            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end()) {
                sl_log_info(LOG_TAG.data(), "All endpoint capabilities received for node %d", session.node_id);
                return done();
            }

            component_connector connector;
            command_class_multi_channel_types::command_class_multi_channel_commands_capability_get_payload_t payload_map;
            payload_map.device_endpoint_node = session.endpoint_node;
            payload_map.endpoint_id          = *session.endpoints.current_endpoint_it;

            connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_events_t::COMMAND_CLASS_MULTI_CHANNEL_COMMANDS_CAPABILITY_GET), payload_map);

            return stay();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
