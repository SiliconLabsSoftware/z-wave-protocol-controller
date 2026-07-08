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

#include "steps/ota_step_process_device_transfer_outcome.hpp"
#include "mqtt_api_base.hpp"
#include "ota_mqtt_api.hpp"
#include "ota_mqtt_constants.hpp"
#include "log.h"
#include "nlohmann/json.hpp"

#include <string_view>

namespace ota
{

    using namespace mqtt_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_process_device_transfer_outcome";

    std::string OtaStepProcessDeviceTransferOutcome::name() const
    {
        return "OTA Step Process Device Transfer Outcome";
    }

    bool OtaStepProcessDeviceTransferOutcome::handles_external_event(ota_external_event_t event_type) const
    {
        return event_type == ota_external_event_t::FIRMWARE_UPDATE_MD_STATUS_REPORT_RECEIVED;
    }

    StepResult OtaStepProcessDeviceTransferOutcome::on_enter(OtaSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult OtaStepProcessDeviceTransferOutcome::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        if (!event.has_value()) {
            return stay();
        }

        try {
            const auto &zw_report = std::any_cast<const ZwaveReportPayload &>(event->payload);
            const auto &attr_map  = zw_report.attribute_map;

            auto it_status     = attr_map.find("status");
            uint8_t status     = (it_status != attr_map.end() && std::holds_alternative<uint8_t>(it_status->second)) ? std::get<uint8_t>(it_status->second) : 0;
            auto it_waittime   = attr_map.find("waittime");
            uint16_t wait_time = (it_waittime != attr_map.end() && std::holds_alternative<uint16_t>(it_waittime->second)) ? std::get<uint16_t>(it_waittime->second) : 0;

            sl_log_info(LOG_TAG.data(),
                        "Status Report from node %d — "
                        "status=0x%02X, waittime=%u",
                        session.node_id,
                        status,
                        wait_time);

            if (session.transfer.abort_requested && static_cast<FirmwareUpdateMdStatusReport>(status) == FirmwareUpdateMdStatusReport::CHECKSUM_ERROR) {
                sl_log_info(LOG_TAG.data(), "Node %d confirmed abort (checksum error 0x00)", session.node_id);

                nlohmann::json abort_completion;
                abort_completion[key::NODE_ID]      = session.node_id;
                abort_completion[key::IMAGE_SIZE]   = session.transfer.image_size;
                abort_completion[key::CURRENT_SENT] = session.transfer.bytes_transferred;
                abort_completion[key::PERCENTAGE]   = (session.transfer.image_size > 0) ? static_cast<uint32_t>((static_cast<uint64_t>(session.transfer.bytes_transferred) * 100) / session.transfer.image_size) : 0;
                abort_completion[key::STATUS]       = status::ABORTED;
                OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_PROGRESS_REPORT_TOPIC, abort_completion.dump(), false);

                nlohmann::json start_report;
                start_report[key::NODE_ID]    = session.node_id;
                start_report[key::IMAGE_NAME] = session.upload.image_name;
                start_report[key::STATUS]     = status::ABORTED;
                OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_START_FIRMWARE_UPLOAD_REPORT_TOPIC, start_report.dump(), false);

                session.firmware_image_cache.clear();
                return done();
            }

            bool success = true;
            nlohmann::json completion;
            completion[key::NODE_ID]    = session.node_id;
            completion[key::IMAGE_SIZE] = session.transfer.image_size;

            switch (static_cast<FirmwareUpdateMdStatusReport>(status)) {
                case FirmwareUpdateMdStatusReport::SUCCESS:
                    completion[key::STATUS]  = status::SUCCESS;
                    session.wait_time        = wait_time;
                    session.transfer_outcome = OtaTransferOutcome::SUCCESS;
                    break;

                case FirmwareUpdateMdStatusReport::WAIT_FOR_ACTIVATION:
                    completion[key::STATUS]   = status::WAITING_FOR_ACTIVATION;
                    completion[key::WAITTIME] = wait_time;
                    session.wait_time         = wait_time;
                    session.transfer_outcome  = OtaTransferOutcome::WAIT_FOR_ACTIVATION;
                    break;

                case FirmwareUpdateMdStatusReport::STORED_NO_RESTART:
                    completion[key::STATUS]   = status::STORED_NO_RESTART;
                    completion[key::WAITTIME] = wait_time;
                    session.transfer_outcome  = OtaTransferOutcome::STORED_NO_RESTART;
                    break;

                default:
                    completion[key::STATUS]      = status::FAILED;
                    completion[key::STATUS_CODE] = status;
                    success                      = false;
                    break;
            }

            if (success) {
                completion[key::CURRENT_SENT] = session.transfer.image_size;
                completion[key::PERCENTAGE]   = 100;
            } else {
                completion[key::CURRENT_SENT] = session.transfer.bytes_transferred;
                completion[key::PERCENTAGE]   = (session.transfer.image_size > 0) ? static_cast<uint32_t>((static_cast<uint64_t>(session.transfer.bytes_transferred) * 100) / session.transfer.image_size) : 0;
            }

            OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_PROGRESS_REPORT_TOPIC, completion.dump(), false);
            session.firmware_image_cache.clear();
            return success ? done() : fail();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Bad payload for Status Report");
            session.firmware_image_cache.clear();
            return fail();
        }
    }

}  // namespace ota
