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

#include "ota_mqtt_api.hpp"
#include "ota_image_store.hpp"
#include "ota_mqtt_constants.hpp"
#include "log.h"
#include "nlohmann/json.hpp"

#include <string_view>

namespace ota
{

    using namespace mqtt_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_mqtt_api";

    OTAMqttApi::OTAMqttApi(::threading::safe_queue<ota_external_event_data> &event_queue) : event_queue(event_queue) {}

    void OTAMqttApi::setup_mqtt_api()
    {
        zwave_command_class::MqttApiBase::subscribe_topic(MQTT_API_OTA_UPLOAD_IMAGE_TOPIC, [](const std::string &t, const std::string &m) { on_upload_image(t, m); });
        zwave_command_class::MqttApiBase::subscribe_topic(MQTT_API_OTA_START_FIRMWARE_UPLOAD_TOPIC, [this](const std::string &t, const std::string &m) { on_start_firmware_upload(t, m); });
        zwave_command_class::MqttApiBase::subscribe_topic(MQTT_API_OTA_LIST_IMAGES_TOPIC, [](const std::string &t, const std::string &m) { on_list_images(t, m); });
        zwave_command_class::MqttApiBase::subscribe_topic(MQTT_API_OTA_REMOVE_IMAGE_TOPIC, [](const std::string &t, const std::string &m) { on_remove_image(t, m); });
        zwave_command_class::MqttApiBase::subscribe_topic(MQTT_API_OTA_PROGRESS_TOPIC, [this](const std::string &t, const std::string &m) { on_progress(t, m); });
        zwave_command_class::MqttApiBase::subscribe_topic(MQTT_API_OTA_ABORT_TOPIC, [this](const std::string &t, const std::string &m) { on_abort(t, m); });
        zwave_command_class::MqttApiBase::subscribe_topic(MQTT_API_OTA_ACTIVATE_TOPIC, [this](const std::string &t, const std::string &m) { on_activate(t, m); });

        sl_log_info(LOG_TAG.data(), "OTA Firmware Manager MQTT API initialized");
    }

    // ---------------------------------------------------------------------------
    // Command handlers
    // ---------------------------------------------------------------------------

    void OTAMqttApi::on_upload_image(const std::string &topic, const std::string &message)
    {
        nlohmann::json report;

        try {
            auto json_payload = nlohmann::json::parse(message);

            UploadImagePayload payload;
            payload.image_name = json_payload[key::IMAGE_NAME].get<std::string>();

            if (json_payload.contains(key::DATA)) {
                const auto &data_arr = json_payload[key::DATA];
                payload.data.reserve(data_arr.size());
                for (const auto &byte: data_arr) {
                    payload.data.push_back(byte.get<uint8_t>());
                }
            }

            sl_status_t status = ota::OTAImageStore::store_image(payload.image_name, payload.data);

            report[key::IMAGE_NAME] = payload.image_name;
            report[key::STATUS]     = (status == SL_STATUS_OK) ? status::OK : status::ERROR;

        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Failed to parse UploadImage: %s", e.what());
            report[key::STATUS] = status::ERROR;
            report[key::REASON] = e.what();
        }
        publish_report(MQTT_API_OTA_UPLOAD_IMAGE_REPORT_TOPIC, report.dump(), false);
    }

    void OTAMqttApi::on_start_firmware_upload(const std::string & /*topic*/, const std::string &message)
    {
        try {
            auto json_payload = nlohmann::json::parse(message);

            StartFirmwareUploadPayload payload;
            payload.node_id             = json_payload[key::NODE_ID].get<zwave_node_id_t>();
            payload.image_name          = json_payload[key::IMAGE_NAME].get<std::string>();
            payload.wait_for_activation = json_payload[key::WAIT_FOR_ACTIVATION].get<bool>();

            ota_external_event_data ev;
            ev.event   = ota_external_event_t::MQTT_START_UPLOAD;
            ev.node_id = payload.node_id;
            ev.payload = payload;
            event_queue.push(ev);

        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Failed to parse StartFirmwareUpload: %s", e.what());
            nlohmann::json report;
            report[key::STATUS] = status::ERROR;
            report[key::REASON] = e.what();
            publish_report(MQTT_API_OTA_START_FIRMWARE_UPLOAD_REPORT_TOPIC, report.dump(), false);
        }
    }

    void OTAMqttApi::on_list_images(const std::string & /*topic*/, const std::string & /*message*/)
    {
        auto images = ota::OTAImageStore::list_images();

        nlohmann::json report;
        report[key::IMAGES] = images;
        publish_report(MQTT_API_OTA_LIST_IMAGES_REPORT_TOPIC, report.dump(), false);
    }

    void OTAMqttApi::on_remove_image(const std::string & /*topic*/, const std::string &message)
    {
        try {
            auto j = nlohmann::json::parse(message);

            std::string name   = j.value(key::IMAGE_NAME, "");
            sl_status_t status = ota::OTAImageStore::remove_image(name);

            nlohmann::json report;
            report[key::IMAGE_NAME] = name;
            report[key::STATUS]     = (status == SL_STATUS_OK) ? status::OK : status::ERROR;
            publish_report(MQTT_API_OTA_REMOVE_IMAGE_REPORT_TOPIC, report.dump(), false);

        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Failed to parse RemoveImage: %s", e.what());
            nlohmann::json report;
            report[key::STATUS] = status::ERROR;
            report[key::REASON] = e.what();
            publish_report(MQTT_API_OTA_REMOVE_IMAGE_REPORT_TOPIC, report.dump(), false);
        }
    }

    void OTAMqttApi::on_progress(const std::string &topic, const std::string &message)
    {
        ota_external_event_data ev;
        ev.event = ota_external_event_t::MQTT_PROGRESS_REQUEST;
        try {
            auto j     = nlohmann::json::parse(message);
            ev.node_id = j.value(key::NODE_ID, static_cast<zwave_node_id_t>(0));
        } catch (...) {
            ev.node_id = 0;  // fall back: process_event will pick any in-progress session
        }
        event_queue.push(ev);
    }

    void OTAMqttApi::on_abort(const std::string &topic, const std::string &message)
    {
        try {
            auto j = nlohmann::json::parse(message);
            ota_external_event_data ev;
            ev.event   = ota_external_event_t::MQTT_ABORT;
            ev.node_id = j[key::NODE_ID].get<zwave_node_id_t>();
            event_queue.push(ev);
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Failed to parse Abort: %s", e.what());
            nlohmann::json report;
            report[key::STATUS] = status::ERROR;
            report[key::REASON] = e.what();
            publish_report(MQTT_API_OTA_ABORT_REPORT_TOPIC, report.dump(), false);
        }
    }

    void OTAMqttApi::on_activate(const std::string &topic, const std::string &message)
    {
        try {
            auto j = nlohmann::json::parse(message);

            ActivatePayload payload;
            payload.node_id = j[key::NODE_ID].get<zwave_node_id_t>();

            ota_external_event_data ev;
            ev.event   = ota_external_event_t::MQTT_ACTIVATE;
            ev.node_id = payload.node_id;
            ev.payload = payload;
            event_queue.push(ev);

        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Failed to parse Activate: %s", e.what());
            nlohmann::json report;
            report[key::STATUS] = status::ERROR;
            report[key::REASON] = e.what();
            publish_report(MQTT_API_OTA_ACTIVATE_REPORT_TOPIC, report.dump(), false);
        }
    }

}  // namespace ota
