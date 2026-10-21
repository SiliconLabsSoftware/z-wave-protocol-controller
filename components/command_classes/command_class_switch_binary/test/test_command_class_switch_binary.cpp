/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <array>
#include <gtest/gtest.h>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
#include "command_class_switch_binary.hpp"
#include "command_class_switch_binary_attribute_store.hpp"
#include "command_class_switch_binary_mqtt.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_switch_binary_types;

namespace
{
    // Switch Binary protocol values used by the cases below.
    constexpr uint8_t SWITCH_BINARY_COMMAND_CLASS_ID = 0x25;
    constexpr uint8_t SWITCH_BINARY_SET_COMMAND_ID   = 0x01;
    constexpr uint8_t SWITCH_BINARY_VERSION_1        = 1;
    constexpr uint8_t SWITCH_BINARY_VERSION_2        = 2;
    constexpr uint8_t SWITCH_BINARY_OFF              = 0;
    constexpr uint8_t SWITCH_BINARY_ON               = 255;
    constexpr uint8_t SWITCH_BINARY_UNKNOWN          = 254;
    constexpr uint8_t SWITCH_BINARY_SAMPLE_DURATION  = 17;
    constexpr uint8_t SWITCH_BINARY_SET_FRAME_LENGTH = 4;
    constexpr uint8_t SWITCH_BINARY_DEFAULT_VALUE    = 0;
    constexpr uint16_t WRONG_TYPE_CURRENT_VALUE      = 7;
    constexpr uint16_t WRONG_TYPE_TARGET_VALUE       = 8;
    constexpr uint16_t WRONG_TYPE_DURATION           = 9;
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID      = 2;
    constexpr uint8_t TEST_ENDPOINT_ID   = 0;
    constexpr uint32_t FIRST_CHILD_INDEX = 0;
}  // namespace

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class TestableSwitchBinaryMqtt : public command_class_switch_binary_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class SwitchBinaryCommandClassTest : public ::testing::Test
{
    protected:
        static TestableSwitchBinaryMqtt &mqtt()
        {
            static TestableSwitchBinaryMqtt instance;
            return instance;
        }

        static command_class_switch_binary &component()
        {
            static command_class_switch_binary instance;
            return instance;
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto root        = attribute_store::attribute::root();
            auto home_id     = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(TEST_HOME_ID.begin(), TEST_HOME_ID.end()));
            auto node_id     = home_id.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint         = node_id.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            endpoint_node_id = attribute_store_get_node_child_by_type(attribute_store_get_node_child_by_type(attribute_store_get_node_child_by_type(attribute_store_get_root(), ATTRIBUTE_HOME_ID, FIRST_CHILD_INDEX), ATTRIBUTE_NODE_ID, FIRST_CHILD_INDEX), ATTRIBUTE_ENDPOINT_ID, FIRST_CHILD_INDEX);
            (void)mqtt();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        attribute_store::attribute endpoint;
        attribute_store_node_t endpoint_node_id = ATTRIBUTE_STORE_INVALID_NODE;

        void expect_set_frame_for_peer_version(uint8_t peer_version)
        {
            // Prepare a Set group using On and the v2 duration field.
            auto set_group    = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));
            auto target_value = set_group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::target_value));
            auto duration     = set_group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::duration));
            target_value.set_desired(SWITCH_BINARY_ON);
            duration.set_desired(SWITCH_BINARY_SAMPLE_DURATION);

            // Record the destination's supported version to verify peer-version behavior.
            auto version = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_switch_binary_attributes_t::supported_version));
            version.set_reported(peer_version);

            // Assemble the current controller Set frame for this peer.
            uint8_t data[SWITCH_BINARY_SET_FRAME_LENGTH] {};
            uint16_t length = 0;
            zwave_frame_generator generator {SWITCH_BINARY_COMMAND_CLASS_ID};
            generator.initialize_frame(SWITCH_BINARY_SET_COMMAND_ID, data, &length);
            command_class_switch_binary_core::set_requested_args args {set_group, &generator};

            ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_set_requested(args, data, &length));

            // Existing Set commands retain the controller's v2 wire format, including Duration.
            EXPECT_EQ(SWITCH_BINARY_SET_FRAME_LENGTH, length);
            EXPECT_EQ(SWITCH_BINARY_COMMAND_CLASS_ID, data[0]);
            EXPECT_EQ(SWITCH_BINARY_SET_COMMAND_ID, data[1]);
            EXPECT_EQ(SWITCH_BINARY_ON, data[2]);
            EXPECT_EQ(SWITCH_BINARY_SAMPLE_DURATION, data[3]);
        }
};

