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

#include "steps/ota_step_transfer_done.hpp"
#include "attribute.hpp"
#include "attribute_resolver.h"
#include "log.h"

#include <string_view>

namespace ota
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_transfer_done";

    std::string OtaStepTransferDone::name() const
    {
        return "OTA Step Transfer Done";
    }

    bool OtaStepTransferDone::handles_external_event(ota_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult OtaStepTransferDone::on_enter(OtaSession &session)
    {
        const attribute_store::attribute endpoint = session.endpoint_node;
        const zwave_node_id_t node_id             = session.node_id;
        const bool was_paused                     = session.resolution_paused;

        session = OtaSession(0);

        if (endpoint.is_valid() && was_paused) {
            attribute_resolver_resume_node_resolution(endpoint);
            sl_log_info(LOG_TAG.data(), "Resumed attribute resolution for node %d", node_id);
        }

        return stay();
    }

    StepResult OtaStepTransferDone::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace ota
