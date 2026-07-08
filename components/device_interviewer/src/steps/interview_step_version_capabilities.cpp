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

#include "interview_step_version_capabilities.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_version_events.hpp"
#include "command_class_version_types.hpp"
#include "zwave_command_class_utils.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool VersionCapabilitiesInterviewStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::VERSION_CAPABILITIES_REPORT_RECEIVED;
    }

    StepResult VersionCapabilitiesInterviewStep::on_enter(InterviewSession &session)
    {
        session.version_zwave_software_supported = false;

        if (!command_class_utils::is_version_command_class_in_s2_s0_nif_lists(session.s2_supported_command_classes, session.s0_supported_command_classes, session.node_information_command_class_list)) {
            sl_log_info(LOG_TAG.data(), "Node %d: Version CC (0x86) not in merged capability lists, skipping Version Capabilities Get", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult VersionCapabilitiesInterviewStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_version_types::command_class_version_get_payload_t payload;
            payload.device_endpoint_node = session.endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CAPABILITIES_GET_INTERVIEW), payload);
            return stay();
        }

        try {
            const auto &p                            = std::any_cast<command_class_version_types::command_class_version_capabilities_report_callback_payload_t>(event->payload);
            session.version_zwave_software_supported = (p.z_wave_software != 0);
            sl_log_info(LOG_TAG.data(), "Node %d received Version Capabilities Report (z_wave_software=%u)", session.node_id, static_cast<unsigned>(p.z_wave_software));
            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for VERSION_CAPABILITIES_REPORT_RECEIVED");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
