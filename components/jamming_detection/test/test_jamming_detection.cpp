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
 *****************************************************************************/

#include "jamming_detection.hpp"
#include "zwapi_jamming.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <array>
#include <map>
#include <vector>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

using nlohmann::json;
using zwave_command_class::JammingDetection;
using zwave_command_class::MqttApiBase;

namespace
{
    using Callback = std::function<void(const std::string &, const std::string &)>;
    std::map<std::string, Callback> subscriptions;
    std::vector<std::pair<std::string, json>> reports;
    zwapi_jamming_notification_callback_t notification_callback;
    sl_status_t command_status;
    unsigned command_calls;
    std::vector<int> sent_values;
    std::mutex command_mutex;
    std::condition_variable command_changed;
    bool block_command   = false;
    bool command_entered = false;
    bool release_command = false;
    std::thread::id command_thread;
    constexpr auto ASYNC_TIMEOUT = std::chrono::seconds(2);
    constexpr auto PREFIX        = "Network/JammingDetection/";
    constexpr std::array<const char *, 3> COMMANDS {"Configuration/Channel", "Configuration/JammingReportPeriod", "Configuration/RssiCollection"};
}  // namespace

// Capture the public MQTT boundary without a broker or serial device.
namespace zwave_command_class
{
    MqttApiBase::MqttApiBase() = default;

    void MqttApiBase::subscribe_topic(std::string_view topic, const Callback &callback, bool add_base_topic)
    {
        EXPECT_TRUE(add_base_topic);
        subscriptions[std::string(topic)] = callback;
    }

    void MqttApiBase::publish_report(std::string_view topic, const std::string &payload, bool retain, bool add_base_topic)
    {
        EXPECT_FALSE(retain);
        EXPECT_TRUE(add_base_topic);
        reports.emplace_back(topic, json::parse(payload));
    }
}  // namespace zwave_command_class

extern "C" {
void zwapi_jamming_register_notification_callback(zwapi_jamming_notification_callback_t callback)
{
    notification_callback = callback;
}

sl_status_t zwapi_jamming_set_channel_config(uint8_t channel, int8_t threshold, uint8_t samples, uint8_t *echo_channel, int8_t *echo_threshold, uint8_t *echo_samples)
{
    {
        std::unique_lock<std::mutex> lock(command_mutex);
        command_thread  = std::this_thread::get_id();
        command_entered = true;
        command_changed.notify_all();
        command_changed.wait(lock, [] { return !block_command || release_command; });
    }
    ++command_calls;
    sent_values     = {channel, threshold, samples};
    *echo_channel   = 2;
    *echo_threshold = -60;
    *echo_samples   = 90;
    return command_status;
}

sl_status_t zwapi_jamming_set_report_period(uint16_t period, uint16_t *echo)
{
    ++command_calls;
    sent_values = {period};
    *echo       = period;
    return command_status;
}

sl_status_t zwapi_jamming_set_rssi_collection(uint16_t period, uint16_t *echo)
{
    ++command_calls;
    sent_values = {period};
    *echo       = period;
    return command_status;
}
}

class JammingDetectionTest : public ::testing::Test
{
    protected:
        JammingDetection component;

        void SetUp() override
        {
            block_command = command_entered = release_command = false;
            subscriptions.clear();
            reports.clear();
            sent_values.clear();
            command_status = SL_STATUS_OK;
            command_calls  = 0;
            ASSERT_EQ(component.initialize(), SL_STATUS_OK);
            ASSERT_EQ(subscriptions.size(), COMMANDS.size());
        }

        void TearDown() override
        {
            EXPECT_EQ(component.shutdown(), 0);
            EXPECT_EQ(notification_callback, nullptr);
        }

        void request(const char *command, const std::string &payload)
        {
            reports.clear();
            const auto topic = std::string(PREFIX) + command;
            subscriptions.at(topic)(topic, payload);
            // Drive one worker iteration deterministically; threading is tested separately.
            component.run();
            ASSERT_EQ(reports.size(), 1U);
            EXPECT_EQ(reports.back().first, topic + "/Report");
        }

        void expect_invalid(const char *command, const std::string &payload)
        {
            const auto before = command_calls;
            request(command, payload);
            ASSERT_EQ(reports.size(), 1U);
            EXPECT_EQ(reports.back().second, (json {{"status", "fail"}, {"reason", "invalid_parameter"}}));
            EXPECT_TRUE(reports.back().second.at("reason").is_string());
            EXPECT_EQ(command_calls, before);
        }

