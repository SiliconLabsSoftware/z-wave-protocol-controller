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

#include "interview_step_get_agi_group_count.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "component_connector_types.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_types.hpp"
#include "log.h"
#include <algorithm>
#include <future>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool GetAgiGroupCountStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult GetAgiGroupCountStep::on_enter(InterviewSession &session)
    {
        const bool has_agi = std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x59) != session.version_cc.command_classes_to_query.end();

        if (!has_agi) {
            sl_log_debug(LOG_TAG.data(), "Node %d does not support Association Group Info (0x59), skipping AGI group count", session.node_id);
            return skip();
        }

        if (session.agi.agi_total_groups > 0) {
            sl_log_debug(LOG_TAG.data(), "Node %d already has agi_total_groups=%d, skipping AGI group count", session.node_id, session.agi.agi_total_groups);
            return skip();
        }

        return stay();
    }

    StepResult GetAgiGroupCountStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            const bool has_mca                 = std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x8E) != session.version_cc.command_classes_to_query.end();
            session.agi.agi_used_multi_channel = has_mca;

            component_connector connector;
            if (session.agi.agi_used_multi_channel) {
                component_connector_multi_channel_association_groupings_get_payload_t payload;
                payload.endpoint_node = session.endpoint_node;
                auto future           = connector.fire_event_async<component_connector_multi_channel_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), payload);
                auto [status, result] = future.get();
                session.agi.agi_total_groups = (status == SL_STATUS_OK) ? result : 0;
                sl_log_info(LOG_TAG.data(), "Node %d: AGI group count from Multi Channel Association: %d", session.node_id, session.agi.agi_total_groups);
            } else {
                component_connector_association_groupings_get_payload_t payload;
                payload.endpoint_node        = session.endpoint_node;
                auto future                  = connector.fire_event_async<component_connector_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), payload);
                auto [status, result]        = future.get();
                session.agi.agi_total_groups = (status == SL_STATUS_OK) ? result : 0;
                sl_log_info(LOG_TAG.data(), "Node %d: AGI group count from Association: %d", session.node_id, session.agi.agi_total_groups);
            }
            return done();
        }

        return stay();
    }

}  // namespace zwave_command_class
