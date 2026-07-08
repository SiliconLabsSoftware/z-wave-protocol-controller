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

#include "interview_step_s2_commands_supported.hpp"
#include "interview_state_machine.hpp"
#include "component_connector.hpp"
#include "command_class_s2_events.hpp"
#include "command_class_s2_types.hpp"
#include "zwave_controller_utils.h"
#include "ZW_classcmd.h"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    namespace
    {
        // Match Version CC Get retry budget used elsewhere in the interview.
        constexpr uint8_t S2_COMMANDS_SUPPORTED_MAX_TX_RETRIES = 5;

        void fire_s2_commands_supported_get(InterviewSession &session)
        {
            component_connector connector;
            command_class_s2_types::s2_supported_get_payload_t payload_map_s2;
            payload_map_s2.endpoint_id   = 0;
            payload_map_s2.device_node   = session.device_node;
            payload_map_s2.endpoint_node = session.endpoint_node;
            payload_map_s2.zwave_node_id = session.node_id;
            payload_map_s2.granted_keys  = session.granted_keys;
            connector.fire_event(static_cast<uint32_t>(command_class_s2_events_t::COMMAND_CLASS_S2_COMMANDS_SUPPORTED_GET), payload_map_s2);
        }
    }  // namespace

    bool S2CommandsSupportedStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_REPORT || event_type == device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_GET_TX_FAILED;
    }

    StepResult S2CommandsSupportedStep::on_enter(InterviewSession &session)
    {
        const auto &nif = session.node_information_command_class_list;
        if (!is_command_class_in_supported_list(COMMAND_CLASS_SECURITY_2, nif.data(), static_cast<uint8_t>(nif.size()))) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support S2, skipping S2 interview and going directly to VERSION_CC_SEQUENCE", session.node_id);
            return skip();
        }

        if ((session.granted_keys & ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY) == 0 && (session.granted_keys & ZWAVE_CONTROLLER_S2_ACCESS_KEY) == 0 && (session.granted_keys & ZWAVE_CONTROLLER_S2_UNAUTHENTICATED_KEY) == 0) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support S2, skipping S2 interview and going directly to VERSION_CC_SEQUENCE", session.node_id);
            return skip();
        }

        session.s2_commands_supported_tx_retries = 0;
        return stay();
    }

    StepResult S2CommandsSupportedStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            fire_s2_commands_supported_get(session);
            return stay();
        }

        if (event->event == device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_GET_TX_FAILED) {
            ++session.s2_commands_supported_tx_retries;
            if (session.s2_commands_supported_tx_retries >= S2_COMMANDS_SUPPORTED_MAX_TX_RETRIES) {
                sl_log_error(LOG_TAG.data(), "Node %d: S2 Commands Supported Get failed after %u TX attempts, failing interview", session.node_id, session.s2_commands_supported_tx_retries);
                return fail();
            }

            sl_log_warning(LOG_TAG.data(), "Node %d: S2 Commands Supported Get TX failed, retry %u/%u", session.node_id, session.s2_commands_supported_tx_retries, S2_COMMANDS_SUPPORTED_MAX_TX_RETRIES);
            fire_s2_commands_supported_get(session);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<command_class_s2_types::s2_supported_report_payload_t>(event->payload);

            session.s2_supported_command_classes     = payload.supported_cc_list;
            session.s2_commands_supported_tx_retries = 0;

            sl_log_info(LOG_TAG.data(), "Node %d received S2 commands supported report, transitioning to VersionCCSequenceStep", session.node_id);

            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for S2_COMMANDS_SUPPORTED_REPORT");
            return stay(SL_STATUS_FAIL);
        }
    }

}  // namespace zwave_command_class
