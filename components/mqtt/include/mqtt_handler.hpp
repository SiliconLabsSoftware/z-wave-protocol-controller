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

#ifndef MQTT_HANDLER_HPP
#define MQTT_HANDLER_HPP

#include <atomic>
#include <cstdint>
#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <functional>
#include <memory>
#include <mutex>
#include "threading.hpp"
#include "safe_queue.hpp"
#include "init_builder.hpp"
#include "sl_status.h"
#include "zwave_node_id_definitions.h"

// Paho MQTT C++
#include <mqtt/async_client.h>
#include <mqtt/connect_options.h>
#include <mqtt/ssl_options.h>
#include <mqtt/message.h>
#include <mqtt/callback.h>

namespace zwave_component
{
    class mqtt_handler : public threading::threading, public Initializable
    {
        public:
            static mqtt_handler &get_instance();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

            /**
             * @brief Create an Initializable wrapper for the singleton instance
             *
             * Since mqtt_handler is a singleton, this method returns a wrapper
             * that can be used with InitBuilder.
             *
             * @return Unique pointer to an Initializable wrapper
             */
            static std::unique_ptr<Initializable> create_initializable_wrapper();

            void subscribe(const std::string &topic, const std::function<void(const std::string &topic, const std::string &message)> &callback);
            void unsubscribe(const std::string &topic, const std::function<void(const std::string &topic, const std::string &message)> &callback);
            void publish(const std::string &topic, const std::string &message, bool retain);
            void unretain(const std::string &prefix);
            std::string get_client_id() const;

            void reset_subscriptions(zwave_home_id_t new_home_id);

            // Delete copy constructor and assignment operator
            mqtt_handler(const mqtt_handler &)            = delete;
            mqtt_handler &operator=(const mqtt_handler &) = delete;

        private:
            mqtt_handler();
            ~mqtt_handler();

            // Paho MQTT callback implementation
            class mqtt_callback : public virtual mqtt::callback
            {
                public:
                    mqtt_callback(mqtt_handler *handler) : handler_instance(handler) {}
                    void connected(const std::string &cause) override;
                    void connection_lost(const std::string &cause) override;
                    void message_arrived(mqtt::const_message_ptr msg) override;
                    void delivery_complete(mqtt::delivery_token_ptr tok) override;

                private:
                    mqtt_handler *handler_instance;
            };

            // Subscription callback type (used consistently across queue and storage)
            using subscription_callback_t = std::function<void(const std::string &topic, const std::string &message)>;

            // Publish message queue entry
            struct publish_message {
                    std::string topic;
                    std::string message;
                    bool retain;
            };

            // Subscribe message queue entry
            struct subscribe_message {
                    std::string topic;
                    subscription_callback_t callback;
            };

            // Unsubscribe message queue entry
            struct unsubscribe_message {
                    std::string topic;
                    subscription_callback_t callback;
            };

            // Unretain message queue entry
            struct unretain_message {
                    std::string prefix;
            };

            // Reset subscriptions message queue entry
            struct reset_subscriptions_message {
                    zwave_home_id_t new_home_id;
            };

            // Helper functions
            static bool topic_matches_sub(const std::string &sub, const std::string &topic);
            void handle_message(const std::string &topic, const std::string &message);
            void on_connect_internal();
            void on_disconnect_internal();

            constexpr static std::chrono::milliseconds mqtt_client_poll_interval = std::chrono::milliseconds(100);  // MQTT_CLIENT_POLL_TIMER_MILLISECONDS
            constexpr static int mqtt_keep_alive_interval                        = 60;                              // seconds
            constexpr static int mqtt_qos                                        = 0;

            struct config {
                    std::string client_id;
                    std::string mqtt_host;
                    std::string cafile;
                    std::string certfile;
                    std::string keyfile;
                    uint32_t port;
            } config;

            std::unique_ptr<mqtt::async_client> paho_client;
            std::unique_ptr<mqtt_callback> paho_callback;
            mqtt::connect_options conn_opts;
            std::string broker_uri;

            // Shared state — protected by client_mutex.
            // Accessed from the handler thread (_internal methods) and Paho callback threads.
            std::map<std::string, std::vector<subscription_callback_t>> subscription_callbacks;
            std::set<std::string> retained_topics;
            std::mutex client_mutex;

            // Connection flag — written from Paho callbacks, read from handler thread.
            // Atomic for lock-free reads in run(); avoids calling paho_client->is_connected()
            // which takes Paho's internal mqttasync_mutex.
            std::atomic<bool> connected_ {false};

            // Operation queues — thread-safe, used to pass work to the handler thread.
            // Public methods enqueue; run() dequeues and processes.
            ::threading::safe_queue<publish_message> publish_queue;
            ::threading::safe_queue<subscribe_message> subscribe_queue;
            ::threading::safe_queue<unsubscribe_message> unsubscribe_queue;
            ::threading::safe_queue<unretain_message> unretain_queue;
            ::threading::safe_queue<reset_subscriptions_message> reset_queue;

            // Internal methods — run only on the handler thread.
            // Lock client_mutex briefly for shared-state updates, then release before
            // calling any Paho API. This avoids ABBA deadlock between client_mutex and
            // Paho's internal mqttasync_mutex (held by the receive thread when it
            // delivers messages via handle_message, which also needs client_mutex).
            void publish_internal(const std::string &topic, const std::string &message, bool retain);
            void subscribe_internal(const std::string &topic, const subscription_callback_t &callback);
            void unsubscribe_internal(const std::string &topic, const subscription_callback_t &callback);
            void unretain_internal(const std::string &prefix);
            void reset_subscriptions_internal(zwave_home_id_t new_home_id);

            void run() override;
    };
}  // namespace zwave_component
#endif  // MQTT_HANDLER_HPP