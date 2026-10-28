/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include <gtest/gtest.h>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_thermostat_fan_mode.hpp"
#include "command_class_thermostat_fan_mode_attribute_store.hpp"
#include "command_class_thermostat_fan_mode_mqtt.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_network_management_remove_failed_report.h"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_thermostat_fan_mode_types;

namespace
{
    constexpr uint8_t THERMOSTAT_FAN_MODE_CC_ID            = 0x44;
    constexpr uint8_t THERMOSTAT_FAN_MODE_SET_ID           = 0x01;
    constexpr uint8_t THERMOSTAT_FAN_MODE_GET_ID           = 0x02;
    constexpr uint8_t THERMOSTAT_FAN_MODE_SUPPORTED_GET_ID = 0x04;
    constexpr uint8_t FAN_MODE_LOW                         = 0x01;
    constexpr uint8_t FAN_MODE_MEDIUM                      = 0x05;
    constexpr uint8_t FAN_MODE_CIRCULATION                 = 0x06;
    constexpr uint8_t FAN_MODE_LEFT_RIGHT                  = 0x08;
    constexpr uint8_t FAN_MODE_OFF_DISABLED                = 0;
    constexpr uint8_t FAN_MODE_OFF_ENABLED                 = 1;
    constexpr uint8_t PACKED_OFF_BIT_AND_MEDIUM_MODE       = 0x85;
    constexpr uint8_t SUPPORTED_VERSION_1                  = 1;
    constexpr uint8_t SUPPORTED_VERSION_2                  = 2;
    constexpr uint8_t SUPPORTED_VERSION_3                  = 3;
    constexpr uint8_t SUPPORTED_VERSION_4                  = 4;
    constexpr uint8_t SUPPORTED_VERSION_5                  = 5;
    constexpr uint8_t TEST_HOME_ID[]                       = {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID                        = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                     = 0;
}  // namespace

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class ThermostatFanModeTest : public ::testing::Test
{
    protected:
        static command_class_thermostat_fan_mode &component()
        {
            static command_class_thermostat_fan_mode instance;
            return instance;
        }

        void SetUp() override
        {
            // Create an isolated RAM-only node and endpoint for this case.
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto home = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(std::begin(TEST_HOME_ID), std::end(TEST_HOME_ID)));
            auto node = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint  = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            (void)component();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        template<typename AttributeEnum, typename Value> void desired(attribute_store::attribute group, AttributeEnum type, Value value)
        {
            group.emplace_node(static_cast<attribute_store_type_t>(type)).set_desired(value);
        }

        void set_peer_version(uint8_t version)
        {
            // Record which peer version this frame case represents; existing Set uses one command ID across versions.
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_thermostat_fan_mode_attributes_t::supported_version)).set_reported(version);
        }

        attribute_store::attribute endpoint;
};

// Spec 2026B §2.2.108, Thermostat Fan Mode Set v1.
// Checks the version 1 LOW mode in the legacy one-byte format; Off is clear in the v1-defined prefix.
TEST_F(ThermostatFanModeTest, SetV1PeerReceivesCurrentFormatWithLegacyLowMode)
{
    set_peer_version(SUPPORTED_VERSION_1);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
    desired(group, thermostat_fan_mode_set_group_attributes_t::fan_mode, FAN_MODE_LOW);
    desired(group, thermostat_fan_mode_set_group_attributes_t::off, FAN_MODE_OFF_DISABLED);

    // Assemble the existing Set command and check CC ID, command ID, then the unchanged LOW mode value.
    uint8_t data[3] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    command_class_thermostat_fan_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_set_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_FAN_MODE_CC_ID, THERMOSTAT_FAN_MODE_SET_ID, FAN_MODE_LOW};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Set v2.
