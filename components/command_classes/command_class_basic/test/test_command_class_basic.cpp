/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and governed by the
 * sections of the MSLA applicable to Source Code.
 ******************************************************************************/
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_basic.hpp"
#include "command_class_basic_core.hpp"
#include "command_class_basic_events.hpp"
#include "command_class_basic_mqtt.hpp"
#include "command_class_basic_types.hpp"
#include "component_connector.hpp"
#include "zwave_command_class_base.h"
#include "zwave_frame_parser.hpp"

using namespace zwave_command_class;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}
using namespace zwave_command_class::command_class_basic_types;

namespace
{
    constexpr uint8_t BASIC_CC_ID             = 0x20;  // COMMAND_CLASS_BASIC
    constexpr uint8_t BASIC_REPORT_COMMAND    = 0x03;  // BASIC_REPORT
    constexpr uint8_t BASIC_SET_COMMAND       = 0x01;  // BASIC_SET
    constexpr uint8_t BASIC_VERSION_1         = 1;
    constexpr uint8_t BASIC_VERSION_2         = 2;
    constexpr uint8_t UNSUPPORTED_VERSION     = 0;     // Version Report when Basic was not advertised
    constexpr uint8_t ABSENT_V2_FIELD         = 0;     // CC:0000.00.00.11.003 initializes a field the shorter frame omits
    constexpr uint8_t TEST_CURRENT_VALUE      = 0x32;  // 50, inside the v1 Value range 0x01..0x63
    constexpr uint8_t TEST_TARGET_VALUE       = 0x63;  // 99, the value table's 100% / On level
    constexpr uint8_t TEST_TRANSITION_SECONDS = 0x05;  // 5 seconds, v2 Duration in the seconds range
    constexpr int OUT_OF_RANGE_SET_VALUE      = 300;   // Does not fit the 8-bit Basic Set Value field
    constexpr int CONNECTOR_DRAIN_ATTEMPTS    = 8;     // run() pops one queued event; bound the drain
    constexpr uint16_t TEST_NODE_ID           = 2;     // Fixture node, not a protocol constant
    constexpr uint8_t TEST_ENDPOINT_ID        = 0;     // Root endpoint
    constexpr uint8_t TEST_SIBLING_ENDPOINT   = 1;
    constexpr size_t FRAME_BUFFER_SIZE        = 8;
    // Fixture home ID. The four bytes only identify the RAM tree, not a Z-Wave field under test.
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {0x12, 0x34, 0x56, 0x78};

    std::string basic_set_json(int value)
    {
        return std::string(R"({"value":)") + std::to_string(value) + "}";
    }

    const auto REPORT_GROUP = static_cast<attribute_store_type_t>(basic_report_group_attributes_t::BASIC_REPORT_GROUP);
    const auto SET_GROUP    = static_cast<attribute_store_type_t>(basic_set_group_attributes_t::BASIC_SET_GROUP);
    const auto GET_GROUP    = static_cast<attribute_store_type_t>(basic_get_group_attributes_t::BASIC_GET_GROUP);
    const auto VALUE        = static_cast<attribute_store_type_t>(basic_set_group_attributes_t::value);
    const auto CURRENT      = static_cast<attribute_store_type_t>(basic_report_group_attributes_t::current_value);
    const auto TARGET       = static_cast<attribute_store_type_t>(basic_report_group_attributes_t::target_value);
    const auto DURATION     = static_cast<attribute_store_type_t>(basic_report_group_attributes_t::duration);
}  // namespace

// Exposes the protected MQTT dispatch map, as the other command-class tests do.
class TestableBasicMqtt : public command_class_basic_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class BasicCommandClassTest : public ::testing::Test
{
    protected:
        static TestableBasicMqtt &mqtt()
        {
            static TestableBasicMqtt instance;
            return instance;
        }

        static command_class_basic &component()
        {
            static command_class_basic instance;
            return instance;
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            // Home / node / root endpoint. Basic attributes hang off the endpoint.
            auto home = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(TEST_HOME_ID.begin(), TEST_HOME_ID.end()));
            auto node = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint  = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            // Construction registers the resolver give-up listener once.
            // Attribute-store teardown clears the version callback, so register it every test.
            (void)component();
            (void)mqtt();
            command_class_basic::register_version_listener();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        attribute_store::attribute endpoint;
};

