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

#include "interview_step_multi_channel_association_supported_groupings.hpp"
#include "interview_state_machine.hpp"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool MultiChannelAssociationSupportedGroupingsStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult MultiChannelAssociationSupportedGroupingsStep::on_enter(InterviewSession &session)
    {
        const bool has_multi_channel_association = std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), 0x8E) != session.version_cc.command_classes_to_query.end();

        sl_log_info(LOG_TAG.data(), "MultiChannelAssociationSupportedGroupings step for node %d: has_multi_channel_association=%d", session.node_id, has_multi_channel_association);

        if (!has_multi_channel_association) {
            return skip();
        }

        return stay();
    }

    StepResult MultiChannelAssociationSupportedGroupingsStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        (void)session;
        if (!event.has_value()) {
            return done();
        }
        return stay();
    }

}  // namespace zwave_command_class
