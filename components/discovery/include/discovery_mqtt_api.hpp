/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef DISCOVERY_MQTT_API_H
#define DISCOVERY_MQTT_API_H

#include "mqtt_api_base.hpp"
#include <string>
#include <string_view>

namespace zwave_command_class
{
    /**
     * @brief MQTT API for discovery operations
     *
     * Handles MQTT commands for discovery:
     * - Discovery (returns home_id)
     */
    class DiscoveryMqttApi : public MqttApiBase, public Initializable
    {
        public:
            DiscoveryMqttApi();
            ~DiscoveryMqttApi() = default;

            void setup_mqtt_api() override;
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        private:
            inline static std::string MQTT_API_DISCOVERY_TOPIC        = "zpc/Discovery";
            inline static std::string MQTT_API_DISCOVERY_REPORT_TOPIC = MQTT_API_DISCOVERY_TOPIC + "/Report";

            // Topic handlers
            static void on_discovery(const std::string &topic, const std::string &message);
    };
}  // namespace zwave_command_class

#endif  // DISCOVERY_MQTT_API_H
