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

#include "discovery_mqtt_api.hpp"
#include "log.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include <string>
#include <map>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "discovery_mqtt_api";

    DiscoveryMqttApi::DiscoveryMqttApi() {}

    void DiscoveryMqttApi::setup_mqtt_api()
    {
        // Discovery related commands
        subscribe_topic(std::string(MQTT_API_DISCOVERY_TOPIC), [](const std::string &topic, const std::string &message) { zwave_command_class::DiscoveryMqttApi::on_discovery(topic, message); }, false);
    }

    sl_status_t DiscoveryMqttApi::initialize()
    {
        setup_mqtt_api();
        return SL_STATUS_OK;
    }

    int DiscoveryMqttApi::shutdown()
    {
        return 0;
    }

    std::string DiscoveryMqttApi::name() const
    {
        return "Discovery MQTT API";
    }

    void DiscoveryMqttApi::on_discovery(const std::string &topic, const std::string &message)
    {
        (void)topic;
        (void)message;
        std::map<std::string, std::string> discovery_map;

        std::string home_id = fmt::format("{:8X}", zwave_network_management_get_home_id());
        discovery_map.insert({"home_id", home_id});
        nlohmann::json j_map(discovery_map);
        auto json_str = j_map.dump();

        publish_report(std::string(MQTT_API_DISCOVERY_REPORT_TOPIC), json_str, false, false);
    }
}  // namespace zwave_command_class