// Checks MEDIUM mode and the Off bit introduced in v2 are packed into the current Set properties byte.
TEST_F(ThermostatFanModeTest, SetV2PeerReceivesMediumModeAndOffBit)
{
    set_peer_version(SUPPORTED_VERSION_2);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
    desired(group, thermostat_fan_mode_set_group_attributes_t::fan_mode, FAN_MODE_MEDIUM);
    desired(group, thermostat_fan_mode_set_group_attributes_t::off, FAN_MODE_OFF_ENABLED);

    // The properties byte carries Off in bit 7 and MEDIUM in its low four bits.
    uint8_t data[3] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    command_class_thermostat_fan_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_set_requested(args, data, &length));
    EXPECT_EQ(PACKED_OFF_BIT_AND_MEDIUM_MODE, data[2]);
    EXPECT_EQ(3, length);
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Set v3.
// Checks CIRCULATION, a mode introduced in version 3, is encoded in the low mode bits.
TEST_F(ThermostatFanModeTest, SetV3PeerReceivesCirculationMode)
{
    set_peer_version(SUPPORTED_VERSION_3);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
    desired(group, thermostat_fan_mode_set_group_attributes_t::fan_mode, FAN_MODE_CIRCULATION);
    desired(group, thermostat_fan_mode_set_group_attributes_t::off, FAN_MODE_OFF_DISABLED);

    uint8_t data[3] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    command_class_thermostat_fan_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_set_requested(args, data, &length));
    EXPECT_EQ(FAN_MODE_CIRCULATION, data[2]);
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Set v4.
// Checks LEFT & RIGHT mode, first defined in version 4, with Off clear.
TEST_F(ThermostatFanModeTest, SetV4PeerReceivesLeftRightMode)
{
    set_peer_version(SUPPORTED_VERSION_4);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
    desired(group, thermostat_fan_mode_set_group_attributes_t::fan_mode, FAN_MODE_LEFT_RIGHT);
    desired(group, thermostat_fan_mode_set_group_attributes_t::off, FAN_MODE_OFF_DISABLED);

    uint8_t data[3] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    command_class_thermostat_fan_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_set_requested(args, data, &length));
    EXPECT_EQ(FAN_MODE_LEFT_RIGHT, data[2]);
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Set v5.
// Confirms v5 retains the established Set byte layout and an older-defined mode remains meaningful.
TEST_F(ThermostatFanModeTest, SetV5PeerReceivesCurrentModeFormat)
{
    set_peer_version(SUPPORTED_VERSION_5);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
    desired(group, thermostat_fan_mode_set_group_attributes_t::fan_mode, FAN_MODE_LOW);
    desired(group, thermostat_fan_mode_set_group_attributes_t::off, FAN_MODE_OFF_DISABLED);

    uint8_t data[3] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    command_class_thermostat_fan_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_set_requested(args, data, &length));
    EXPECT_EQ(FAN_MODE_LOW, data[2]);
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Get.
// Verifies a Get command emits only the CC and command identifiers.
TEST_F(ThermostatFanModeTest, GetBuildsEmptyPayloadFrame)
{
    // A Get request uses an empty JSON payload and creates the resolution group before transmission.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_get_command(endpoint, "{}"));
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_get_group_attributes_t::THERMOSTAT_FAN_MODE_GET_GROUP)).is_valid());

    // Frame contains the CC ID and Get ID only; the command has no payload fields.
    uint8_t data[2] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_GET_ID, data, &length);
    command_class_thermostat_fan_mode_core::get_requested_args args {endpoint, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_FAN_MODE_CC_ID, THERMOSTAT_FAN_MODE_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Supported Get.
// Verifies the supported-mode query is a payload-free command.
TEST_F(ThermostatFanModeTest, SupportedGetBuildsEmptyPayloadFrame)
{
    // The Supported Get request creates a query group without needing MQTT properties.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_supported_get_command(endpoint, "{}"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_get_group_attributes_t::THERMOSTAT_FAN_MODE_SUPPORTED_GET_GROUP));
    ASSERT_TRUE(group.is_valid());

    // The protocol command contains the CC and Supported Get IDs with no following data bytes.
    uint8_t data[2] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SUPPORTED_GET_ID, data, &length);
    command_class_thermostat_fan_mode_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_supported_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_FAN_MODE_CC_ID, THERMOSTAT_FAN_MODE_SUPPORTED_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Report and Supported Report.
// Verifies report fields persist and missing parsed fields use their zero defaults.
TEST_F(ThermostatFanModeTest, ReportsStoreModeOffAndSupportedMask)
{
    // The Report's mode and Off fields describe the current operating choice.
    auto &store = static_cast<command_class_thermostat_fan_mode_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_fan_mode_report_received_store(endpoint, {{"fan_mode", uint8_t {FAN_MODE_MEDIUM}}, {"off", uint8_t {FAN_MODE_OFF_ENABLED}}}));
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::THERMOSTAT_FAN_MODE_REPORT_GROUP));
    EXPECT_EQ(FAN_MODE_MEDIUM, report.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::fan_mode)).reported<uint8_t>());
    EXPECT_EQ(FAN_MODE_OFF_ENABLED, report.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::off)).reported<uint8_t>());

    // An empty parsed report stores protocol default values for both fields.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_fan_mode_report_received_store(endpoint, {}));
    EXPECT_EQ(0, report.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::fan_mode)).reported<uint8_t>());
    EXPECT_EQ(0, report.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_report_group_attributes_t::off)).reported<uint8_t>());

    // Supported Report bit 1 advertises the LOW fan mode; store the mask without changing report values.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_fan_mode_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {0x02}}}));
    auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_report_group_attributes_t::THERMOSTAT_FAN_MODE_SUPPORTED_REPORT_GROUP));
    EXPECT_EQ((std::vector<uint8_t> {0x02}), supported.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_report_group_attributes_t::bit_mask)).reported<std::vector<uint8_t>>());
}