        static json valid_payload(size_t command)
        {
            if (command == 0) {
                return {{"channel", 3}, {"threshold_dbm", -50}, {"nb_samples", 95}};
            }
            return command == 1 ? json {{"period_secs", 5}} : json {{"period_100ms", 100}};
        }
};

// Success reports expose the NCP echo as numeric fields with the shared status.
TEST_F(JammingDetectionTest, SuccessReportsUseSharedStatusAndEchoedNumericValues)
{
    request(COMMANDS[0], valid_payload(0).dump());
    EXPECT_EQ(reports.back().second, (json {{"status", "success"}, {"channel", 2}, {"threshold_dbm", -60}, {"nb_samples", 90}}));
    EXPECT_TRUE(reports.back().second.at("threshold_dbm").is_number_integer());
    EXPECT_EQ(sent_values, (std::vector<int> {3, -50, 95}));
    for (size_t command = 1; command < COMMANDS.size(); ++command) {
        auto expected      = valid_payload(command);
        expected["status"] = "success";
        request(COMMANDS[command], valid_payload(command).dump());
        EXPECT_EQ(reports.back().second, expected);
    }
    EXPECT_EQ(command_calls, 3U);
}

// Numeric strings are accepted at input and normalized to JSON numbers in reports.
TEST_F(JammingDetectionTest, CommandsAcceptDecimalAndHexStringsAndPublishNumbers)
{
    const std::array<std::vector<json>, 3> inputs {{
      {json {{"channel", "3"}, {"threshold_dbm", "-50"}, {"nb_samples", "95"}}, json {{"channel", "0x03"}, {"threshold_dbm", -50}, {"nb_samples", "0x5F"}}},
      {json {{"period_secs", "5"}}, json {{"period_secs", "0x05"}}},
      {json {{"period_100ms", "100"}}, json {{"period_100ms", "0x64"}}},
    }};
    const std::array<std::vector<int>, 3> expected_values {{{3, -50, 95}, {5}, {100}}};
    for (size_t command = 0; command < COMMANDS.size(); ++command) {
        for (const auto &payload: inputs[command]) {
            request(COMMANDS[command], payload.dump());
            EXPECT_EQ(sent_values, expected_values[command]);
            EXPECT_EQ(reports.back().second.at("status"), "success");
            EXPECT_FALSE(reports.back().second.contains("reason"));
            for (const auto &[key, value]: reports.back().second.items()) {
                if (key != "status") {
                    EXPECT_TRUE(value.is_number_integer()) << key;
                }
            }
        }
    }
    EXPECT_EQ(command_calls, 6U);
}

// Each transport status maps to a readable failure on the matching report topic.
TEST_F(JammingDetectionTest, EveryCommandPublishesReadableSerialApiFailureReasons)
{
    const std::pair<sl_status_t, const char *> failures[] {{SL_STATUS_FAIL, "operation_failed"},
                                                           {SL_STATUS_TIMEOUT, "timeout"},
                                                           {SL_STATUS_NOT_SUPPORTED, "not_supported"},
                                                           {SL_STATUS_BUSY, "busy"},
                                                           {SL_STATUS_NOT_READY, "not_ready"},
                                                           {SL_STATUS_ABORT, "abort"},
                                                           {SL_STATUS_PERMISSION, "permission"},
                                                           {SL_STATUS_INVALID_PARAMETER, "invalid_parameter"},
                                                           {static_cast<sl_status_t>(0xffff), "unknown"}};
    for (const auto &[status, reason]: failures) {
        command_status = status;
        for (size_t command = 0; command < COMMANDS.size(); ++command) {
            request(COMMANDS[command], valid_payload(command).dump());
            EXPECT_EQ(reports.back().second, (json {{"status", "fail"}, {"reason", reason}}));
            EXPECT_TRUE(reports.back().second.at("reason").is_string());
        }
    }
    EXPECT_EQ(command_calls, 27U);
}

// Malformed JSON and non-object payloads fail without sending any command.
TEST_F(JammingDetectionTest, MalformedAndNonObjectPayloadsReturnInvalidParameter)
{
    for (auto command: COMMANDS) {
        for (const auto *payload: {"", "{", "{}", "null", "[]", "42", "true", "\"text\""}) {
            SCOPED_TRACE(payload);
            expect_invalid(command, payload);
        }
    }
}

