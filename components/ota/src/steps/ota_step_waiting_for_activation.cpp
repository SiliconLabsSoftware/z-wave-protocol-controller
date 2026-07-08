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

#include "steps/ota_step_waiting_for_activation.hpp"
#include "command_class_firmware_update_md_events.hpp"
#include "command_class_firmware_update_md_types.hpp"
#include "component_connector.hpp"
#include "attribute_resolver.h"
#include "log.h"

#include <string_view>

namespace ota
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_waiting_for_activation";

    std::string OtaStepWaitingForActivation::name() const
    {
        return "OTA Step Waiting For Activation";
    }

    bool OtaStepWaitingForActivation::handles_external_event(ota_external_event_t event_type) const
    {
        return event_type == ota_external_event_t::MQTT_ACTIVATE;
    }

    StepResult OtaStepWaitingForActivation::on_enter(OtaSession &session)
    {
        // Resume resolution so the stack can exchange other commands with the device
        // while waiting for the MQTT Activate command. Clear the flag so TransferDone /
        // Failed do not issue a second resume for this pause.
        if (session.resolution_paused) {
            attribute_resolver_resume_node_resolution(session.endpoint_node);
            session.resolution_paused = false;
            sl_log_info(LOG_TAG.data(), "Resumed attribute resolution for node %d", session.node_id);
        }

        sl_log_info(LOG_TAG.data(), "Node %d: firmware downloaded (status 0xFD), waiting for MQTT Activate command", session.node_id);
        return stay();
    }

    StepResult OtaStepWaitingForActivation::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        if (!event.has_value()) {
            return stay();
        }

        sl_log_info(LOG_TAG.data(), "Node %d: MQTT Activate received, sending Firmware Update Activation Set", session.node_id);

        using namespace zwave_command_class::command_class_firmware_update_md_types;

        command_class_firmware_update_md_activation_set_payload_t payload;
        payload.endpoint_node    = session.endpoint_node;
        payload.manufacturer_id  = session.firmware_md.manufacturer_id;
        payload.firmware_id      = session.firmware_md.firmware_id;
        payload.checksum         = session.transfer.firmware_checksum;
        payload.firmware_target  = session.upload.firmware_target;
        payload.hardware_version = session.firmware_md.hardware_version;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_ACTIVATION_SET), payload);

        return done();
    }

}  // namespace ota
