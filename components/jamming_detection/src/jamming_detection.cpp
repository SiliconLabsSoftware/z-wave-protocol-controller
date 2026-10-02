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

#include "jamming_detection.hpp"
#include "zwapi_jamming.h"
#include "log.h"
#include "nlohmann/json.hpp"
#include <exception>
#include <string>
#include <string_view>

namespace zwave_command_class
{
    static constexpr std::string_view LOG_TAG = "jamming_detection";

    sl_status_t JammingDetection::initialize()
    {
        zwapi_jamming_register_notification_callback(on_jamming_notification);
        setup_mqtt_api();
        return SL_STATUS_OK;
    }

    int JammingDetection::shutdown()
    {
        zwapi_jamming_register_notification_callback(nullptr);
        return 0;
    }

    std::string JammingDetection::name() const
    {
        return "Jamming Detection";
    }

    void JammingDetection::setup_mqtt_api()
    {
        subscribe_topic(TOPIC_CFG_CHANNEL, [](const std::string &t, const std::string &m) { on_channel_config(t, m); });
        subscribe_topic(TOPIC_CFG_PERIOD, [](const std::string &t, const std::string &m) { on_report_period(t, m); });
        subscribe_topic(TOPIC_CFG_RSSI, [](const std::string &t, const std::string &m) { on_rssi_collection(t, m); });
    }

    std::string JammingDetection::status_from_sl(sl_status_t status)
    {
        return (status == SL_STATUS_OK) ? "ok" : "fail";
    }

