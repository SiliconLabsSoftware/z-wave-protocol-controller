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

#include "interview_step_zwaveplus_info.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_zwaveplus_info_events.hpp"
#include "command_class_zwaveplus_info_types.hpp"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool ZWavePlusInfoStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::ZWAVEPLUS_INFO_REPORT_RECEIVED;
    }

    StepResult ZWavePlusInfoStep::on_enter(InterviewSession &session)
    {
        if (std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x5E) == session.version_cc.command_classes_to_query.end()) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support Z-Wave Plus Info CC (0x5E), skipping", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult ZWavePlusInfoStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_zwaveplus_info_types::zwaveplus_info_get_payload_t payload;
            payload.device_endpoint_node = session.endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_zwaveplus_info_events_t::COMMAND_CLASS_ZWAVEPLUS_INFO_GET_INTERVIEW), payload);
            return stay();
        }

        try {
            std::any_cast<command_class_zwaveplus_info_types::zwaveplus_info_report_payload_t>(event->payload);
            sl_log_info(LOG_TAG.data(), "Node %d received Z-Wave Plus Info Report", session.node_id);
            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for ZWAVEPLUS_INFO_REPORT_RECEIVED");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
