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

#include "interview_step_version_report.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_version_events.hpp"
#include "command_class_version_types.hpp"
#include "zwave_command_class_utils.hpp"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool VersionReportStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::VERSION_REPORT_RECEIVED;
    }

    StepResult VersionReportStep::on_enter(InterviewSession &session)
    {
        if (!command_class_utils::is_version_command_class_in_s2_s0_nif_lists(session.s2_supported_command_classes, session.s0_supported_command_classes, session.node_information_command_class_list)) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support Version CC (0x86), skipping Version Get", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult VersionReportStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_version_types::command_class_version_get_payload_t payload;
            payload.device_endpoint_node = session.endpoint_node;
            connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_GET_INTERVIEW), payload);
            return stay();
        }

        sl_log_info(LOG_TAG.data(), "Node %d received Version Report (library type, protocol version, app version)", session.node_id);
        return done();
    }

}  // namespace zwave_command_class