    void JammingDetection::on_channel_config(const std::string & /*topic*/, const std::string &message)
    {
        nlohmann::json payload;
        try {
            payload = nlohmann::json::parse(message);
        } catch (const nlohmann::json::exception &e) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: invalid JSON: %s\n", e.what());
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"fail"})");
            return;
        }

        auto ch_it      = payload.find("channel");
        auto thresh_it  = payload.find("threshold_dbm");
        auto samples_it = payload.find("nb_samples");

        if (ch_it == payload.end() || !ch_it->is_number_integer()) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: missing or invalid field 'channel'\n");
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"invalid_channel"})");
            return;
        }
        if (thresh_it == payload.end() || !thresh_it->is_string()) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: missing or invalid field 'threshold_dbm'\n");
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"invalid_threshold_dbm"})");
            return;
        }
        if (samples_it == payload.end() || !samples_it->is_number_integer()) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: missing or invalid field 'nb_samples'\n");
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"invalid_nb_samples"})");
            return;
        }

        int channel   = ch_it->get<int>();
        int nb        = samples_it->get<int>();
        int threshold = 0;
        try {
            threshold = std::stoi(thresh_it->get<std::string>());
        } catch (const std::exception &) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: threshold_dbm is not an integer\n");
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"invalid_threshold_dbm"})");
            return;
        }

        if (channel < 0 || channel > 4) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: channel out of range\n");
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"invalid_channel"})");
            return;
        }
        if (threshold < -128 || threshold > 0) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: threshold_dbm out of range\n");
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"invalid_threshold_dbm"})");
            return;
        }
        if (nb < 1 || nb > 150) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: nb_samples out of range\n");
            publish_report(TOPIC_CFG_CHANNEL_REPORT, R"({"status":"invalid_nb_samples"})");
            return;
        }

        uint8_t echoed_channel      = 0;
        int8_t echoed_threshold_dbm = 0;
        uint8_t echoed_nb_samples   = 0;
        sl_status_t status          = zwapi_jamming_set_channel_config((uint8_t)channel, (int8_t)threshold, (uint8_t)nb, &echoed_channel, &echoed_threshold_dbm, &echoed_nb_samples);
        nlohmann::json report;
        report["status"] = status_from_sl(status);
        if (status == SL_STATUS_OK) {
            report["channel"]       = echoed_channel;
            report["threshold_dbm"] = std::to_string(echoed_threshold_dbm);
            report["nb_samples"]    = echoed_nb_samples;
        }
        publish_report(TOPIC_CFG_CHANNEL_REPORT, report.dump());
    }

    void JammingDetection::on_report_period(const std::string & /*topic*/, const std::string &message)
    {
        nlohmann::json payload;
        try {
            payload = nlohmann::json::parse(message);
        } catch (const nlohmann::json::exception &e) {
            sl_log_error(LOG_TAG.data(), "Configuration/JammingReportPeriod: invalid JSON: %s\n", e.what());
            publish_report(TOPIC_CFG_PERIOD_REPORT, R"({"status":"fail"})");
            return;
        }

        auto it = payload.find("period_secs");
        if (it == payload.end() || !it->is_number_integer()) {
            sl_log_error(LOG_TAG.data(), "Configuration/JammingReportPeriod: missing or invalid field 'period_secs'\n");
            publish_report(TOPIC_CFG_PERIOD_REPORT, R"({"status":"invalid_period_secs"})");
            return;
        }

        int period = it->get<int>();
        if (period < 0 || period > 20) {
            sl_log_error(LOG_TAG.data(), "Configuration/JammingReportPeriod: period_secs out of range\n");
            publish_report(TOPIC_CFG_PERIOD_REPORT, R"({"status":"invalid_period_secs"})");
            return;
        }

        uint16_t echoed_period_secs = 0;
        sl_status_t status          = zwapi_jamming_set_report_period((uint16_t)period, &echoed_period_secs);
        nlohmann::json report;
        report["status"] = status_from_sl(status);
        if (status == SL_STATUS_OK) {
            report["period_secs"] = echoed_period_secs;
        }
        publish_report(TOPIC_CFG_PERIOD_REPORT, report.dump());
    }

    void JammingDetection::on_rssi_collection(const std::string & /*topic*/, const std::string &message)
    {
        nlohmann::json payload;
        try {
            payload = nlohmann::json::parse(message);
        } catch (const nlohmann::json::exception &e) {
            sl_log_error(LOG_TAG.data(), "Configuration/RssiCollection: invalid JSON: %s\n", e.what());
            publish_report(TOPIC_CFG_RSSI_REPORT, R"({"status":"fail"})");
            return;
        }

        auto it = payload.find("period_100ms");
        if (it == payload.end() || !it->is_number_integer()) {
            sl_log_error(LOG_TAG.data(), "Configuration/RssiCollection: missing or invalid field 'period_100ms'\n");
            publish_report(TOPIC_CFG_RSSI_REPORT, R"({"status":"invalid_period_100ms"})");
            return;
        }

        int period = it->get<int>();
        if (period < 0 || period > 65535) {
            sl_log_error(LOG_TAG.data(), "Configuration/RssiCollection: period_100ms out of range\n");
            publish_report(TOPIC_CFG_RSSI_REPORT, R"({"status":"invalid_period_100ms"})");
            return;
        }

        uint16_t echoed_period_100ms = 0;
        sl_status_t status           = zwapi_jamming_set_rssi_collection((uint16_t)period, &echoed_period_100ms);
        nlohmann::json report;
        report["status"] = status_from_sl(status);
        if (status == SL_STATUS_OK) {
            report["period_100ms"] = echoed_period_100ms;
        }
        publish_report(TOPIC_CFG_RSSI_REPORT, report.dump());
    }

    void JammingDetection::on_jamming_notification(uint8_t sub_cmd, const uint8_t *data, uint8_t len)
    {
        if (sub_cmd == JAMMING_SUB_CMD_REPORT) {
            publish_jamming_report(data, len);
        } else if (sub_cmd == JAMMING_SUB_CMD_COLLECTION) {
            publish_rssi_collection(data, len);
        } else {
            sl_log_warning(LOG_TAG.data(), "Unexpected jamming sub-command 0x%02x, ignoring\n", sub_cmd);
        }
    }

    void JammingDetection::publish_jamming_report(const uint8_t *data, uint8_t len)
    {
        // Expected layout: [jammed_bitmap, ch0_threshold, ch0_trigger, ch0_samples,
        //                   ch1_threshold, ch1_trigger, ch1_samples, ... (5 channels × 3 bytes)]
        constexpr uint8_t EXPECTED_LEN = 1U + (ZWAPI_JAMMING_CHANNEL_COUNT * 3U);
        if (len < EXPECTED_LEN) {
            sl_log_warning(LOG_TAG.data(), "Jamming report too short (%u bytes, expected %u)\n", (unsigned)len, (unsigned)EXPECTED_LEN);
            return;
        }

        uint8_t jammed_bitmap   = data[0];
        nlohmann::json channels = nlohmann::json::array();

        for (uint8_t ch = 0; ch < ZWAPI_JAMMING_CHANNEL_COUNT; ch++) {
            uint8_t offset        = 1U + (ch * 3U);
            int8_t threshold_dbm  = (int8_t)data[offset];
            uint8_t trigger       = data[offset + 1];
            uint8_t samples_above = data[offset + 2];
            bool jammed           = (jammed_bitmap & (1U << ch)) != 0U;

            nlohmann::json entry;
            entry["channel"]                         = ch;
            entry["jammed"]                          = jammed;
            entry["threshold_dbm"]                   = std::to_string(threshold_dbm);
            entry["nb_samples_trigger_notification"] = trigger;
            entry["samples_above_threshold"]         = samples_above;
            channels.push_back(entry);
        }

        nlohmann::json report;
        report["channels"] = channels;
        publish_report(TOPIC_NOTIF_JAMMING, report.dump());
    }

    void JammingDetection::publish_rssi_collection(const uint8_t *data, uint8_t len)
    {
        // Expected layout: [rssi_ch0, rssi_ch1, rssi_ch2, rssi_ch3, rssi_ch4]
        if (len < ZWAPI_JAMMING_CHANNEL_COUNT) {
            sl_log_warning(LOG_TAG.data(), "RSSI collection report too short (%u bytes)\n", (unsigned)len);
            return;
        }

        nlohmann::json channels = nlohmann::json::array();
        for (uint8_t ch = 0; ch < ZWAPI_JAMMING_CHANNEL_COUNT; ch++) {
            int8_t rssi_dbm = (int8_t)data[ch];
            nlohmann::json entry;
            entry["channel"]  = ch;
            entry["rssi_dbm"] = std::to_string(rssi_dbm);
            channels.push_back(entry);
        }

        nlohmann::json report;
        report["channels"] = channels;
        publish_report(TOPIC_NOTIF_RSSI, report.dump());
    }

}  // namespace zwave_command_class
