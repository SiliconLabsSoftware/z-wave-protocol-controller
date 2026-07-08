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

#include "network_monitor_mqtt_api.hpp"
#include "log.h"
#include <nlohmann/json.hpp>
#include <string_view>

namespace network_monitor
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "network_monitor_mqtt_api";

    NetworkMonitorMqttApi::NetworkMonitorMqttApi() = default;

    void NetworkMonitorMqttApi::setup_mqtt_api() {}

    void NetworkMonitorMqttApi::publish_network_status(zwave_node_id_t node_id, NetworkMonitorNetworkStatus network_status)
    {
        const char *status_str = "unknown";
        switch (network_status) {
            case NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL:
                status_str = "online";
                break;
            case NETWORK_MONITOR_NETWORK_STATUS_OFFLINE:
                status_str = "offline";
                break;
            default:
                status_str = "unknown";
                break;
        }

        nlohmann::json payload;
        payload["node_id"] = node_id;
        payload["status"]  = status_str;
        publish_report(MQTT_API_NETWORK_STATUS_REPORT_TOPIC, payload.dump(), false);
    }
}  // namespace network_monitor
