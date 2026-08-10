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
#include "zwave_command_class_mqtt_utils.hpp"
#include "log.h"
#include "sl_status_strings.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>

namespace zwave_command_class
{
    static constexpr std::string_view LOG_TAG = "jamming_detection";

    sl_status_t JammingDetection::initialize()
    {
        {
            std::lock_guard<std::mutex> lock(command_mutex_);
            accepting_commands_ = true;
        }
        zwapi_jamming_register_notification_callback(on_jamming_notification);
        setup_mqtt_api();
        return SL_STATUS_OK;
    }

    int JammingDetection::shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(command_mutex_);
            accepting_commands_ = false;
            commands_.clear();
        }
        // Join any in-flight Serial API command before the NCP is shut down.
        stop();
        zwapi_jamming_register_notification_callback(nullptr);
        return 0;
    }

    std::string JammingDetection::name() const
    {
        return "Jamming Detection";
    }

    void JammingDetection::setup_mqtt_api()
    {
        subscribe_topic(TOPIC_CFG_CHANNEL, [this](const std::string &t, const std::string &m) { enqueue_command(on_channel_config, t, m, TOPIC_CFG_CHANNEL_REPORT); });
        subscribe_topic(TOPIC_CFG_PERIOD, [this](const std::string &t, const std::string &m) { enqueue_command(on_report_period, t, m, TOPIC_CFG_PERIOD_REPORT); });
        subscribe_topic(TOPIC_CFG_RSSI, [this](const std::string &t, const std::string &m) { enqueue_command(on_rssi_collection, t, m, TOPIC_CFG_RSSI_REPORT); });
    }

    void JammingDetection::enqueue_command(CommandHandler handler, const std::string &topic, const std::string &message, std::string_view report_topic)
    {
        sl_status_t status;
        {
            std::lock_guard<std::mutex> lock(command_mutex_);
            if (!accepting_commands_) {
                status = SL_STATUS_NOT_READY;
            } else if (commands_.size() >= MAX_PENDING_COMMANDS) {
                status = SL_STATUS_BUSY;
            } else {
                commands_.push([handler, topic, message] { handler(topic, message); });
                return;
            }
        }
        publish_failure(report_topic, status);
    }

    void JammingDetection::run()
    {
        // Match other component workers: the timeout lets stop() join an idle worker.
        if (auto command = commands_.pop(100)) {
            (*command)();
        }
    }

    namespace
    {
        int required_integer(const nlohmann::json &payload, const char *key, int minimum, int maximum)
        {
            auto it = payload.find(key);
            if (it == payload.end()) {
                throw std::invalid_argument(std::string(key) + " is required");
            }
            int value = command_class_mqtt_utils::parse_numeric<int>(*it);
            if (value < minimum || value > maximum) {
                throw std::out_of_range(std::string(key) + " out of range");
            }
            return value;
        }
    }  // namespace

    void JammingDetection::publish_failure(std::string_view topic, sl_status_t reason)
    {
        std::string reason_text           = sl_status_string(reason);
        constexpr std::string_view prefix = "SL_STATUS_";
        if (reason_text.compare(0, prefix.size(), prefix) == 0) {
            reason_text.erase(0, prefix.size());
        }
        std::transform(reason_text.begin(), reason_text.end(), reason_text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (reason == SL_STATUS_FAIL) {
            reason_text = "operation_failed";
        }
        nlohmann::json report {{"status", MQTT_STATUS_FAIL}, {"reason", reason_text}};
        publish_report(topic, report.dump());
    }

    void JammingDetection::on_channel_config(const std::string & /*topic*/, const std::string &message)
    {
        int channel;
        int threshold;
        int nb;
        try {
            auto payload = nlohmann::json::parse(message);
            channel      = required_integer(payload, "channel", 0, ZWAPI_JAMMING_CHANNEL_COUNT - 1);
            threshold    = required_integer(payload, "threshold_dbm", -128, 0);
            nb           = required_integer(payload, "nb_samples", 1, 150);
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Configuration/Channel: invalid payload: %s\n", e.what());
            publish_failure(TOPIC_CFG_CHANNEL_REPORT, SL_STATUS_INVALID_PARAMETER);
            return;
        }

        uint8_t echoed_channel      = 0;
        int8_t echoed_threshold_dbm = 0;
        uint8_t echoed_nb_samples   = 0;
        sl_status_t status          = zwapi_jamming_set_channel_config((uint8_t)channel, (int8_t)threshold, (uint8_t)nb, &echoed_channel, &echoed_threshold_dbm, &echoed_nb_samples);
        if (status != SL_STATUS_OK) {
            publish_failure(TOPIC_CFG_CHANNEL_REPORT, status);
            return;
        }
        nlohmann::json report {{"status", MQTT_STATUS_SUCCESS}, {"channel", echoed_channel}, {"threshold_dbm", echoed_threshold_dbm}, {"nb_samples", echoed_nb_samples}};
        publish_report(TOPIC_CFG_CHANNEL_REPORT, report.dump());
    }

    void JammingDetection::on_report_period(const std::string & /*topic*/, const std::string &message)
    {
        int period;
        try {
            auto payload = nlohmann::json::parse(message);
            period       = required_integer(payload, "period_secs", 0, 20);
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Configuration/JammingReportPeriod: invalid payload: %s\n", e.what());
            publish_failure(TOPIC_CFG_PERIOD_REPORT, SL_STATUS_INVALID_PARAMETER);
            return;
        }

        uint16_t echoed_period_secs = 0;
        sl_status_t status          = zwapi_jamming_set_report_period((uint16_t)period, &echoed_period_secs);
        if (status != SL_STATUS_OK) {
            publish_failure(TOPIC_CFG_PERIOD_REPORT, status);
            return;
        }
        nlohmann::json report {{"status", MQTT_STATUS_SUCCESS}, {"period_secs", echoed_period_secs}};
        publish_report(TOPIC_CFG_PERIOD_REPORT, report.dump());
    }

    void JammingDetection::on_rssi_collection(const std::string & /*topic*/, const std::string &message)
    {
        int period;
        try {
            auto payload = nlohmann::json::parse(message);
            period       = required_integer(payload, "period_100ms", 0, 65535);
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Configuration/RssiCollection: invalid payload: %s\n", e.what());
            publish_failure(TOPIC_CFG_RSSI_REPORT, SL_STATUS_INVALID_PARAMETER);
            return;
        }

        uint16_t echoed_period_100ms = 0;
        sl_status_t status           = zwapi_jamming_set_rssi_collection((uint16_t)period, &echoed_period_100ms);
        if (status != SL_STATUS_OK) {
            publish_failure(TOPIC_CFG_RSSI_REPORT, status);
            return;
        }
        nlohmann::json report {{"status", MQTT_STATUS_SUCCESS}, {"period_100ms", echoed_period_100ms}};
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
            entry["threshold_dbm"]                   = threshold_dbm;
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
        if (len < ZWAPI_JAMMING_CHANNEL_COUNT) {
            sl_log_warning(LOG_TAG.data(), "RSSI collection report too short (%u bytes)\n", (unsigned)len);
            return;
        }

        nlohmann::json channels = nlohmann::json::array();
        for (uint8_t ch = 0; ch < ZWAPI_JAMMING_CHANNEL_COUNT; ch++) {
            int8_t rssi_dbm = (int8_t)data[ch];
            nlohmann::json entry;
            entry["channel"]  = ch;
            entry["rssi_dbm"] = rssi_dbm;
            channels.push_back(entry);
        }

        nlohmann::json report;
        report["channels"] = channels;
        publish_report(TOPIC_NOTIF_RSSI, report.dump());
    }

}  // namespace zwave_command_class
