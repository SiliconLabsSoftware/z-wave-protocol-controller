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

#include "mqtt_handler.hpp"
#include "init_builder.hpp"
#include "threading.hpp"
#include "log.h"
#include "config.h"
#include "zwave_node_id_definitions.h"
#include <functional>
#include <string>
#include <string_view>
#include <algorithm>
#include <regex>
#include <chrono>
#include <thread>
#include <fmt/format.h>

namespace zwave_component
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "mqtt";
    static constexpr size_t MQTT_PUBLISH_QUEUE_MAX_DEPTH       = 500;

    static std::string stall_diag_topic_snippet(const std::string &topic)
    {
        constexpr size_t max_len = 80;
        if (topic.size() <= max_len) {
            return topic;
        }
        return topic.substr(0, max_len);
    }

    // Helper function for MQTT topic matching
    // Supports # (multi-level wildcard) and + (single-level wildcard)
    bool mqtt_handler::topic_matches_sub(const std::string &sub, const std::string &topic)
    {
        constexpr char MULTI_WILDCARD  = '#';
        constexpr char SINGLE_WILDCARD = '+';
        constexpr char TOPIC_SEPARATOR = '/';

        auto sub_it          = sub.begin();
        auto topic_it        = topic.begin();
        const auto sub_end   = sub.end();
        const auto topic_end = topic.end();

        while (sub_it != sub_end && topic_it != topic_end) {
            if (*sub_it == MULTI_WILDCARD) {
                // Multi-level wildcard matches everything from here
                return true;
            }

            if (*sub_it == SINGLE_WILDCARD) {
                // Single-level wildcard - skip to next separator in both strings
                sub_it   = std::find(sub_it, sub_end, TOPIC_SEPARATOR);
                topic_it = std::find(topic_it, topic_end, TOPIC_SEPARATOR);

                // Skip the separator itself if present
                if (sub_it != sub_end) {
                    ++sub_it;
                }
                if (topic_it != topic_end) {
                    ++topic_it;
                }
                continue;
            }

            // Literal character match
            if (*sub_it == *topic_it) {
                ++sub_it;
                ++topic_it;
            } else {
                return false;
            }
        }

        // Both strings should be exhausted for a match
        return (sub_it == sub_end && topic_it == topic_end);
    }

    // Paho callback implementation
    void mqtt_handler::mqtt_callback::connected(const std::string &cause)
    {
        sl_log_info(LOG_TAG.data(), "Connected to MQTT broker: %s\n", cause.c_str());
        handler_instance->on_connect_internal();
    }

    void mqtt_handler::mqtt_callback::connection_lost(const std::string &cause)
    {
        sl_log_warning(LOG_TAG.data(), "Connection lost: %s\n", cause.c_str());
        handler_instance->on_disconnect_internal();
    }

    void mqtt_handler::mqtt_callback::message_arrived(mqtt::const_message_ptr msg)
    {
        std::string topic   = msg->get_topic();
        std::string payload = msg->to_string();
        handler_instance->handle_message(topic, payload);
    }

    void mqtt_handler::mqtt_callback::delivery_complete(mqtt::delivery_token_ptr tok)
    {
        // Message delivery completed - can be used for QoS > 0 acknowledgments
    }

    mqtt_handler &mqtt_handler::get_instance()
    {
        static mqtt_handler instance;
        return instance;
    }

    mqtt_handler::mqtt_handler() : threading("MQTT Handler")
    {
        // Read config values into temporary C string variables
        const char *client_id_cstr = nullptr;
        const char *mqtt_host_cstr = nullptr;
        const char *cafile_cstr    = nullptr;
        const char *certfile_cstr  = nullptr;
        const char *keyfile_cstr   = nullptr;
        int port_value             = 0;

        if (CONFIG_STATUS_OK != config_get_as_string(CONFIG_KEY_MQTT_CLIENT_ID, &client_id_cstr)) {
            sl_log_error(LOG_TAG.data(), "Failed to get MQTT client ID\n");
            return;
        }
        config.client_id = client_id_cstr;

        if (CONFIG_STATUS_OK != config_get_as_string(CONFIG_KEY_MQTT_HOST, &mqtt_host_cstr)) {
            sl_log_error(LOG_TAG.data(), "Failed to get MQTT host\n");
            return;
        }
        config.mqtt_host = mqtt_host_cstr;

        if (CONFIG_STATUS_OK != config_get_as_int(CONFIG_KEY_MQTT_PORT, &port_value)) {
            sl_log_error(LOG_TAG.data(), "Failed to get MQTT port\n");
            return;
        }
        config.port = port_value;

        if (CONFIG_STATUS_OK != config_get_as_string(CONFIG_KEY_MQTT_CAFILE, &cafile_cstr)) {
            sl_log_error(LOG_TAG.data(), "Failed to get MQTT CA file\n");
            return;
        }
        config.cafile = cafile_cstr;

        if (CONFIG_STATUS_OK != config_get_as_string(CONFIG_KEY_MQTT_CERTFILE, &certfile_cstr)) {
            sl_log_error(LOG_TAG.data(), "Failed to get MQTT cert file\n");
            return;
        }
        config.certfile = certfile_cstr;

        if (CONFIG_STATUS_OK != config_get_as_string(CONFIG_KEY_MQTT_KEYFILE, &keyfile_cstr)) {
            sl_log_error(LOG_TAG.data(), "Failed to get MQTT key file\n");
            return;
        }
        config.keyfile = keyfile_cstr;

        // Build broker URI
        if (!config.cafile.empty() && !config.certfile.empty() && !config.keyfile.empty()) {
            broker_uri = "ssl://" + config.mqtt_host + ":" + std::to_string(config.port);
        } else {
            broker_uri = "tcp://" + config.mqtt_host + ":" + std::to_string(config.port);
        }

        try {
            sl_log_debug(LOG_TAG.data(), "Creating MQTT client: %s\n", config.client_id.c_str());

            // Create Paho async client
            paho_client = std::make_unique<mqtt::async_client>(broker_uri, config.client_id);

            // Create and set callback
            paho_callback = std::make_unique<mqtt_callback>(this);
            paho_client->set_callback(*paho_callback);

            // Configure connection options
            conn_opts.set_clean_session(true);
            conn_opts.set_keep_alive_interval(mqtt_keep_alive_interval);
            conn_opts.set_automatic_reconnect(true);

            // Configure TLS if certificates are provided
            if (!config.cafile.empty() && !config.certfile.empty() && !config.keyfile.empty()) {
                sl_log_debug(LOG_TAG.data(), "Setting Certificate based TLS.\n");
                mqtt::ssl_options sslOpts;
                sslOpts.set_trust_store(config.cafile);
                sslOpts.set_key_store(config.certfile);
                sslOpts.set_private_key(config.keyfile);
                conn_opts.set_ssl(sslOpts);
            } else if (!config.cafile.empty() || !config.certfile.empty() || !config.keyfile.empty()) {
                sl_log_error(LOG_TAG.data(),
                             "One of the TLS certificate configuration file missing."
                             "Certificate based TLS setting failed\n");
                return;
            }

            // Connect to broker
            sl_log_debug(LOG_TAG.data(), "Connecting to MQTT broker: %s\n", broker_uri.c_str());
            paho_client->connect(conn_opts)->wait();

        } catch (const mqtt::exception &exc) {
            sl_log_error(LOG_TAG.data(), "Error setting up MQTT client: %s\n", exc.what());
        }
    }

    sl_status_t mqtt_handler::initialize()
    {
        return SL_STATUS_OK;
    }

    int mqtt_handler::shutdown()
    {
        stop();
        return 0;
    }

    std::string mqtt_handler::name() const
    {
        return "MQTT Handler";
    }

    // Wrapper class for singleton access
    // Inherits from threading::threading so InitBuilder can detect it automatically
    // Overrides start() to start the singleton's thread instead of the wrapper's thread
    class MqttHandlerInitializableWrapper : public threading::threading, public Initializable
    {
        public:
            MqttHandlerInitializableWrapper() : threading::threading("MQTT Handler Wrapper") {}

            sl_status_t initialize() override
            {
                return mqtt_handler::get_instance().initialize();
            }

            int shutdown() override
            {
                mqtt_handler::get_instance().stop();
                return 0;
            }

            std::string name() const override
            {
                return "MQTT Handler";
            }

            void run() override
            {
                // This should never be called since we override start() to start the singleton's thread
            }

            void start() override
            {
                // Start the singleton's thread, not this wrapper's thread
                mqtt_handler::get_instance().start();
            }
    };

    std::unique_ptr<Initializable> mqtt_handler::create_initializable_wrapper()
    {
        return std::make_unique<MqttHandlerInitializableWrapper>();
    }

    mqtt_handler::~mqtt_handler()
    {
        stop();

        // Always attempt a clean disconnect when the client exists.
        // Do NOT gate on connected_.load(): after stop() our handler thread is
        // gone but Paho's auto-reconnect thread is still alive (automatic_reconnect
        // is enabled).  It can flip connected_ at any moment via on_connect_internal
        // / on_disconnect_internal, so a lock-free read here is racy.  Calling
        // disconnect() on an already-disconnected client simply throws, which the
        // catch handles.
        if (paho_client) {
            try {
                paho_client->disconnect()->wait();
            } catch (const mqtt::exception &exc) {
                sl_log_error(LOG_TAG.data(), "Error disconnecting: %s\n", exc.what());
            }
        }
        connected_.store(false);

        // Destroy client before callback to avoid use-after-free if Paho
        // attempts a callback during async_client destruction.
        paho_client.reset();
        paho_callback.reset();
    }

    void mqtt_handler::run()
    {
        if (!paho_client || !connected_.load()) {
            std::this_thread::sleep_for(mqtt_client_poll_interval);
            return;
        }

        // Drain non-timing-critical queues without blocking.
        while (auto msg = reset_queue.try_pop()) {
            reset_subscriptions_internal(msg->new_home_id);
        }
        while (auto msg = subscribe_queue.try_pop()) {
            subscribe_internal(msg->topic, msg->callback);
        }
        while (auto msg = unsubscribe_queue.try_pop()) {
            unsubscribe_internal(msg->topic, msg->callback);
        }
        while (auto msg = unretain_queue.try_pop()) {
            unretain_internal(msg->prefix);
        }

        if (auto msg = publish_queue.pop(static_cast<uint32_t>(mqtt_client_poll_interval.count()))) {
            publish_internal(msg->topic, msg->message, msg->retain);
            while (auto next = publish_queue.try_pop()) {
                publish_internal(next->topic, next->message, next->retain);
            }
        }
    }

    void mqtt_handler::subscribe(const std::string &topic, const std::function<void(const std::string &topic, const std::string &message)> &callback)
    {
        sl_log_info(LOG_TAG.data(), "Subscribing to topic: %s\n", topic.c_str());

        // Prepare message structure
        subscribe_message sub_msg;
        sub_msg.topic    = topic;
        sub_msg.callback = callback;

        // Always queue the subscription for consistent processing
        subscribe_queue.push(std::move(sub_msg));
        sl_log_debug(LOG_TAG.data(), "Queued subscribe message for topic: %s\n", topic.c_str());
    }

    void mqtt_handler::unsubscribe(const std::string &topic, const std::function<void(const std::string &topic, const std::string &message)> &callback)
    {
        sl_log_info(LOG_TAG.data(), "Unsubscribing from topic: %s\n", topic.c_str());

        // Prepare message structure
        unsubscribe_message unsub_msg;
        unsub_msg.topic    = topic;
        unsub_msg.callback = callback;

        // Always queue the unsubscription for consistent processing
        unsubscribe_queue.push(std::move(unsub_msg));
        sl_log_debug(LOG_TAG.data(), "Queued unsubscribe message for topic: %s\n", topic.c_str());
    }

    void mqtt_handler::publish(const std::string &topic, const std::string &message, bool retain)
    {
        sl_log_info(LOG_TAG.data(), "Publishing message to topic: %s\n", topic.c_str());

        // Always queue: callers may be on Paho's I/O thread (e.g. message_arrived
        // → handle_message → callback → publish). Calling paho_client->publish()->wait()
        // on that thread would deadlock because Paho needs the same thread for delivery.
        if (publish_queue.size() >= MQTT_PUBLISH_QUEUE_MAX_DEPTH) {
            publish_queue.try_pop();
            sl_log_debug(LOG_TAG.data(),
                         "MQTT publish queue overflow: queue_depth=%zu — "
                         "dropped oldest message to cap queue",
                         publish_queue.size());
        }

        publish_message pub_msg;
        pub_msg.topic   = topic;
        pub_msg.message = message;
        pub_msg.retain  = retain;

        publish_queue.push(std::move(pub_msg));
    }

    void mqtt_handler::publish_internal(const std::string &topic, const std::string &message, bool retain)
    {
        if (!paho_client || !connected_.load()) {
            sl_log_error(LOG_TAG.data(), "MQTT client not connected\n");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(client_mutex);
            if (retain) {
                if (!message.empty()) {
                    retained_topics.insert(topic);
                } else {
                    retained_topics.erase(topic);
                }
            } else {
                retained_topics.erase(topic);
            }
        }

        const size_t queue_depth        = publish_queue.size();
        const std::string topic_snippet = stall_diag_topic_snippet(topic);
        const auto wait_start           = std::chrono::steady_clock::now();
        if (queue_depth == 0) {
            sl_log_debug(LOG_TAG.data(), "MQTT publish wait start: topic=%s queue_depth=%zu tid=%lu", topic_snippet.c_str(), queue_depth, sl_log_thread_id());
        } else {
            sl_log_debug(LOG_TAG.data(), "MQTT publish wait start: topic=%s queue_depth=%zu tid=%lu", topic_snippet.c_str(), queue_depth, sl_log_thread_id());
        }
        try {
            auto pubmsg = mqtt::make_message(topic, message);
            pubmsg->set_qos(mqtt_qos);
            pubmsg->set_retained(retain);
            paho_client->publish(pubmsg)->wait();
            const auto wait_end = std::chrono::steady_clock::now();
            const auto wait_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(wait_end - wait_start).count();
            sl_log_debug(LOG_TAG.data(), "MQTT publish wait done: topic=%s queue_depth=%zu wait_ms=%lld tid=%lu", topic_snippet.c_str(), publish_queue.size(), static_cast<long long>(wait_ms), sl_log_thread_id());
            sl_log_debug(LOG_TAG.data(), "Published to topic: %s\n", topic.c_str());
        } catch (const mqtt::exception &exc) {
            sl_log_debug(LOG_TAG.data(), "MQTT publish wait fail: topic=%s exc=%s tid=%lu", topic_snippet.c_str(), exc.what(), sl_log_thread_id());
            sl_log_error(LOG_TAG.data(), "Error publishing to topic [%s]: %s", topic.c_str(), exc.what());
        }
    }

    void mqtt_handler::subscribe_internal(const std::string &topic, const subscription_callback_t &callback)
    {
        if (!paho_client) {
            sl_log_error(LOG_TAG.data(), "MQTT client not initialized\n");
            return;
        }

        bool should_subscribe = false;
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            if (callback != nullptr) {
                subscription_callbacks[topic].push_back(callback);
            }
            should_subscribe = (subscription_callbacks.contains(topic));
        }

        if (should_subscribe && connected_.load()) {
            try {
                paho_client->subscribe(topic, mqtt_qos)->wait();
                sl_log_debug(LOG_TAG.data(), "Subscribed to topic: %s\n", topic.c_str());
            } catch (const mqtt::exception &exc) {
                sl_log_error(LOG_TAG.data(), "Error subscribing to topic [%s]: %s\n", topic.c_str(), exc.what());
            }
        }
    }

    void mqtt_handler::unsubscribe_internal(const std::string &topic, const subscription_callback_t &callback)
    {
        if (!paho_client) {
            sl_log_error(LOG_TAG.data(), "MQTT client not initialized\n");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(client_mutex);
            if (!subscription_callbacks.contains(topic)) {
                return;
            }
            subscription_callbacks.erase(topic);
        }

        if (connected_.load()) {
            try {
                paho_client->unsubscribe(topic)->wait();
                sl_log_debug(LOG_TAG.data(), "Unsubscribed from topic: %s\n", topic.c_str());
            } catch (const mqtt::exception &exc) {
                sl_log_error(LOG_TAG.data(), "Error unsubscribing from topic [%s]: %s\n", topic.c_str(), exc.what());
            }
        }
    }

    void mqtt_handler::unretain_internal(const std::string &prefix)
    {
        if (!paho_client || !connected_.load()) {
            sl_log_error(LOG_TAG.data(), "MQTT client not connected\n");
            return;
        }

        std::vector<std::string> topics_to_unretain;
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            for (const auto &topic: retained_topics) {
                if (topic.rfind(prefix, 0) == 0) {
                    topics_to_unretain.push_back(topic);
                }
            }
        }

        for (const auto &topic: topics_to_unretain) {
            if (!connected_.load()) {
                break;
            }
            try {
                auto pubmsg = mqtt::make_message(topic, "");
                pubmsg->set_qos(mqtt_qos);
                pubmsg->set_retained(true);
                paho_client->publish(pubmsg)->wait();

                std::lock_guard<std::mutex> lock(client_mutex);
                retained_topics.erase(topic);
            } catch (const mqtt::exception &exc) {
                sl_log_error(LOG_TAG.data(), "Error unretaining topic [%s]: %s\n", topic.c_str(), exc.what());
            }
        }
    }

    void mqtt_handler::unretain(const std::string &prefix)
    {
        sl_log_info(LOG_TAG.data(), "Unretaining topics with prefix: %s\n", prefix.c_str());

        // Prepare message structure
        unretain_message unretain_msg;
        unretain_msg.prefix = prefix;

        // Always queue the unretain operation for consistent processing
        unretain_queue.push(std::move(unretain_msg));
        sl_log_debug(LOG_TAG.data(), "Queued unretain message for prefix: %s\n", prefix.c_str());
    }

    std::string mqtt_handler::get_client_id() const
    {
        return config.client_id;
    }

    void mqtt_handler::handle_message(const std::string &topic, const std::string &message)
    {
        sl_log_debug(LOG_TAG.data(), "Message arrived: %s - %s...\n", topic.c_str(), message.substr(0, 25).c_str());

        // Collect matching callbacks while holding the lock
        std::vector<std::function<void(const std::string &, const std::string &)>> callbacks_to_invoke;
        {
            std::lock_guard<std::mutex> lock(client_mutex);

            if (subscription_callbacks.empty()) {
                return;
            }

            // Match topic against all subscriptions and collect matching callbacks
            for (const auto &[cb_topic, callbacks]: subscription_callbacks) {
                bool match_topic_result = topic_matches_sub(cb_topic, topic);

                if (match_topic_result) {
                    for (const subscription_callback_t &cb: callbacks) {
                        callbacks_to_invoke.push_back(cb);
                    }
                }
            }
        }

        // Release lock before calling callbacks to prevent deadlock
        // if callbacks try to publish
        const bool is_command_topic = (topic.find("/Command/") != std::string::npos);
        if (is_command_topic) {
            sl_log_debug(LOG_TAG.data(), "MQTT command enter: topic=%s tid=%lu", stall_diag_topic_snippet(topic).c_str(), sl_log_thread_id());
        }
        for (const auto &callback: callbacks_to_invoke) {
            callback(topic, message);
        }
        if (is_command_topic) {
            sl_log_debug(LOG_TAG.data(), "MQTT command exit: topic=%s tid=%lu", stall_diag_topic_snippet(topic).c_str(), sl_log_thread_id());
        }
    }

    void mqtt_handler::on_connect_internal()
    {
        std::lock_guard<std::mutex> lock(client_mutex);

        sl_log_info(LOG_TAG.data(), "MQTT client connected\n");
        connected_.store(true);

        // Queue broker resubscriptions for all previously subscribed topics
        // Callbacks are already registered in subscription_callbacks (preserved on disconnect),
        // so we only need to restore broker subscriptions by queuing with nullptr callback
        // This uses the same queue mechanism as subscribe() for consistent processing
        for (const auto &[topic, callbacks]: subscription_callbacks) {
            subscribe_message sub_msg;
            sub_msg.topic    = topic;
            sub_msg.callback = nullptr;  // nullptr indicates broker-only resubscription
            subscribe_queue.push(std::move(sub_msg));
        }

        // Note: Queued operations (publish, subscribe, unsubscribe, unretain) will be
        // automatically processed by the run() method on the next iteration
    }

    void mqtt_handler::on_disconnect_internal()
    {
        std::lock_guard<std::mutex> lock(client_mutex);

        sl_log_info(LOG_TAG.data(), "MQTT client disconnected\n");
        connected_.store(false);

        // Broker subscriptions are lost on disconnect, but callbacks are preserved in subscription_callbacks
        // On reconnection, we'll resubscribe to all topics that have callbacks registered
    }

    void mqtt_handler::reset_subscriptions(zwave_home_id_t new_home_id)
    {
        sl_log_info(LOG_TAG.data(), "Queueing subscription reset for new home ID: %08X\n", new_home_id);
        reset_subscriptions_message msg;
        msg.new_home_id = new_home_id;
        reset_queue.push(std::move(msg));
    }

    void mqtt_handler::reset_subscriptions_internal(zwave_home_id_t new_home_id)
    {
        if (!paho_client) {
            sl_log_error(LOG_TAG.data(), "MQTT client not initialized\n");
            return;
        }

        std::map<std::string, std::pair<std::string, std::vector<subscription_callback_t>>> topic_mappings;
        {
            std::lock_guard<std::mutex> lock(client_mutex);

            std::regex regex("zpc/([A-F0-9 ]{8})/");
            std::string converted_home_id = fmt::format("{:8X}", new_home_id);

            for (const auto &[topic, callbacks]: subscription_callbacks) {
                std::smatch match;
                if (std::regex_search(topic, match, regex)) {
                    std::string new_topic = topic;
                    size_t pos            = match.position(1);
                    new_topic.replace(pos, match[1].length(), converted_home_id);
                    topic_mappings[topic] = {new_topic, callbacks};
                }
            }
        }

        for (const auto &[old_topic, mapping]: topic_mappings) {
            const auto &[new_topic, callbacks] = mapping;

            if (connected_.load()) {
                try {
                    paho_client->unsubscribe(old_topic)->wait();
                    sl_log_debug(LOG_TAG.data(), "Unsubscribed from topic: %s\n", old_topic.c_str());
                } catch (const mqtt::exception &exc) {
                    sl_log_error(LOG_TAG.data(), "Error unsubscribing from topic [%s]: %s\n", old_topic.c_str(), exc.what());
                }
            }

            {
                std::lock_guard<std::mutex> lock(client_mutex);
                subscription_callbacks.erase(old_topic);
                subscription_callbacks[new_topic] = callbacks;
            }

            if (connected_.load()) {
                try {
                    paho_client->subscribe(new_topic, mqtt_qos)->wait();
                    sl_log_debug(LOG_TAG.data(), "Subscribed to topic: %s\n", new_topic.c_str());
                } catch (const mqtt::exception &exc) {
                    sl_log_error(LOG_TAG.data(), "Error subscribing to topic [%s]: %s\n", new_topic.c_str(), exc.what());
                }
            }
        }
    }
}  // namespace zwave_component

extern "C" void mqtt_handler_reset_subscriptions(zwave_home_id_t new_home_id)
{
    zwave_component::mqtt_handler::get_instance().reset_subscriptions(new_home_id);
}