// The resolver calls this listener with the Basic Get group after retries are exhausted.
// The listener publishes that group's parent endpoint.
TEST_F(BasicCommandClassTest, ResolverGiveUpPublishesEndpoint)
{
    component_connector connector;
    std::optional<attribute_store::attribute> received_endpoint;
    ASSERT_EQ(SL_STATUS_OK, (connector.connect_typed<command_class_basic_events_t, basic_get_resolution_give_up_payload_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET_RESOLUTION_GIVE_UP, [&received_endpoint](const basic_get_resolution_give_up_payload_t &payload) {
                  received_endpoint = payload.device_endpoint_node;
                  return SL_STATUS_OK;
              })));

    // The group the resolver gives up on. Its parent is the interviewed endpoint.
    const auto basic_get_group = endpoint.emplace_node(GET_GROUP);
    command_class_basic::on_basic_get_resolution_give_up(static_cast<attribute_store_node_t>(basic_get_group));
    connector.run();

    ASSERT_TRUE(received_endpoint.has_value());
    EXPECT_EQ(endpoint, *received_endpoint);
}

// A give-up for a node that is not in the tree has no endpoint to publish.
TEST_F(BasicCommandClassTest, ResolverGiveUpIgnoresOrphanGroup)
{
    component_connector connector;
    bool event_received = false;
    ASSERT_EQ(SL_STATUS_OK, (connector.connect_typed<command_class_basic_events_t, basic_get_resolution_give_up_payload_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET_RESOLUTION_GIVE_UP, [&event_received](const basic_get_resolution_give_up_payload_t &) {
                  event_received = true;
                  return SL_STATUS_OK;
              })));

    command_class_basic::on_basic_get_resolution_give_up(ATTRIBUTE_STORE_INVALID_NODE);
    connector.run();

    EXPECT_FALSE(event_received);
}

// Spec 2026B Basic CC v1, Basic Report Value (CC:0020.01.03.11.002).
// A v1 Report carries only Value. CC:0000.00.00.11.003 leaves the v2 Target Value and Duration initialized.
TEST_F(BasicCommandClassTest, ReportV1StoresCurrentValueOnly)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC)).set_reported(BASIC_VERSION_1);
    // A v1 Report has only the current value after the two-byte command header.
    const std::array<uint8_t, 3> report {BASIC_CC_ID, BASIC_REPORT_COMMAND, TEST_CURRENT_VALUE};
    zwave_frame_parser parser {report.data(), static_cast<uint16_t>(report.size())};
    command_class_basic_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_basic_report_received(args));

    const auto group = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_CURRENT_VALUE, group.child_by_type(CURRENT).reported<uint8_t>());
    // v1 has no Target Value or Duration. Those v2 fields stay at their initialized value.
    EXPECT_EQ(ABSENT_V2_FIELD, group.child_by_type(TARGET).reported<uint8_t>());
    EXPECT_EQ(ABSENT_V2_FIELD, group.child_by_type(DURATION).reported<uint8_t>());
}

// Spec 2026B Basic CC v2, Basic Report (CC:0020.02.03.11.001, CC:0020.02.03.11.003, CC:0020.02.03.12.002).
// The v2 frame keeps the v1 Current Value and appends Target Value and Duration.
TEST_F(BasicCommandClassTest, ReportV2StoresCurrentTargetAndDuration)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC)).set_reported(BASIC_VERSION_2);
    // The v2 Report appends target value and duration after current value.
    const std::array<uint8_t, 5> report {BASIC_CC_ID, BASIC_REPORT_COMMAND, TEST_CURRENT_VALUE, TEST_TARGET_VALUE, TEST_TRANSITION_SECONDS};
    zwave_frame_parser parser {report.data(), static_cast<uint16_t>(report.size())};
    command_class_basic_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_basic_report_received(args));

    const auto group = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_CURRENT_VALUE, group.child_by_type(CURRENT).reported<uint8_t>());
    EXPECT_EQ(TEST_TARGET_VALUE, group.child_by_type(TARGET).reported<uint8_t>());
    EXPECT_EQ(TEST_TRANSITION_SECONDS, group.child_by_type(DURATION).reported<uint8_t>());
}