// Required fields reject wrong types, partial numeric strings, and integer overflow.
TEST_F(JammingDetectionTest, FieldsRejectWrongTypesMissingValuesAndIntegerOverflow)
{
    const std::vector<std::vector<const char *>> fields {{"channel", "threshold_dbm", "nb_samples"}, {"period_secs"}, {"period_100ms"}};
    const std::vector<json> invalid {nullptr,
                                     true,
                                     1.5,
                                     "",
                                     "text",
                                     "0x",
                                     "0x1junk",
                                     "-50garbage",
                                     "-50.5",
                                     json::array(),
                                     json::object(),
                                     json::parse("4294967296"),
                                     json::parse("4294967297"),
                                     json::parse("-4294967296"),
                                     json::parse("18446744073709551615"),
                                     json::parse("-9223372036854775808"),
                                     "4294967296",
                                     "-4294967296",
                                     "18446744073709551615",
                                     "-9223372036854775808"};
    for (size_t command = 0; command < COMMANDS.size(); ++command) {
        for (auto field: fields[command]) {
            SCOPED_TRACE(field);
            auto payload = valid_payload(command);
            payload.erase(field);
            expect_invalid(COMMANDS[command], payload.dump());
            for (const auto &value: invalid) {
                payload[field] = value;
                expect_invalid(COMMANDS[command], payload.dump());
            }
        }
    }
}

// Each documented field accepts its endpoints and rejects values outside its range.
TEST_F(JammingDetectionTest, ValidatesEveryFieldAtItsBoundaries)
{
    struct Boundary {
            size_t command;
            const char *field;
            int minimum;
            int maximum;
    };
    for (const auto &boundary: {Boundary {0, "channel", 0, 4}, Boundary {0, "threshold_dbm", -128, 0}, Boundary {0, "nb_samples", 1, 150}, Boundary {1, "period_secs", 0, 20}, Boundary {2, "period_100ms", 0, 65535}}) {
        SCOPED_TRACE(boundary.field);
        auto payload = valid_payload(boundary.command);
        for (int value: {boundary.minimum - 1, boundary.maximum + 1}) {
            payload[boundary.field] = value;
            expect_invalid(COMMANDS[boundary.command], payload.dump());
            payload[boundary.field] = std::to_string(value);
            expect_invalid(COMMANDS[boundary.command], payload.dump());
        }
        for (int value: {boundary.minimum, boundary.maximum}) {
            payload[boundary.field] = value;
            const auto before       = command_calls;
            request(COMMANDS[boundary.command], payload.dump());
            EXPECT_EQ(reports.back().second.at("status"), "success");
            EXPECT_EQ(command_calls, before + 1);
            const size_t index = boundary.command == 0 ? (std::string(boundary.field) == "channel" ? 0 : std::string(boundary.field) == "threshold_dbm" ? 1 : 2) : 0;
            EXPECT_EQ(sent_values.at(index), value);
            payload[boundary.field] = std::to_string(value);
            request(COMMANDS[boundary.command], payload.dump());
            EXPECT_EQ(reports.back().second.at("status"), "success");
            EXPECT_EQ(command_calls, before + 2);
            EXPECT_EQ(sent_values.at(index), value);
        }
    }
}

// Notification bytes preserve signed dBm values and the channel jamming bitmap.
TEST_F(JammingDetectionTest, NotificationsPublishSignedNumericRssiAndThreshold)
{
    const uint8_t jamming[] {1, 206, 95, 101, 128, 0, 0, 0, 0, 0, 255, 1, 1, 200, 2, 3};
    notification_callback(JAMMING_SUB_CMD_REPORT, jamming, sizeof(jamming));
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.back().first, std::string(PREFIX) + "Notification/Jamming");
    const auto channels = reports.back().second.at("channels");
    ASSERT_EQ(channels.size(), 5U);
    EXPECT_TRUE(channels[0].at("threshold_dbm").is_number_integer());
    EXPECT_EQ(channels[0].at("threshold_dbm"), -50);
    EXPECT_EQ(channels[1].at("threshold_dbm"), -128);
    EXPECT_EQ(channels[0].at("jammed"), true);
    EXPECT_EQ(channels[0].at("samples_above_threshold"), 101);
    const uint8_t rssi[] {154, 153, 153, 159, 128};
    notification_callback(JAMMING_SUB_CMD_COLLECTION, rssi, sizeof(rssi));
    ASSERT_EQ(reports.size(), 2U);
    EXPECT_EQ(reports.back().first, std::string(PREFIX) + "Notification/RssiCollection");
    EXPECT_TRUE(reports.back().second.at("channels")[0].at("rssi_dbm").is_number_integer());
    EXPECT_EQ(reports.back().second.at("channels")[0].at("rssi_dbm"), -102);
    EXPECT_EQ(reports.back().second.at("channels")[4].at("rssi_dbm"), -128);
}

