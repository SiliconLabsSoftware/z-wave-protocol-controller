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

#include "interview_step_check_multi_channel_support.hpp"
#include "interview_state_machine.hpp"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    static constexpr uint8_t MULTI_CHANNEL_CC_ID = 0x60;

    bool CheckMultiChannelSupportStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult CheckMultiChannelSupportStep::on_enter(InterviewSession &session)
    {
        if (std::find(session.version_cc.command_classes_to_query.begin(), session.version_cc.command_classes_to_query.end(), MULTI_CHANNEL_CC_ID) != session.version_cc.command_classes_to_query.end()) {
            sl_log_info(LOG_TAG.data(), "Multi channel support is found for node %d, endpoint %d", session.node_id, session.endpoint_id);
            return done();
        }
        sl_log_info(LOG_TAG.data(), "Multi channel support is not found for node %d, endpoint %d", session.node_id, session.endpoint_id);
        return skip();
    }

    StepResult CheckMultiChannelSupportStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace zwave_command_class
