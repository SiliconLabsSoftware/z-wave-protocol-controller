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

#include "interview_step_s0_commands_supported.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_s0_events.hpp"
#include "command_class_s0_types.hpp"
#include "zwave_controller_utils.h"
#include "ZW_classcmd.h"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool S0CommandsSupportedStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::S0_COMMANDS_SUPPORTED_REPORT;
    }

    StepResult S0CommandsSupportedStep::on_enter(InterviewSession &session)
    {
        const auto &nif = session.node_information_command_class_list;
        if (!is_command_class_in_supported_list(COMMAND_CLASS_SECURITY, nif.data(), static_cast<uint8_t>(nif.size()))) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support S0, skipping S0 interview and going directly to S2 interview", session.node_id);
            return skip();
        }

        if ((session.granted_keys & ZWAVE_CONTROLLER_S0_KEY) == 0) {
            sl_log_info(LOG_TAG.data(), "Node %d was not granted the S0 key, skipping S0 interview", session.node_id);
            return skip();
        }

        // Spec (S2 Commands Supported Get / DT Security Class learning): when any S2 key is
        // granted, S0 is not the highest class — S0 Commands Supported Report is empty and the
        // secure CC list comes from S2. SIS already knows granted keys, so skip the S0 Get.
        constexpr zwave_keyset_t S2_KEYS = ZWAVE_CONTROLLER_S2_ACCESS_KEY | ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY | ZWAVE_CONTROLLER_S2_UNAUTHENTICATED_KEY;
        if ((session.granted_keys & S2_KEYS) != 0) {
            sl_log_info(LOG_TAG.data(), "Node %d: S0 is not the highest granted security class, skipping S0 Commands Supported Get", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult S0CommandsSupportedStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_s0_types::s0_supported_get_payload_t payload_map;
            payload_map.endpoint_id   = 0;
            payload_map.device_node   = session.device_node;
            payload_map.endpoint_node = session.endpoint_node;
            payload_map.zwave_node_id = session.node_id;
            payload_map.granted_keys  = session.granted_keys;
            connector.fire_event(static_cast<uint32_t>(command_class_s0_events_t::COMMAND_CLASS_S0_COMMANDS_SUPPORTED_GET), payload_map);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_s0_types::s0_supported_report_payload_t>(event->payload);

            session.s0_supported_command_classes = payload.supported_cc_list;

            sl_log_info(LOG_TAG.data(), "Node %d received S0 commands supported report, transitioning to S2CommandsSupportedStep", session.node_id);

            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for S0_COMMANDS_SUPPORTED_REPORT");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