// CC:0025.01.03.11.001
// Verifies a version 1 Report stores Off as the current value.
TEST_F(SwitchBinaryCommandClassTest, ReportV1StoresOffCurrentValue)
{
    // Version 1 reports contain the current value; Off is encoded as zero.
    command_class_switch_binary_attribute_map_t report {{"current_value", SWITCH_BINARY_OFF}};

    // Store the decoded report and read back the reported attribute.
    ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, report));
    const auto report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report_group.is_valid());
    const auto current_value = report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value));
    ASSERT_TRUE(current_value.reported_exists());
    EXPECT_EQ(SWITCH_BINARY_OFF, current_value.reported<switch_binary_report_current_value_t>());
}

// CC:0025.01.03.11.001, CC:0025.01.03.11.002
// Verifies version 1 Report preserves the Unknown current value.
TEST_F(SwitchBinaryCommandClassTest, ReportV1StoresUnknownCurrentValue)
{
    // Version 1 defines 254 as the Unknown current-value encoding.
    command_class_switch_binary_attribute_map_t report {{"current_value", SWITCH_BINARY_UNKNOWN}};

    // Store the decoded report and locate the current-value attribute.
    ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, report));
    const auto report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report_group.is_valid());
    const auto current_value = report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value));
    ASSERT_TRUE(current_value.reported_exists());
    EXPECT_EQ(SWITCH_BINARY_UNKNOWN, current_value.reported<switch_binary_report_current_value_t>());
}

// CC:0025.01.03.11.001
// Verifies version 1 Report preserves the On current value.
TEST_F(SwitchBinaryCommandClassTest, ReportV1StoresOnCurrentValue)
{
    // Version 1 reports encode On as 255.
    command_class_switch_binary_attribute_map_t report {{"current_value", SWITCH_BINARY_ON}};

    // Store the decoded report and read back the reported current value.
    ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, report));
    const auto report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report_group.is_valid());
    const auto current_value = report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value));
    ASSERT_TRUE(current_value.reported_exists());
    EXPECT_EQ(SWITCH_BINARY_ON, current_value.reported<switch_binary_report_current_value_t>());
}

// CC:0025.02.03.11.001, CC:0025.02.03.11.003, CC:0025.02.03.11.004, CC:0025.02.03.12.001
// Verifies version 2 reports preserve current value, target value, and duration.
TEST_F(SwitchBinaryCommandClassTest, ReportV2StoresCurrentTargetAndDuration)
{
    // A version 2 Report adds the target and duration after the current value.
    command_class_switch_binary_attribute_map_t report {
      {"current_value", SWITCH_BINARY_OFF},
      {"target_value", SWITCH_BINARY_ON},
      {"duration", SWITCH_BINARY_SAMPLE_DURATION},
    };

    // Store the report, then retrieve each of its three defined fields.
    ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, report));
    const auto report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report_group.is_valid());
    const auto current_value = report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value));
    const auto target_value  = report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::target_value));
    const auto duration      = report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::duration));

    // Confirm the current state, most recent target, and transition duration are retained.
    ASSERT_TRUE(current_value.reported_exists());
    ASSERT_TRUE(target_value.reported_exists());
    ASSERT_TRUE(duration.reported_exists());
    EXPECT_EQ(SWITCH_BINARY_OFF, current_value.reported<switch_binary_report_current_value_t>());
    EXPECT_EQ(SWITCH_BINARY_ON, target_value.reported<switch_binary_report_target_value_t>());
    EXPECT_EQ(SWITCH_BINARY_SAMPLE_DURATION, duration.reported<switch_binary_report_duration_t>());
}

