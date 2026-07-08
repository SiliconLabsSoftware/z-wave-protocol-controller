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

#include "steps/ota_step_deliver_requested_firmware_chunks.hpp"
#include "mqtt_api_base.hpp"
#include "ota_mqtt_api.hpp"
#include "ota_mqtt_constants.hpp"
#include "command_class_firmware_update_md_events.hpp"
#include "command_class_firmware_update_md_types.hpp"
#include "component_connector.hpp"
#include "log.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <future>
#include <string_view>
#include <thread>

namespace ota
{

    using namespace mqtt_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_deliver_requested_firmware_chunks";

    std::string OtaStepDeliverRequestedFirmwareChunks::name() const
    {
        return "OTA Step Deliver Requested Firmware Chunks";
    }

    bool OtaStepDeliverRequestedFirmwareChunks::handles_external_event(ota_external_event_t event_type) const
    {
        return event_type == ota_external_event_t::FIRMWARE_UPDATE_MD_GET_RECEIVED;
    }

    StepResult OtaStepDeliverRequestedFirmwareChunks::on_enter(OtaSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult OtaStepDeliverRequestedFirmwareChunks::send_abort_md_report(OtaSession &session, uint16_t report_number)
    {
        std::vector<uint8_t> corrupted_data(session.firmware_md.max_fragment_size, 0xFF);

        using zwave_command_class::command_class_firmware_update_md_types::command_class_firmware_update_md_report_payload_t;
        command_class_firmware_update_md_report_payload_t abort_payload;
        abort_payload.endpoint_node = session.endpoint_node;
        abort_payload.report_number = report_number;
        abort_payload.is_last       = true;
        abort_payload.data          = corrupted_data;
        abort_payload.qos_offset    = 1;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_REPORT), abort_payload);

