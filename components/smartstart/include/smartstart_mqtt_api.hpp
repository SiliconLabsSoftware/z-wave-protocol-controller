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

#ifndef SMARTSTART_MQTT_API_H
#define SMARTSTART_MQTT_API_H

#include "mqtt_api_base.hpp"
#include <string>
#include <functional>

namespace zwave_command_class
{
    /**
     * @brief MQTT API for SmartStart operations
     *
     * Handles MQTT commands for SmartStart:
     * - SmartStart Update (replaces the entire provisioning list)
     * - SmartStart Add (appends new entries to the existing list; existing DSKs are skipped)
     * - SmartStart Remove (removes the entries identified by the given DSKs)
     * - SmartStart Clear (purges the entire provisioning list)
     * - SmartStart List (returns the current SmartStart cache)
     */
    class SmartStartMqttApi : public MqttApiBase
    {
        public:
            SmartStartMqttApi();
            ~SmartStartMqttApi() = default;

            void setup_mqtt_api() override;

            /**
             * @brief Set the callback function to handle SmartStart cache updates
             *
             * This callback should route the update through the thread-safe event queue
             * to avoid data races. The callback will be invoked from the MQTT callback thread.
             *
             * @param callback Function that accepts a const std::string& (the message payload)
             */
            void set_cache_update_callback(std::function<void(const std::string &)> callback);

            /**
             * @brief Set the callback function to handle SmartStart cache add (incremental append)
             *
             * Same threading contract as set_cache_update_callback. The payload uses the
             * same shape as Network/SmartStart/Update; entries with DSKs already in the
             * cache are skipped (existing entries are kept untouched).
             *
             * @param callback Function that accepts a const std::string& (the message payload)
             */
            void set_cache_add_callback(std::function<void(const std::string &)> callback);

            /**
             * @brief Set the callback function to handle SmartStart cache remove
             *
             * Same threading contract as set_cache_update_callback. The payload uses the
             * same shape as Network/SmartStart/Update; only the `DSK` field of each entry
             * is read (other fields are ignored). DSKs that are not in the cache are
             * silently skipped.
             *
             * @param callback Function that accepts a const std::string& (the message payload)
             */
            void set_cache_remove_callback(std::function<void(const std::string &)> callback);

            /**
             * @brief Set the callback function to handle SmartStart cache clear
             *
             * The callback should route the request through the thread-safe event queue
             * so that the cache is mutated on the SmartStart handler thread. The MQTT
             * payload is ignored.
             */
            void set_cache_clear_callback(std::function<void()> callback);

            /**
             * @brief Set the callback function to handle SmartStart list requests
             *
             * This callback should route the request through the thread-safe event queue
             * so that the cache is read on the SmartStart handler thread.
             */
            void set_list_request_callback(std::function<void()> callback);

            /**
             * @brief Publish the SmartStart list (cache contents) on Network/SmartStart/List/Report.
             *
             * @param payload Serialized JSON payload (same shape as the Network/SmartStart/Update input)
             */
            static void publish_smartstart_list(const std::string &payload);

        private:
            inline static std::string MQTT_API_NETWORK_SMARTSTART_UPDATE_TOPIC      = "Network/SmartStart/Update";
            inline static std::string MQTT_API_NETWORK_SMARTSTART_ADD_TOPIC         = "Network/SmartStart/Add";
            inline static std::string MQTT_API_NETWORK_SMARTSTART_REMOVE_TOPIC      = "Network/SmartStart/Remove";
            inline static std::string MQTT_API_NETWORK_SMARTSTART_CLEAR_TOPIC       = "Network/SmartStart/Clear";
            inline static std::string MQTT_API_NETWORK_SMARTSTART_LIST_TOPIC        = "Network/SmartStart/List";
            inline static std::string MQTT_API_NETWORK_SMARTSTART_LIST_REPORT_TOPIC = MQTT_API_NETWORK_SMARTSTART_LIST_TOPIC + "/Report";

            // Topic handlers
            void on_smartstart_update(const std::string &topic, const std::string &message);
            void on_smartstart_add(const std::string &topic, const std::string &message);
            void on_smartstart_remove(const std::string &topic, const std::string &message);
            void on_smartstart_clear(const std::string &topic, const std::string &message);
            void on_smartstart_list(const std::string &topic, const std::string &message);

            // Callbacks routed through the SmartStart handler's thread-safe event queue
            std::function<void(const std::string &)> cache_update_callback;
            std::function<void(const std::string &)> cache_add_callback;
            std::function<void(const std::string &)> cache_remove_callback;
            std::function<void()> cache_clear_callback;
            std::function<void()> list_request_callback;
    };
}  // namespace zwave_command_class

#endif  // SMARTSTART_MQTT_API_H