// Verifies report fields with mismatched decoded types fall back to their zero defaults.
TEST_F(SwitchBinaryCommandClassTest, ReportFieldsWithWrongTypesUseDefaults)
{
    // Deliberately pass 16-bit values where the decoded report map expects 8-bit fields.
    command_class_switch_binary_attribute_map_t report {
      {"current_value", WRONG_TYPE_CURRENT_VALUE},
      {"target_value", WRONG_TYPE_TARGET_VALUE},
      {"duration", WRONG_TYPE_DURATION},
    };

    // The storage layer cannot read the mismatched types and supplies zero defaults.
    ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, report));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SWITCH_BINARY_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value)).reported<switch_binary_report_current_value_t>());
    EXPECT_EQ(SWITCH_BINARY_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::target_value)).reported<switch_binary_report_target_value_t>());
    EXPECT_EQ(SWITCH_BINARY_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::duration)).reported<switch_binary_report_duration_t>());
}

// Verifies a report with no recognized fields stores the protocol defaults for all optional values.
TEST_F(SwitchBinaryCommandClassTest, EmptyReportStoresDefaultValues)
{
    // No decoded report fields means each report attribute receives its zero default.
    ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, {}));
    const auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SWITCH_BINARY_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value)).reported<switch_binary_report_current_value_t>());
    EXPECT_EQ(SWITCH_BINARY_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::target_value)).reported<switch_binary_report_target_value_t>());
    EXPECT_EQ(SWITCH_BINARY_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::duration)).reported<switch_binary_report_duration_t>());
}

// CC:0025.01.01.11.001, CC:0025.02.01.11.001, CC:0025.02.01.12.001
// Verifies Set frame generation requires both target and duration.
TEST_F(SwitchBinaryCommandClassTest, SetV1PeerUsesControllerVersion2WireFormat)
{
    // A version 1 peer supports Set but knows only its Value prefix.
    expect_set_frame_for_peer_version(SWITCH_BINARY_VERSION_1);
}

// CC:0025.02.01.11.001, CC:0025.02.01.12.001, CC:0025.02.01.11.002
// Verifies a version 2 peer receives Target Value followed by Duration.
TEST_F(SwitchBinaryCommandClassTest, SetV2PeerReceivesTargetAndDuration)
{
    // A version 2 peer supports both the target and transition duration fields.
    expect_set_frame_for_peer_version(SWITCH_BINARY_VERSION_2);
}

// CC:0025.01.01.11.001, CC:0025.02.01.11.001, CC:0025.02.01.12.001
// Verifies Set frame generation rejects a missing Target Value.
TEST_F(SwitchBinaryCommandClassTest, SetWithoutTargetReturnsNotReady)
{
    // Create a Set request without its required Target Value.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));
    uint8_t data[SWITCH_BINARY_SET_FRAME_LENGTH] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SWITCH_BINARY_COMMAND_CLASS_ID};
    generator.initialize_frame(SWITCH_BINARY_SET_COMMAND_ID, data, &length);
    command_class_switch_binary_core::set_requested_args args {group, &generator};

    // Without Target Value, the request cannot be assembled.
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_switch_binary_set_requested(args, data, &length));
}

// CC:0025.01.01.11.001, CC:0025.02.01.11.001, CC:0025.02.01.12.001
// Verifies Set frame generation waits until Duration is available.
TEST_F(SwitchBinaryCommandClassTest, SetRequiresTargetAndDuration)
{
    // Create a Set group with a target but no duration; the incomplete request is not ready.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::target_value)).set_desired(SWITCH_BINARY_ON);
    uint8_t data[SWITCH_BINARY_SET_FRAME_LENGTH] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SWITCH_BINARY_COMMAND_CLASS_ID};
    generator.initialize_frame(SWITCH_BINARY_SET_COMMAND_ID, data, &length);
    command_class_switch_binary_core::set_requested_args args {group, &generator};

    // Missing Duration prevents assembly; supplying it makes the frame ready.
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_switch_binary_set_requested(args, data, &length));
    group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::duration)).set_desired(SWITCH_BINARY_SAMPLE_DURATION);
    generator.initialize_frame(SWITCH_BINARY_SET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_OK, component().on_switch_binary_set_requested(args, data, &length));
}