        nlohmann::json progress_report;
        progress_report[key::NODE_ID]      = session.node_id;
        progress_report[key::IMAGE_SIZE]   = session.transfer.image_size;
        progress_report[key::CURRENT_SENT] = session.transfer.bytes_transferred;
        progress_report[key::PERCENTAGE]   = (session.transfer.image_size > 0) ? static_cast<uint32_t>((static_cast<uint64_t>(session.transfer.bytes_transferred) * 100) / session.transfer.image_size) : 0;
        progress_report[key::STATUS]       = status::ABORTED;
        OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_PROGRESS_REPORT_TOPIC, progress_report.dump(), false);

        sl_log_debug(LOG_TAG.data(), "Sent abort report #%u with corrupted data to node %d", report_number, session.node_id);
        return stay();
    }

    StepResult OtaStepDeliverRequestedFirmwareChunks::send_firmware_report_batch(OtaSession &session, uint8_t number_of_reports, uint16_t report_number)
    {
        const uint32_t image_size = static_cast<uint32_t>(session.firmware_image_cache.size());

        component_connector connector;
        for (uint8_t i = 0; i < number_of_reports; ++i) {
            uint16_t current_report = report_number + i;

            uint32_t offset = static_cast<uint32_t>(current_report - 1) * static_cast<uint32_t>(session.firmware_md.max_fragment_size);

            if (offset >= image_size) {
                return done();
            }

            uint32_t remaining    = image_size - offset;
            uint16_t chunk_length = static_cast<uint16_t>(std::min(static_cast<uint32_t>(session.firmware_md.max_fragment_size), remaining));

            bool is_last = (offset + chunk_length) >= image_size;

            const uint32_t qos_offset = static_cast<uint32_t>(number_of_reports - i);

            using zwave_command_class::command_class_firmware_update_md_types::command_class_firmware_update_md_report_payload_t;
            command_class_firmware_update_md_report_payload_t md_report_payload;
            md_report_payload.endpoint_node = session.endpoint_node;
            md_report_payload.report_number = current_report;
            md_report_payload.is_last       = is_last;
            md_report_payload.data          = std::vector<uint8_t>(session.firmware_image_cache.data() + offset, session.firmware_image_cache.data() + offset + chunk_length);
            md_report_payload.qos_offset    = qos_offset;

            auto future             = connector.fire_event_async(static_cast<uint32_t>(command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_REPORT), md_report_payload);
            sl_status_t send_status = future.get();
            if (send_status != SL_STATUS_OK) {
                sl_log_debug(LOG_TAG.data(), "Failed to enqueue report #%u for node %d, stopping batch", current_report, session.node_id);
                break;
            }

            uint32_t new_transferred           = offset + chunk_length;
            session.transfer.bytes_transferred = std::max(new_transferred, session.transfer.bytes_transferred);

            sl_log_debug(LOG_TAG.data(),
                         "Sent report #%u to node %d "
                         "(%u/%u bytes, last=%s)",
                         current_report,
                         session.node_id,
                         session.transfer.bytes_transferred,
                         image_size,
                         is_last ? "yes" : "no");

            // Spec CC:007A.08.06.11.001: when sending more than one report back-to-back,
            // a delay MUST be applied between frames. Minimum is 35 ms at 40 kbit/s and
            // 15 ms at 100 kbit/s. 50 ms covers both with margin.
            constexpr uint32_t INTER_FRAME_DELAY_MS = 50;
            if (i + 1 < number_of_reports) {
                std::this_thread::sleep_for(std::chrono::milliseconds(INTER_FRAME_DELAY_MS));
            }
        }

        return stay();
    }

    StepResult OtaStepDeliverRequestedFirmwareChunks::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        if (!event.has_value()) {
            return stay();
        }

        try {
            const auto &zw_report = std::any_cast<const ZwaveReportPayload &>(event->payload);
            const auto &attr_map  = zw_report.attribute_map;
            auto attr_get_u8      = [&attr_map](const std::string &key, uint8_t def) -> uint8_t {
                auto it = attr_map.find(key);
                return (it != attr_map.end() && std::holds_alternative<uint8_t>(it->second)) ? std::get<uint8_t>(it->second) : def;
            };
            const uint8_t number_of_reports = attr_get_u8("number_of_reports", 1);
            const uint8_t rn1               = attr_get_u8("report_number_1", 0);
            const uint8_t rn2               = attr_get_u8("report_number_2", 0);
            const uint16_t report_number    = static_cast<uint16_t>((static_cast<uint16_t>(rn1) << 8) | rn2);

            sl_log_debug(LOG_TAG.data(),
                         "Node %d requesting %u report(s) "
                         "starting at report #%u",
                         session.node_id,
                         number_of_reports,
                         report_number);

            if (report_number == 0) {
                sl_log_error(LOG_TAG.data(),
                             "Invalid MD Get: report_number is 0 "
                             "(missing or invalid report_number fields) "
                             "for node %d",
                             session.node_id);
                session.upload_in_progress = false;
                session.firmware_image_cache.clear();
                return fail();
            }
            if (number_of_reports == 0) {
                sl_log_error(LOG_TAG.data(),
                             "Invalid MD Get: number_of_reports is 0 "
                             "for node %d",
                             session.node_id);
                session.upload_in_progress = false;
                session.firmware_image_cache.clear();
                return fail();
            }

            if (session.transfer.abort_requested) {
                return send_abort_md_report(session, report_number);
            }

            if (session.firmware_md.max_fragment_size == 0) {
                sl_log_error(LOG_TAG.data(),
                             "max_fragment_size is 0, cannot "
                             "send data");
                session.upload_in_progress = false;
                session.firmware_image_cache.clear();
                return fail();
            }

            return send_firmware_report_batch(session, number_of_reports, report_number);
        } catch (const std::bad_any_cast &) {
            sl_log_debug(LOG_TAG.data(), "Bad payload for MD Get");
        }

        return stay();
    }

}  // namespace ota
