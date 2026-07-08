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

#include "steps/ota_step_activating.hpp"
#include "mqtt_api_base.hpp"
#include "ota_mqtt_api.hpp"
#include "ota_mqtt_constants.hpp"
#include "log.h"
#include "nlohmann/json.hpp"

#include <string_view>

namespace ota
{

    using namespace mqtt_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_activating";

    // Firmware Update Activation Status: 0xFF = success (SDS13782 Table 3.97).
    static constexpr uint8_t ACTIVATION_STATUS_SUCCESS = 0xFF;

    std::string OtaStepActivating::name() const
    {
        return "OTA Step Activating";
    }

    bool OtaStepActivating::handles_external_event(ota_external_event_t event_type) const
    {
        return event_type == ota_external_event_t::FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT;
    }

    StepResult OtaStepActivating::on_enter(OtaSession &session)
    {
        sl_log_info(LOG_TAG.data(), "Node %d: Activation Set sent, waiting for Activation Status Report", session.node_id);
        return stay();
    }

    StepResult OtaStepActivating::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        if (!event.has_value()) {
            return stay();
        }

        try {
            const auto &zw_report = std::any_cast<const ZwaveReportPayload &>(event->payload);
            const auto &attr_map  = zw_report.attribute_map;

            auto it_status            = attr_map.find("firmware_update_status");
            uint8_t activation_status = (it_status != attr_map.end() && std::holds_alternative<uint8_t>(it_status->second)) ? std::get<uint8_t>(it_status->second) : 0;

            sl_log_info(LOG_TAG.data(), "Node %d: Activation Status Report — status=0x%02X", session.node_id, activation_status);

            if (activation_status != ACTIVATION_STATUS_SUCCESS) {
                sl_log_error(LOG_TAG.data(), "Node %d: Activation failed (status=0x%02X), aborting OTA", session.node_id, activation_status);

                nlohmann::json report;
                report[key::NODE_ID]     = session.node_id;
                report[key::STATUS]      = status::FAILED;
                report[key::STATUS_CODE] = activation_status;
                OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_PROGRESS_REPORT_TOPIC, report.dump(), false);

                return fail();
            }

            auto it_waittime = attr_map.find("waittime");
            if (it_waittime != attr_map.end() && std::holds_alternative<uint16_t>(it_waittime->second)) {
                session.wait_time = std::get<uint16_t>(it_waittime->second);
            }

            sl_log_info(LOG_TAG.data(), "Node %d: Activation confirmed (wait_time=%u s), proceeding to reconnect check", session.node_id, session.wait_time);

            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Bad payload for Activation Status Report");
            return fail();
        }
    }

}  // namespace ota
