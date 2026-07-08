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

#include "interview_step_prepare_endpoint_versions.hpp"
#include "interview_state_machine.hpp"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool PrepareEndpointVersionsStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult PrepareEndpointVersionsStep::on_enter(InterviewSession &session)
    {
        if (session.endpoints.endpoint_discovered_command_classes.empty()) {
            sl_log_info(LOG_TAG.data(), "Node %d: no new endpoint CCs discovered, skipping endpoint version queries", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult PrepareEndpointVersionsStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            std::vector<uint8_t> new_ccs;
            for (const auto &cc: session.endpoints.endpoint_discovered_command_classes) {
                if (cc == 0x00 || cc == 0x20) {
                    sl_log_info(LOG_TAG.data(), "Node %d: skipping versioning for CC 0x%02X", session.node_id, cc);
                    continue;
                }
                if (std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), cc) != session.version_cc.command_classes_to_query.end()) {
                    continue;
                }
                if (std::find(new_ccs.begin(), new_ccs.end(), cc) != new_ccs.end()) {
                    continue;
                }
                new_ccs.push_back(cc);
            }

            if (new_ccs.empty()) {
                sl_log_info(LOG_TAG.data(), "Node %d: all endpoint CCs already versioned, skipping", session.node_id);
                return skip();
            }

            sl_log_info(LOG_TAG.data(), "Node %d: %zu new endpoint CCs to version", session.node_id, new_ccs.size());

            session.version_cc.command_classes_to_query = new_ccs;
            session.version_cc.current_cc_it            = session.version_cc.command_classes_to_query.begin();

            return done();
        }

        return stay();
    }

}  // namespace zwave_command_class
