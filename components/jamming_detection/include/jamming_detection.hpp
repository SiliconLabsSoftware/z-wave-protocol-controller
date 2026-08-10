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

#ifndef JAMMING_DETECTION_HPP
#define JAMMING_DETECTION_HPP

#include "mqtt_api_base.hpp"
#include "init_builder.hpp"
#include <string>

namespace zwave_command_class
{
    /**
     * @brief Bridges NCP jamming detection to the MQTT API.
     *
     * Subscribes to MQTT configuration topics, forwards them to the NCP via
     * the zwave_api jamming SAPI commands, and publishes reports that include
     * the NCP-echoed configuration on success.
     * Unsolicited jamming and RSSI collection notifications from the NCP are
     * published on the corresponding notification topics.
     *
     * MQTT topics (relative to zpc/<home_id>):
     *   Configuration/Channel             – subscribe
     *   Configuration/Channel/Report      – publish
     *   Configuration/JammingReportPeriod – subscribe
     *   Configuration/JammingReportPeriod/Report – publish
     *   Configuration/RssiCollection      – subscribe
     *   Configuration/RssiCollection/Report – publish
     *   Notification/Jamming              – publish (unsolicited)
     *   Notification/RssiCollection       – publish (unsolicited)
     */
    class JammingDetection : public MqttApiBase, public Initializable
    {
        public:
            JammingDetection()  = default;
            ~JammingDetection() = default;

            void setup_mqtt_api() override;
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        private:
            static constexpr std::string_view TOPIC_CFG_CHANNEL        = "Network/JammingDetection/Configuration/Channel";
            static constexpr std::string_view TOPIC_CFG_CHANNEL_REPORT = "Network/JammingDetection/Configuration/Channel/Report";
            static constexpr std::string_view TOPIC_CFG_PERIOD         = "Network/JammingDetection/Configuration/JammingReportPeriod";
            static constexpr std::string_view TOPIC_CFG_PERIOD_REPORT  = "Network/JammingDetection/Configuration/JammingReportPeriod/Report";
            static constexpr std::string_view TOPIC_CFG_RSSI           = "Network/JammingDetection/Configuration/RssiCollection";
            static constexpr std::string_view TOPIC_CFG_RSSI_REPORT    = "Network/JammingDetection/Configuration/RssiCollection/Report";
            static constexpr std::string_view TOPIC_NOTIF_JAMMING      = "Network/JammingDetection/Notification/Jamming";
            static constexpr std::string_view TOPIC_NOTIF_RSSI         = "Network/JammingDetection/Notification/RssiCollection";

            static void on_channel_config(const std::string &topic, const std::string &message);
            static void on_report_period(const std::string &topic, const std::string &message);
            static void on_rssi_collection(const std::string &topic, const std::string &message);

            static void on_jamming_notification(uint8_t sub_cmd, const uint8_t *data, uint8_t len);
            static void publish_jamming_report(const uint8_t *data, uint8_t len);
            static void publish_rssi_collection(const uint8_t *data, uint8_t len);

            static std::string status_from_sl(sl_status_t status);
    };
}  // namespace zwave_command_class

#endif  // JAMMING_DETECTION_HPP