// CC:0025.02.03.11.001
// Verifies interview initializes the report requirement and starts a Get resolution for a supported endpoint.
TEST_F(SwitchBinaryCommandClassTest, InterviewCreatesReportAndGetGroups)
{
    // Mark Switch Binary v2 as supported, then deliver the normal interview-complete event.
    (void)component();
    endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t> {SWITCH_BINARY_COMMAND_CLASS_ID});
    auto version = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_switch_binary_attributes_t::supported_version));
    version.set_reported(SWITCH_BINARY_VERSION_2);

    component_connector connector;
    const component_connector_interview_done_payload_t payload {endpoint_node_id, SL_STATUS_OK};
    auto status = connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload);
    connector.run();
    ASSERT_EQ(SL_STATUS_OK, status.get());
    // Interview registers the report value as required and queues a Get resolution.
    const auto report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report_group.is_valid());
    EXPECT_TRUE(report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_get_group_attributes_t::SWITCH_BINARY_GET_GROUP)).is_valid());
}

// CC:0025.01.02.11.001, CC:0025.02.00.21.002
// Verifies MQTT Set rejects a missing duration and stores both required version 2 fields.
TEST_F(SwitchBinaryCommandClassTest, MqttSetParsesRequiredFieldsAndRejectsMissingFields)
{
    (void)component();
    // Each invalid payload below omits a required field or uses a malformed or out-of-range Target Value or Duration.
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":255})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"duration":17})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":"on","duration":17})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":255,"duration":"fast"})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, "not-json"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":null,"duration":17})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":256,"duration":17})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":-1,"duration":17})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":1.5,"duration":17})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":255,"duration":{}})"));
    EXPECT_NE(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, "null"));

    // The valid payload requests On (255) over the sample transition duration (17).
    EXPECT_EQ(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":255,"duration":17})"));
    const auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    const auto target_value = group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::target_value));
    const auto duration     = group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::duration));
    ASSERT_TRUE(target_value.desired_exists());
    ASSERT_TRUE(duration.desired_exists());
    EXPECT_EQ(SWITCH_BINARY_ON, target_value.desired<switch_binary_set_target_value_t>());
    EXPECT_EQ(SWITCH_BINARY_SAMPLE_DURATION, duration.desired<switch_binary_set_duration_t>());

    mqtt().mqtt_command_handler(endpoint, "SwitchBinarySet", R"({"target_value":255,"duration":17})");
    mqtt().mqtt_command_handler(endpoint, "SwitchBinaryGet", "{}");
    mqtt().mqtt_command_handler(endpoint, "SwitchBinarySet", R"({"target_value":255,"duration":17})");
}

// CC:0025.01.02.11.001, CC:0025.02.00.21.002
// Verifies MQTT Get creates a Switch Binary Get resolution group.
TEST_F(SwitchBinaryCommandClassTest, MqttGetCreatesAGetResolutionGroup)
{
    (void)component();
    // An empty MQTT Get command requests the current Switch Binary value.
    EXPECT_EQ(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_get_command(endpoint, "{}"));
    mqtt().mqtt_command_handler(endpoint, "SwitchBinaryGet", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_get_group_attributes_t::SWITCH_BINARY_GET_GROUP)).is_valid());
}

// Verifies the registered MQTT Get and Set command names dispatch to their handlers.
TEST_F(SwitchBinaryCommandClassTest, RegisteredMqttCommandsDispatch)
{
    mqtt().mqtt_command_handler(endpoint, "SwitchBinaryGet", "{}");
    mqtt().mqtt_command_handler(endpoint, "SwitchBinarySet", R"({"target_value":255,"duration":17})");

    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_get_group_attributes_t::SWITCH_BINARY_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP)).is_valid());
}
