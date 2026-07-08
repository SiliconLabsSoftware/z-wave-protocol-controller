/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "steps/ota_step_trigger_interview.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "log.h"

#include <string_view>

namespace ota
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_trigger_interview";

    std::string OtaStepTriggerInterview::name() const
    {
        return "OTA Step Trigger Interview";
    }

    bool OtaStepTriggerInterview::handles_external_event(ota_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult OtaStepTriggerInterview::on_enter(OtaSession &session)
    {
        sl_log_info(LOG_TAG.data(), "Node %d: triggering re-interview after firmware activation", session.node_id);

        using namespace zwave_command_class;

        component_connector_node_interview_requested_payload_t payload;
        payload.node_id = session.node_id;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_INTERVIEW_REQUESTED), payload);

        sl_log_info(LOG_TAG.data(), "Node %d: re-interview triggered", session.node_id);
        return done();
    }

    StepResult OtaStepTriggerInterview::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace ota
