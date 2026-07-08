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

#ifndef MQTT_API_BASE_H
#define MQTT_API_BASE_H

#include "sl_status.h"
#include "zpc_status.hpp"
#include "zwave_network_management.h"
#include "mqtt_handler.hpp"
#include <string>
#include <string_view>
#include <functional>

namespace zwave_command_class
{
    /**
     * @brief Base class for MQTT API implementations
     *
     * Provides common MQTT functionality (subscribe, publish) and helper methods
     * for topic formatting. Components that want to expose MQTT functionality
     * should inherit from this class and implement the setup_mqtt_api() method.
     */
    class MqttApiBase
    {
        public:
            /** MQTT report status strings used across all MQTT API implementations. */
            static constexpr std::string_view MQTT_STATUS_SUCCESS = "success";
            static constexpr std::string_view MQTT_STATUS_FAIL    = "fail";

            MqttApiBase();
            virtual ~MqttApiBase() = default;

            /**
             * @brief Initialize the MQTT API
             *
             * Derived classes must implement this method to set up their
             * specific MQTT topic subscriptions and event handlers.
             */
            virtual void setup_mqtt_api() = 0;

        protected:
            /**
             * @brief Subscribe to an MQTT topic (This method adds the base topic to the topic if add_base_topic is true)
             *
             * @param topic The MQTT topic to subscribe to
             * @param callback The callback function to invoke when a message is received
             * @param add_base_topic Whether to add the base topic to the topic (default: true)
             * @return void
             */
            static void subscribe_topic(std::string_view topic, const std::function<void(const std::string &, const std::string &)> &callback, bool add_base_topic = true);

            /**
             * @brief Publish a report to an MQTT topic (This method adds the base topic to the topic if add_base_topic is true)
             *
             * @param topic The MQTT topic to publish to
             * @param payload The message payload
             * @param retain Whether the message should be retained (default: false)
             * @param add_base_topic Whether to add the base topic to the topic (default: true)
             * @return void
             */
            static void publish_report(std::string_view topic, const std::string &payload, bool retain = false, bool add_base_topic = true);

            /**
             * @brief Get the base topic for this home_id
             *
             * @return std::string The base topic (e.g., "zpc/12345678")
             */
            static std::string get_base_topic();

        private:
            static std::string append_base_topic(std::string_view topic);

        private:
            inline static constexpr std::string_view MQTT_API_BASE_TOPIC = "zpc/{home_id:8X}";
    };
}  // namespace zwave_command_class

#endif  // MQTT_API_BASE_H
