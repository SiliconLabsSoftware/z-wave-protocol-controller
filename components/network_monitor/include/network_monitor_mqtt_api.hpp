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

#ifndef NETWORK_MONITOR_MQTT_API_H
#define NETWORK_MONITOR_MQTT_API_H

#include "mqtt_api_base.hpp"
#include "network_monitor_network_status.h"
#include <string>

namespace network_monitor
{
    /**
     * @brief MQTT API for network monitor operations
     *
     * Handles MQTT topics for network status reporting.
     */
    class NetworkMonitorMqttApi : public zwave_command_class::MqttApiBase
    {
        public:
            NetworkMonitorMqttApi();
            ~NetworkMonitorMqttApi() = default;

            void setup_mqtt_api() override;

            /**
             * @brief Publish the network status to MQTT (Network/Status)
             *
             * @param network_status The network status to publish
             */
            static void publish_network_status(zwave_node_id_t node_id, NetworkMonitorNetworkStatus network_status);

        private:
            inline static std::string MQTT_API_NETWORK_STATUS_TOPIC        = "Network/Status";
            inline static std::string MQTT_API_NETWORK_STATUS_REPORT_TOPIC = MQTT_API_NETWORK_STATUS_TOPIC + "/Report";
    };
}  // namespace network_monitor

#endif  // NETWORK_MONITOR_MQTT_API_H
