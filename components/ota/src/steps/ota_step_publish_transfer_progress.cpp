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

#include "steps/ota_step_publish_transfer_progress.hpp"
#include "mqtt_api_base.hpp"
#include "ota_mqtt_api.hpp"
#include "ota_mqtt_constants.hpp"
#include "log.h"
#include "nlohmann/json.hpp"

#include <string_view>

namespace ota
{

    using namespace mqtt_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_publish_transfer_progress";

    std::string OtaStepPublishTransferProgress::name() const
    {
        return "OTA Step Publish Transfer Progress";
    }

    bool OtaStepPublishTransferProgress::handles_external_event(ota_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    void OtaStepPublishTransferProgress::publish_progress_report(OtaSession &session)
    {
        if (session.transfer.image_size == 0) {
            return;
        }

        uint32_t percentage = static_cast<uint32_t>((static_cast<uint64_t>(session.transfer.bytes_transferred) * 100) / session.transfer.image_size);

        nlohmann::json progress;
        progress[key::NODE_ID]      = session.node_id;
        progress[key::IMAGE_SIZE]   = session.transfer.image_size;
        progress[key::CURRENT_SENT] = session.transfer.bytes_transferred;
        progress[key::PERCENTAGE]   = percentage;
        OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_PROGRESS_REPORT_TOPIC, progress.dump(), false);

        sl_log_debug(LOG_TAG.data(), "Node %d — %u / %u bytes (%u%%)", session.node_id, session.transfer.bytes_transferred, session.transfer.image_size, percentage);
    }

    StepResult OtaStepPublishTransferProgress::on_enter(OtaSession &session)
    {
        publish_progress_report(session);
        return done();
    }

    StepResult OtaStepPublishTransferProgress::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace ota