// Spec 2026B §2.2.108, Thermostat Fan Mode interview.
// Confirms the v1 interview starts the mode and supported-mode queries and records the required mask.
TEST_F(ThermostatFanModeTest, InterviewV1CreatesModeAndSupportedQueries)
{
    // Interview version 1 has the original manual-fan modes and still queries both reports.
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, SUPPORTED_VERSION_1);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_get_group_attributes_t::THERMOSTAT_FAN_MODE_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_get_group_attributes_t::THERMOSTAT_FAN_MODE_SUPPORTED_GET_GROUP)).is_valid());
    auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_report_group_attributes_t::THERMOSTAT_FAN_MODE_SUPPORTED_REPORT_GROUP));
    EXPECT_TRUE(supported.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_supported_report_group_attributes_t::bit_mask)).is_valid());
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Set MQTT properties.
// Checks that a missing required property is rejected and valid properties create a Set request.
TEST_F(ThermostatFanModeTest, MqttSetRequiresFanModeAndOffProperties)
{
    // Both fields are required by Properties1, so an empty payload must not create a usable request.
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_set_command(endpoint, "{}"));

    // LOW is a legacy mode and Off=0 means the fan mode is enabled rather than forced off.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_fan_mode_mqtt::mqtt_on_thermostat_fan_mode_set_command(endpoint, R"({"properties1":{"fan_mode":1,"off":0}})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(FAN_MODE_LOW, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::fan_mode)).desired<uint8_t>());
    EXPECT_EQ(FAN_MODE_OFF_DISABLED, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::off)).desired<uint8_t>());
}

// Spec 2026B §2.2.108, Thermostat Fan Mode Set Properties1.
// Verifies frame assembly waits until both Fan Mode and Off have been supplied.
TEST_F(ThermostatFanModeTest, SetV1WaitsForBothProperties)
{
    // The command cannot be encoded until the mandatory mode and Off flag are both present.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_fan_mode_set_group_attributes_t::THERMOSTAT_FAN_MODE_SET_GROUP));
    uint8_t data[3] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    command_class_thermostat_fan_mode_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_thermostat_fan_mode_set_requested(args, data, &length));

    // Supplying just the requested mode still leaves the Off bit undefined, so assembly must wait.
    desired(group, thermostat_fan_mode_set_group_attributes_t::fan_mode, FAN_MODE_LOW);
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_thermostat_fan_mode_set_requested(args, data, &length));

    // Supplying Off=0 completes Properties1 and lets the frame be assembled.
    desired(group, thermostat_fan_mode_set_group_attributes_t::off, FAN_MODE_OFF_DISABLED);
    generator.initialize_frame(THERMOSTAT_FAN_MODE_SET_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_mode_set_requested(args, data, &length));
    EXPECT_EQ(FAN_MODE_LOW, data[2]);
}
