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

#include "smartstart_mqtt_api.hpp"
#include "log.h"
#include <string>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "smartstart_mqtt_api";

    SmartStartMqttApi::SmartStartMqttApi() : cache_update_callback(nullptr), cache_add_callback(nullptr), cache_remove_callback(nullptr), cache_clear_callback(nullptr), list_request_callback(nullptr) {}

    void SmartStartMqttApi::setup_mqtt_api()
    {
        // SmartStart related commands
        subscribe_topic(SmartStartMqttApi::MQTT_API_NETWORK_SMARTSTART_UPDATE_TOPIC, [this](const std::string &topic, const std::string &message) { this->on_smartstart_update(topic, message); });
        subscribe_topic(SmartStartMqttApi::MQTT_API_NETWORK_SMARTSTART_ADD_TOPIC, [this](const std::string &topic, const std::string &message) { this->on_smartstart_add(topic, message); });
        subscribe_topic(SmartStartMqttApi::MQTT_API_NETWORK_SMARTSTART_REMOVE_TOPIC, [this](const std::string &topic, const std::string &message) { this->on_smartstart_remove(topic, message); });
        subscribe_topic(SmartStartMqttApi::MQTT_API_NETWORK_SMARTSTART_CLEAR_TOPIC, [this](const std::string &topic, const std::string &message) { this->on_smartstart_clear(topic, message); });
        subscribe_topic(SmartStartMqttApi::MQTT_API_NETWORK_SMARTSTART_LIST_TOPIC, [this](const std::string &topic, const std::string &message) { this->on_smartstart_list(topic, message); });
    }

    void SmartStartMqttApi::set_cache_update_callback(std::function<void(const std::string &)> callback)
    {
        cache_update_callback = callback;
    }

    void SmartStartMqttApi::set_cache_add_callback(std::function<void(const std::string &)> callback)
    {
        cache_add_callback = callback;
    }

    void SmartStartMqttApi::set_cache_remove_callback(std::function<void(const std::string &)> callback)
    {
        cache_remove_callback = callback;
    }

    void SmartStartMqttApi::set_cache_clear_callback(std::function<void()> callback)
    {
        cache_clear_callback = callback;
    }

    void SmartStartMqttApi::set_list_request_callback(std::function<void()> callback)
    {
        list_request_callback = callback;
    }

    void SmartStartMqttApi::publish_smartstart_list(const std::string &payload)
    {
        publish_report(SmartStartMqttApi::MQTT_API_NETWORK_SMARTSTART_LIST_REPORT_TOPIC, payload, false);
    }

    void SmartStartMqttApi::on_smartstart_update(const std::string &topic, const std::string &message)
    {
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        if (!message.empty()) {
            if (cache_update_callback) {
                // Route the update through the thread-safe event queue to avoid data races
                cache_update_callback(message);
            } else {
                sl_log_warning(LOG_TAG.data(), "Cache update callback not set, dropping SmartStart update");
            }
        }
    }

    void SmartStartMqttApi::on_smartstart_add(const std::string &topic, const std::string &message)
    {
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        if (!message.empty()) {
            if (cache_add_callback) {
                // Route the add through the thread-safe event queue to avoid data races
                cache_add_callback(message);
            } else {
                sl_log_warning(LOG_TAG.data(), "Cache add callback not set, dropping SmartStart add");
            }
        }
    }

    void SmartStartMqttApi::on_smartstart_remove(const std::string &topic, const std::string &message)
    {
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        if (!message.empty()) {
            if (cache_remove_callback) {
                // Route the remove through the thread-safe event queue to avoid data races
                cache_remove_callback(message);
            } else {
                sl_log_warning(LOG_TAG.data(), "Cache remove callback not set, dropping SmartStart remove");
            }
        }
    }

    void SmartStartMqttApi::on_smartstart_clear(const std::string &topic, const std::string &message)
    {
        (void)message;
        sl_log_debug(LOG_TAG.data(), "%s: %s", __func__, topic.c_str());

        if (cache_clear_callback) {
            // Route the clear through the thread-safe event queue to avoid data races
            cache_clear_callback();
        } else {
            sl_log_warning(LOG_TAG.data(), "Cache clear callback not set, dropping SmartStart clear");
        }
    }

    void SmartStartMqttApi::on_smartstart_list(const std::string &topic, const std::string &message)
    {
        (void)message;
        sl_log_debug(LOG_TAG.data(), "%s: %s", __func__, topic.c_str());

        if (list_request_callback) {
            // Route the read through the thread-safe event queue so cache access stays on the SmartStart thread
            list_request_callback();
        } else {
            sl_log_warning(LOG_TAG.data(), "List request callback not set, dropping SmartStart list request");
        }
    }
}  // namespace zwave_command_class