// Incomplete payloads and unknown subcommands never publish a notification.
TEST_F(JammingDetectionTest, ShortAndUnknownNotificationsAreIgnored)
{
    const uint8_t data[] {0};
    notification_callback(JAMMING_SUB_CMD_REPORT, data, sizeof(data));
    notification_callback(JAMMING_SUB_CMD_COLLECTION, data, sizeof(data));
    notification_callback(0xff, data, sizeof(data));
    EXPECT_TRUE(reports.empty());
}

// A held Serial API response must not hold Paho's configuration callback.
TEST_F(JammingDetectionTest, ConfigurationCallbackReturnsWhileSerialCommandIsBlocked)
{
    block_command = true;
    component.start();
    const auto topic = std::string(PREFIX) + COMMANDS[0];
    // Use another thread to represent Paho so a regression can be released and joined.
    std::thread::id callback_thread;
    auto callback              = std::async(std::launch::async, [&] {
        callback_thread = std::this_thread::get_id();
        subscriptions.at(topic)(topic, valid_payload(0).dump());
    });
    const auto callback_status = callback.wait_for(ASYNC_TIMEOUT);
    bool entered;
    {
        std::unique_lock<std::mutex> lock(command_mutex);
        entered         = command_changed.wait_for(lock, ASYNC_TIMEOUT, [] { return command_entered; });
        release_command = true;
    }
    command_changed.notify_all();
    callback.get();
    // Joining completes all fake writes before checking the report and thread IDs.
    EXPECT_EQ(component.shutdown(), 0);
    EXPECT_EQ(callback_status, std::future_status::ready);
    ASSERT_TRUE(entered);
    EXPECT_NE(command_thread, callback_thread);
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.back().second.at("status"), "success");
}

// Queued commands execute in arrival order rather than on the MQTT caller's thread.
TEST_F(JammingDetectionTest, ConfigurationCommandsAreQueuedInOrder)
{
    for (size_t command = 0; command < COMMANDS.size(); ++command) {
        const auto topic = std::string(PREFIX) + COMMANDS[command];
        subscriptions.at(topic)(topic, valid_payload(command).dump());
    }
    EXPECT_EQ(command_calls, 0U);
    EXPECT_TRUE(reports.empty());
    for (size_t command = 0; command < COMMANDS.size(); ++command) {
        component.run();
        ASSERT_EQ(reports.size(), command + 1);
        EXPECT_EQ(reports.back().first, std::string(PREFIX) + COMMANDS[command] + "/Report");
    }
}

// Backpressure rejects excess work without dropping commands already accepted.
TEST_F(JammingDetectionTest, FullQueueReturnsBusy)
{
    constexpr size_t QUEUE_CAPACITY = 16;  // Documented maximum pending configuration commands.
    const auto topic                = std::string(PREFIX) + COMMANDS[1];
    for (size_t command = 0; command < QUEUE_CAPACITY; ++command) {
        subscriptions.at(topic)(topic, valid_payload(1).dump());
    }
    EXPECT_TRUE(reports.empty());
    subscriptions.at(topic)(topic, valid_payload(1).dump());
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.back().second, (json {{"status", "fail"}, {"reason", "busy"}}));
    reports.clear();
    for (size_t command = 0; command < QUEUE_CAPACITY; ++command) {
        component.run();
    }
    EXPECT_EQ(command_calls, QUEUE_CAPACITY);
    EXPECT_EQ(reports.size(), QUEUE_CAPACITY);
}

// Shutdown discards pending work and rejects subsequent commands without using the NCP.
TEST_F(JammingDetectionTest, ShutdownRejectsNewCommandsAndDiscardsPendingWork)
{
    const auto topic = std::string(PREFIX) + COMMANDS[1];
    subscriptions.at(topic)(topic, valid_payload(1).dump());
    EXPECT_EQ(component.shutdown(), 0);
    subscriptions.at(topic)(topic, valid_payload(1).dump());
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.back().second, (json {{"status", "fail"}, {"reason", "not_ready"}}));
    component.run();  // Empty-queue timeout must not send the discarded command.
    EXPECT_EQ(command_calls, 0U);
}