// Spec 2026B Basic CC v1, Basic Set (CC:0000.00.00.11.002).
// A v1 peer still receives Command Class, BASIC_SET, and the one-byte Value. The command is not shortened.
TEST_F(BasicCommandClassTest, SetV1PeerIncludesValue)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC)).set_reported(BASIC_VERSION_1);
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(VALUE).set_desired(TEST_TARGET_VALUE);
    uint8_t data[FRAME_BUFFER_SIZE] {};
    uint16_t length = 0;
    zwave_frame_generator generator {BASIC_CC_ID};
    generator.initialize_frame(BASIC_SET_COMMAND, data, &length);
    command_class_basic_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_basic_set_requested(args, data, &length));

    const std::vector<uint8_t> expected {BASIC_CC_ID, BASIC_SET_COMMAND, TEST_TARGET_VALUE};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B Basic CC v2, Basic Set (CC:0020.02.00.21.001, CC:0000.00.00.11.002).
// Version 2 does not change Basic Set, so a v2 peer receives the same Value byte as a v1 peer.
TEST_F(BasicCommandClassTest, SetV2PeerKeepsValuePrefix)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC)).set_reported(BASIC_VERSION_2);
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(VALUE).set_desired(TEST_CURRENT_VALUE);
    uint8_t data[FRAME_BUFFER_SIZE] {};
    uint16_t length = 0;
    zwave_frame_generator generator {BASIC_CC_ID};
    generator.initialize_frame(BASIC_SET_COMMAND, data, &length);
    command_class_basic_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_basic_set_requested(args, data, &length));

    const std::vector<uint8_t> expected {BASIC_CC_ID, BASIC_SET_COMMAND, TEST_CURRENT_VALUE};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// A Basic Set frame is not emitted until the Value byte is known. The spec defines no status for a missing local value.
TEST_F(BasicCommandClassTest, SetWithoutDesiredValueReturnsNotReady)
{
    auto group = endpoint.emplace_node(SET_GROUP);
    uint8_t data[FRAME_BUFFER_SIZE] {};
    uint16_t length = 0;
    zwave_frame_generator generator {BASIC_CC_ID};
    generator.initialize_frame(BASIC_SET_COMMAND, data, &length);
    command_class_basic_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_basic_set_requested(args, data, &length));
    EXPECT_EQ(0, length);  // No Value byte was appended.
}

// Spec 2026B Basic CC control, support follows a Basic Report (CL:0020.01.21.02.2).
// A Basic Report on a sibling endpoint proves support when the root Version Report says 0.
TEST_F(BasicCommandClassTest, VersionZeroPromotesWhenSiblingEndpointReported)
{
    auto node             = endpoint.parent();
    auto sibling_endpoint = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_SIBLING_ENDPOINT);
    auto report_group     = sibling_endpoint.emplace_node(REPORT_GROUP);
    report_group.emplace_node(CURRENT).set_reported(TEST_CURRENT_VALUE);
    auto version = endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
    // The resolver creates the version attribute before a report updates it.
    version.set_desired(UNSUPPORTED_VERSION);

    version.set_reported(UNSUPPORTED_VERSION);

    EXPECT_EQ(BASIC_VERSION_1, version.reported<uint8_t>());
}

// Spec 2026B Basic CC control, support follows a Basic Report (CL:0020.01.21.02.2).
// A zero Version Report remains unsupported when no endpoint has answered Basic Get.
TEST_F(BasicCommandClassTest, VersionZeroStaysUnsupportedWithoutReport)
{
    auto version = endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));

    version.set_reported(UNSUPPORTED_VERSION);

    EXPECT_EQ(UNSUPPORTED_VERSION, version.reported<uint8_t>());
}

// Spec 2026B Basic CC control, support follows a Basic Report (CL:0020.01.21.02.2).
// A report group without a reported current value does not prove Basic support.
TEST_F(BasicCommandClassTest, VersionZeroStaysUnsupportedWhenCurrentValueIsNotReported)
{
    auto report_group = endpoint.emplace_node(REPORT_GROUP);
    report_group.emplace_node(CURRENT).set_desired(TEST_CURRENT_VALUE);
    auto version = endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));

    version.set_reported(UNSUPPORTED_VERSION);

    EXPECT_EQ(UNSUPPORTED_VERSION, version.reported<uint8_t>());
}

