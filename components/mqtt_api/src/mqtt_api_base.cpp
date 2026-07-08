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

#include "mqtt_api_base.hpp"
#include "fmt/format.h"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "mqtt_api_base";

    MqttApiBase::MqttApiBase() {}

    void MqttApiBase::subscribe_topic(std::string_view topic, const std::function<void(const std::string &, const std::string &)> &callback, bool add_base_topic)
    {
        std::string topic_str = add_base_topic ? append_base_topic(topic) : std::string(topic);
        zwave_component::mqtt_handler::get_instance().subscribe(topic_str, callback);
    }

    void MqttApiBase::publish_report(std::string_view topic, const std::string &payload, bool retain, bool add_base_topic)
    {
        std::string topic_str = add_base_topic ? append_base_topic(topic) : std::string(topic);
        zwave_component::mqtt_handler::get_instance().publish(topic_str, payload, retain);
    }

    std::string MqttApiBase::append_base_topic(std::string_view topic)
    {
        return get_base_topic() + "/" + std::string(topic);
    }

    std::string MqttApiBase::get_base_topic()
    {
        zwave_home_id_t home_id = zwave_network_management_get_home_id();
        if (home_id == 0) {
            sl_log_error(LOG_TAG.data(), "Home ID is not set, this will cause unexpected behavior");
            return fmt::format(MQTT_API_BASE_TOPIC, fmt::arg("home_id", 0));
        }
        return fmt::format(MQTT_API_BASE_TOPIC, fmt::arg("home_id", home_id));
    }
}  // namespace zwave_command_class
