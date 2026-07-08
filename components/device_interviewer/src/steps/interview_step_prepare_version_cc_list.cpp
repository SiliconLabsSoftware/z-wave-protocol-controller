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

#include "interview_step_prepare_version_cc_list.hpp"
#include "interview_state_machine.hpp"
#include "log.h"
#include "zwave_command_class_utils.hpp"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool PrepareVersionCCListStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult PrepareVersionCCListStep::on_enter(InterviewSession &session)
    {
        if (!command_class_utils::is_version_command_class_in_s2_s0_nif_lists(session.s2_supported_command_classes, session.s0_supported_command_classes, session.node_information_command_class_list)) {
            sl_log_info(LOG_TAG.data(), "Node %d does not support Version CC, skipping Version CC list preparation", session.node_id);
            return skip();
        }

        return stay();
    }

    StepResult PrepareVersionCCListStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            std::vector<uint8_t> all_supported_command_classes;

            std::vector<uint8_t> filtered_s2_command_classes  = command_class_utils::get_normal_command_classes(session.s2_supported_command_classes);
            std::vector<uint8_t> filtered_s0_command_classes  = command_class_utils::get_normal_command_classes(session.s0_supported_command_classes);
            std::vector<uint8_t> filtered_nif_command_classes = command_class_utils::get_normal_command_classes(session.node_information_command_class_list);

            all_supported_command_classes.insert(all_supported_command_classes.end(), filtered_s2_command_classes.begin(), filtered_s2_command_classes.end());
            all_supported_command_classes.insert(all_supported_command_classes.end(), filtered_s0_command_classes.begin(), filtered_s0_command_classes.end());
            all_supported_command_classes.insert(all_supported_command_classes.end(), filtered_nif_command_classes.begin(), filtered_nif_command_classes.end());

            std::sort(all_supported_command_classes.begin(), all_supported_command_classes.end());
            all_supported_command_classes.erase(std::unique(all_supported_command_classes.begin(), all_supported_command_classes.end()), all_supported_command_classes.end());
            all_supported_command_classes.erase(std::remove(all_supported_command_classes.begin(), all_supported_command_classes.end(), 0x00), all_supported_command_classes.end());
            all_supported_command_classes.erase(std::remove(all_supported_command_classes.begin(), all_supported_command_classes.end(), 0x20), all_supported_command_classes.end());

            session.version_cc.command_classes_to_query = all_supported_command_classes;
            session.version_cc.current_cc_it            = session.version_cc.command_classes_to_query.begin();

            if (session.version_cc.current_cc_it == session.version_cc.command_classes_to_query.end()) {
                sl_log_warning(LOG_TAG.data(), "No valid command classes to query for node %d, endpoint %d", session.node_id, session.endpoint_id);
            }

            return done();
        }

        return stay();
    }

}  // namespace zwave_command_class