// Spec 2026B Basic CC control, support follows a Basic Report (CL:0020.01.21.02.2).
// An incomplete report group without a current-value node does not prove Basic support.
TEST_F(BasicCommandClassTest, VersionZeroStaysUnsupportedWhenReportHasNoCurrentValueNode)
{
    endpoint.emplace_node(REPORT_GROUP);
    auto version = endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));

    version.set_reported(UNSUPPORTED_VERSION);

    EXPECT_EQ(UNSUPPORTED_VERSION, version.reported<uint8_t>());
}

// Spec 2026B Basic CC control, support follows a Basic Report (CL:0020.01.21.02.2).
// A Version value without an endpoint ancestor cannot be promoted from endpoint reports.
TEST_F(BasicCommandClassTest, VersionZeroWithoutEndpointAncestorStaysUnsupported)
{
    auto version = attribute_store::attribute::root().emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));

    version.set_reported(UNSUPPORTED_VERSION);

    EXPECT_EQ(UNSUPPORTED_VERSION, version.reported<uint8_t>());
}

// Spec 2026B Basic CC control interview (CL:0020.01.21.01.1).
// The interview issues the Basic Get probe by creating its resolution group.
TEST_F(BasicCommandClassTest, GetInterviewEventCreatesResolutionGroup)
{
    basic_get_interview_payload_t payload {endpoint};
    component_connector connector;

    auto status = connector.fire_event_async(static_cast<uint32_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_GET_INTERVIEW), payload);
    // Earlier reports leave events on the shared connector queue. Drain until this one completes.
    for (int attempt = 0; attempt < CONNECTOR_DRAIN_ATTEMPTS && status.wait_for(std::chrono::seconds(0)) != std::future_status::ready; ++attempt) {
        connector.run();
    }
    ASSERT_EQ(std::future_status::ready, status.wait_for(std::chrono::seconds(0)));
    ASSERT_EQ(SL_STATUS_OK, status.get());

    constexpr auto BASIC_GET_GROUP = static_cast<attribute_store_type_t>(basic_get_group_attributes_t::BASIC_GET_GROUP);
    EXPECT_TRUE(endpoint.child_by_type(BASIC_GET_GROUP).is_valid());
}

// Spec 2026B Basic CC v1, Basic Get behavior.
// A direct Get request clears the previous marker and recreates the resolution group.
TEST_F(BasicCommandClassTest, MqttGetCreatesResolutionGroup)
{
    constexpr auto BASIC_GET_GROUP        = static_cast<attribute_store_type_t>(basic_get_group_attributes_t::BASIC_GET_GROUP);
    auto group                            = endpoint.emplace_node(BASIC_GET_GROUP);
    constexpr uint8_t PREVIOUS_GET_MARKER = 1;
    group.set_reported(PREVIOUS_GET_MARKER);

    ASSERT_EQ(SL_STATUS_OK, command_class_basic_mqtt::mqtt_on_basic_get_command(endpoint, "{}"));

    EXPECT_FALSE(group.reported_exists());
    EXPECT_TRUE(endpoint.child_by_type(BASIC_GET_GROUP).is_valid());
}

// Spec 2026B Basic CC control, support follows a Basic Report (CL:0020.01.21.02.2).
// A nonzero Version Report is preserved even when a Basic Report is already present.
TEST_F(BasicCommandClassTest, NonzeroVersionIsNotPromoted)
{
    auto report_group = endpoint.emplace_node(REPORT_GROUP);
    report_group.emplace_node(CURRENT).set_reported(TEST_CURRENT_VALUE);
    auto version = endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));

    // A reported version other than 0 already records support, so the report must not rewrite it.
    version.set_reported(BASIC_VERSION_2);

    EXPECT_EQ(BASIC_VERSION_2, version.reported<uint8_t>());
}

