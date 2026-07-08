/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "interview_step_version_zwave_software.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_version_events.hpp"
#include "command_class_version_types.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool VersionZwaveSoftwareInterviewStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::VERSION_ZWAVE_SOFTWARE_REPORT_RECEIVED;
    }

    StepResult VersionZwaveSoftwareInterviewStep::on_enter(InterviewSession &session)
    {
        if (!session.version_zwave_software_supported) {
            sl_log_info(LOG_TAG.data(), "Node %d: Z-Wave Software not indicated in Version Capabilities, skipping Version Z-Wave Software Get", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult VersionZwaveSoftwareInterviewStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_version_types::command_class_version_get_payload_t payload;
            payload.device_endpoint_node = session.endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_ZWAVE_SOFTWARE_GET_INTERVIEW), payload);
            return stay();
        }

        try {
            std::any_cast<command_class_version_types::command_class_version_report_callback_payload_t>(event->payload);
            sl_log_info(LOG_TAG.data(), "Node %d received Version Z-Wave Software Report", session.node_id);
            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for VERSION_ZWAVE_SOFTWARE_REPORT_RECEIVED");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