// A non-JSON MQTT body is rejected without creating a Basic Set group.
TEST_F(BasicCommandClassTest, MqttSetRejectsMalformedPayload)
{
    EXPECT_NE(SL_STATUS_OK, command_class_basic_mqtt::mqtt_on_basic_set_command(endpoint, "not-json"));
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// A value outside Basic Set's one-byte field is rejected before a request is created.
TEST_F(BasicCommandClassTest, MqttSetRejectsValueOutsideWireRange)
{
    // 300 cannot be stored in the 8-bit Value field (CC:0020.01.01.12.001).
    EXPECT_NE(SL_STATUS_OK, command_class_basic_mqtt::mqtt_on_basic_set_command(endpoint, basic_set_json(OUT_OF_RANGE_SET_VALUE)));
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// Spec 2026B Basic CC v1, Basic Set Value (CC:0020.01.01.12.001).
// A value inside 0x00..0x63 is stored as the desired Set Value.
TEST_F(BasicCommandClassTest, MqttSetStoresValidDesiredValue)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_basic_mqtt::mqtt_on_basic_set_command(endpoint, basic_set_json(TEST_CURRENT_VALUE)));

    const auto group = endpoint.child_by_type(SET_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_CURRENT_VALUE, group.child_by_type(VALUE).desired<uint8_t>());
}

// Spec 2026B Basic CC control, support follows a Basic Report (CL:0020.01.21.02.2).
// A Basic Report on the same endpoint promotes Version Report 0 to version 1.
TEST_F(BasicCommandClassTest, VersionZeroPromotesWhenSameEndpointHasReport)
{
    // The report is stored on the endpoint that owns the Version attribute.
    auto report_group = endpoint.emplace_node(REPORT_GROUP);
    report_group.emplace_node(CURRENT).set_reported(TEST_CURRENT_VALUE);
    auto version = endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
    version.set_desired(UNSUPPORTED_VERSION);

    // Version Get returned 0 because Basic must not be advertised.
    version.set_reported(UNSUPPORTED_VERSION);

    EXPECT_EQ(BASIC_VERSION_1, version.reported<uint8_t>());
}

// A reported-value callback with no stored version does not invent Basic support.
TEST_F(BasicCommandClassTest, ClearedVersionReportStaysUndefined)
{
    auto version = endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC));
    version.set_reported(UNSUPPORTED_VERSION);

    // Clearing the reported version delivers an update with no value to read.
    version.clear_reported();

    EXPECT_FALSE(version.reported_exists());
}

// A parsed v1 Basic Report notifies listeners with the endpoint that received it.
// The specification defines the Report fields, not this local notification.
TEST_F(BasicCommandClassTest, ReportV1PublishesReceivedEvent)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BASIC)).set_reported(BASIC_VERSION_1);
    component_connector connector;
    std::optional<attribute_store::attribute> received_endpoint;
    ASSERT_EQ(SL_STATUS_OK, (connector.connect_typed<command_class_basic_events_t, basic_report_received_payload_t>(command_class_basic_events_t::COMMAND_CLASS_BASIC_REPORT_RECEIVED, [&received_endpoint](const basic_report_received_payload_t &payload) {
                  received_endpoint = payload.device_endpoint_node;
                  return SL_STATUS_OK;
              })));

    // v1 Report: command class, command, then the one-byte current value.
    const std::array<uint8_t, 3> report {BASIC_CC_ID, BASIC_REPORT_COMMAND, TEST_CURRENT_VALUE};
    zwave_frame_parser parser {report.data(), static_cast<uint16_t>(report.size())};
    command_class_basic_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_basic_report_received(args));
    for (int attempt = 0; attempt < CONNECTOR_DRAIN_ATTEMPTS && !received_endpoint.has_value(); ++attempt) {
        connector.run();
    }

    ASSERT_TRUE(received_endpoint.has_value());
    EXPECT_EQ(endpoint, *received_endpoint);
}

// The registered BasicGet name creates the Basic Get resolution group.
TEST_F(BasicCommandClassTest, RegisteredBasicGetDispatches)
{
    mqtt().mqtt_command_handler(endpoint, "BasicGet", "{}");

    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// Spec 2026B Basic CC v1, Basic Set Value (CC:0020.01.01.12.001).
// The registered BasicSet name stores the requested Value.
TEST_F(BasicCommandClassTest, RegisteredBasicSetDispatches)
{
    mqtt().mqtt_command_handler(endpoint, "BasicSet", basic_set_json(TEST_CURRENT_VALUE));

    const auto group = endpoint.child_by_type(SET_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_CURRENT_VALUE, group.child_by_type(VALUE).desired<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(GET_GROUP).is_valid());
}
