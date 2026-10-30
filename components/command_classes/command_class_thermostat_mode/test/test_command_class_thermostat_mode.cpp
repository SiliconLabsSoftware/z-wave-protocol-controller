/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include <gtest/gtest.h>

#include <array>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_thermostat_mode.hpp"
#include "command_class_thermostat_mode_attribute_store.hpp"
#include "command_class_thermostat_mode_mqtt.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"
#include "zwave_network_management_remove_failed_report.h"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_thermostat_mode_types;

namespace
{
    constexpr uint8_t THERMOSTAT_MODE_CC_ID               = 0x40;
    constexpr uint8_t THERMOSTAT_MODE_SET_ID              = 0x01;
    constexpr uint8_t THERMOSTAT_MODE_GET_ID              = 0x02;
    constexpr uint8_t THERMOSTAT_MODE_SUPPORTED_GET_ID    = 0x04;
    constexpr uint8_t THERMOSTAT_MODE_REPORT_ID           = 0x03;
    constexpr uint8_t MODE_HEAT                           = 0x01;
    constexpr uint8_t MODE_COOL                           = 0x02;
    constexpr uint8_t MODE_MANUFACTURER_SPECIFIC          = 0x1F;
    constexpr uint8_t MANUFACTURER_DATA_BYTE_COUNT        = 2;
    constexpr uint8_t PACKED_MANUFACTURER_MODE_AND_LENGTH = 0x5F;
    constexpr size_t FRAME_CAPACITY                       = 8;
    constexpr uint8_t VERSION_1                           = 1;
    constexpr uint8_t VERSION_2                           = 2;
    constexpr uint8_t VERSION_3                           = 3;
    constexpr uint8_t TEST_HOME_ID[]                      = {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID                       = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                    = 0;
}  // namespace

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class ThermostatModeTest : public ::testing::Test
{
    protected:
        static command_class_thermostat_mode &component()
        {
            static command_class_thermostat_mode instance;
            return instance;
        }

        void SetUp() override
        {
            // Create the isolated endpoint tree used by report storage and request handlers.
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

        void set_peer_version(uint8_t version)
        {
            // Tie each frame test to the peer version whose field definition it documents.
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_thermostat_mode_attributes_t::supported_version)).set_reported(version);
        }

        attribute_store::attribute endpoint;
};

// Spec 2026B §2.2.105, Thermostat Mode Set v1.
// Confirms an existing OFF/HEAT mode is sent using its version 1-defined properties prefix.
TEST_F(ThermostatModeTest, SetV1PeerReceivesHeatMode)
{
    set_peer_version(VERSION_1);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::THERMOSTAT_MODE_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::mode)).set_desired(MODE_HEAT);

    // Assemble the current Set format; Heat is defined in every supported version and needs no manufacturer bytes.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_MODE_SET_ID, data, &length);
    command_class_thermostat_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_set_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_MODE_CC_ID, THERMOSTAT_MODE_SET_ID, MODE_HEAT};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.105, Thermostat Mode Set v2.
// Confirms the v2 peer accepts the unchanged COOL mode field layout introduced in v1.
TEST_F(ThermostatModeTest, SetV2PeerReceivesCoolMode)
{
    set_peer_version(VERSION_2);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::THERMOSTAT_MODE_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::mode)).set_desired(MODE_COOL);

    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_MODE_SET_ID, data, &length);
    command_class_thermostat_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_set_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_MODE_CC_ID, THERMOSTAT_MODE_SET_ID, MODE_COOL};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.105, Thermostat Mode Set v3.
// Checks MANUFACTURER SPECIFIC packs its 2-byte data count above the 5-bit mode and appends both bytes.
TEST_F(ThermostatModeTest, SetV3PeerEncodesManufacturerSpecificMode)
{
    set_peer_version(VERSION_3);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::THERMOSTAT_MODE_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::mode)).set_desired(MODE_MANUFACTURER_SPECIFIC);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::no_of_manufacturer_data_fields)).set_desired(MANUFACTURER_DATA_BYTE_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::manufacturer_data)).set_desired(std::vector<uint8_t> {0xA5, 0x5A});

    // Mode occupies bits 0–4, byte count occupies bits 5–7, then the manufacturer bytes follow.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_MODE_SET_ID, data, &length);
    command_class_thermostat_mode_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_set_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_MODE_CC_ID, THERMOSTAT_MODE_SET_ID, PACKED_MANUFACTURER_MODE_AND_LENGTH, 0xA5, 0x5A};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.105, Thermostat Mode Get.
// Verifies the Get command has no payload and creates an MQTT-requestable resolution group.
TEST_F(ThermostatModeTest, GetBuildsEmptyPayloadFrame)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_mode_mqtt::mqtt_on_thermostat_mode_get_command(endpoint, "{}"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_get_group_attributes_t::THERMOSTAT_MODE_GET_GROUP));
    ASSERT_TRUE(group.is_valid());

    // A Mode Get contains only the command-class identifier and command identifier.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_MODE_GET_ID, data, &length);
    command_class_thermostat_mode_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_MODE_CC_ID, THERMOSTAT_MODE_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.105, Thermostat Mode Supported Get.
// Verifies supported-mode discovery is sent as a payload-free request.
TEST_F(ThermostatModeTest, SupportedGetBuildsEmptyPayloadFrame)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_mode_mqtt::mqtt_on_thermostat_mode_supported_get_command(endpoint, "{}"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_supported_get_group_attributes_t::THERMOSTAT_MODE_SUPPORTED_GET_GROUP));
    ASSERT_TRUE(group.is_valid());

    // Supported Get has no properties after its CC and command identifiers.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_MODE_CC_ID};
    generator.initialize_frame(THERMOSTAT_MODE_SUPPORTED_GET_ID, data, &length);
    command_class_thermostat_mode_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_supported_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_MODE_CC_ID, THERMOSTAT_MODE_SUPPORTED_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.105, Thermostat Mode interview v1.
// Verifies the base mode Get and Supported Get groups are created for the original version.
TEST_F(ThermostatModeTest, InterviewV1CreatesBaseModeQueries)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_1);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_get_group_attributes_t::THERMOSTAT_MODE_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_supported_get_group_attributes_t::THERMOSTAT_MODE_SUPPORTED_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.105, Thermostat Mode interview v2.
// Confirms v2 retains the v1 interview requests because no new interview command is introduced.
TEST_F(ThermostatModeTest, InterviewV2CreatesBaseModeQueries)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_2);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_get_group_attributes_t::THERMOSTAT_MODE_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_supported_get_group_attributes_t::THERMOSTAT_MODE_SUPPORTED_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.105, Thermostat Mode interview v3.
// Confirms v3 retains the same Get requests while adding support for manufacturer-specific Set values.
TEST_F(ThermostatModeTest, InterviewV3CreatesBaseModeQueries)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_3);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_get_group_attributes_t::THERMOSTAT_MODE_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_supported_get_group_attributes_t::THERMOSTAT_MODE_SUPPORTED_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.105, Thermostat Mode Report and Supported Report.
// Verifies stored mode values are updated and absent report fields default to OFF.
TEST_F(ThermostatModeTest, ReportsStoreModeAndDefaultMissingFields)
{
    // Store a decoded COOL report and look up its sole endpoint report group.
    auto &store = static_cast<command_class_thermostat_mode_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_mode_report_received_store(endpoint, {{"mode", uint8_t {MODE_COOL}}}));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_report_group_attributes_t::THERMOSTAT_MODE_REPORT_GROUP));
    auto mode  = group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_report_group_attributes_t::mode));
    ASSERT_TRUE(mode.reported_exists());
    EXPECT_EQ(MODE_COOL, mode.reported<uint8_t>());

    // A report with no decoded mode uses the protocol's OFF value.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_mode_report_received_store(endpoint, {}));
    EXPECT_EQ(0, mode.reported<uint8_t>());

    // Bit 1 advertises HEAT in the Supported Report's first mask byte.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_mode_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {0x02}}}));
    auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_supported_report_group_attributes_t::THERMOSTAT_MODE_SUPPORTED_REPORT_GROUP));
    EXPECT_EQ((std::vector<uint8_t> {0x02}), supported.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_supported_report_group_attributes_t::bit_mask)).reported<std::vector<uint8_t>>());
}

// Spec 2026B §2.2.105, Thermostat Mode Set MQTT fields.
// Checks required mode parsing, optional manufacturer byte count, and optional manufacturer data handling.
TEST_F(ThermostatModeTest, MqttSetParsesModeAndOptionalManufacturerData)
{
    // A missing mode is invalid because the Set request cannot be assembled without it.
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_mode_mqtt::mqtt_on_thermostat_mode_set_command(endpoint, "{}"));

    // An ordinary mode omits the optional manufacturer values and receives a zero data-byte count.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_mode_mqtt::mqtt_on_thermostat_mode_set_command(endpoint, R"({"level":{"mode":1}})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::THERMOSTAT_MODE_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(MODE_HEAT, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::mode)).desired<uint8_t>());
    EXPECT_EQ(0, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::no_of_manufacturer_data_fields)).desired<uint8_t>());

    // MANUFACTURER SPECIFIC provides an explicit byte count matching its two configuration bytes.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_mode_mqtt::mqtt_on_thermostat_mode_set_command(endpoint, R"({"level":{"mode":31,"no_of_manufacturer_data_fields":2},"manufacturer_data":[165,90]})"));
    EXPECT_EQ(MODE_MANUFACTURER_SPECIFIC, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::mode)).desired<uint8_t>());
    EXPECT_EQ(MANUFACTURER_DATA_BYTE_COUNT, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::no_of_manufacturer_data_fields)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {0xA5, 0x5A}), group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::manufacturer_data)).desired<std::vector<uint8_t>>());
}

// Spec 2026B §2.2.105, Thermostat Mode Set manufacturer data.
// Confirms an explicitly empty optional manufacturer payload is accepted without creating a data field.
TEST_F(ThermostatModeTest, MqttSetAcceptsEmptyManufacturerData)
{
    // The HEAT value is a base mode; an empty optional data array carries no manufacturer bytes.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_mode_mqtt::mqtt_on_thermostat_mode_set_command(endpoint, R"({"level":{"mode":1},"manufacturer_data":[]})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::THERMOSTAT_MODE_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(MODE_HEAT, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::mode)).desired<uint8_t>());
    EXPECT_FALSE(group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::manufacturer_data)).is_valid());
}

// Spec 2026B §2.2.105, Thermostat Mode Report v1.
// Exercises report parsing for first, unchanged, and changed reported modes.
TEST_F(ThermostatModeTest, ReportV1StoresModeAndHandlesRepeatedValue)
{
    // A Report frame contains the CC ID, Report ID, and one Properties1 byte carrying HEAT.
    const std::array<uint8_t, 3> heat_report {{THERMOSTAT_MODE_CC_ID, THERMOSTAT_MODE_REPORT_ID, MODE_HEAT}};
    zwave_frame_parser heat_parser {heat_report.data(), static_cast<uint16_t>(heat_report.size())};
    command_class_thermostat_mode_core::report_received_args heat_args {endpoint, heat_parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_report_received(heat_args));

    // Receiving the same mode again must still parse and store the report successfully.
    zwave_frame_parser repeated_heat_parser {heat_report.data(), static_cast<uint16_t>(heat_report.size())};
    command_class_thermostat_mode_core::report_received_args repeated_heat_args {endpoint, repeated_heat_parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_report_received(repeated_heat_args));

    // A COOL Report updates the same report group to the new current mode.
    const std::array<uint8_t, 3> cool_report {{THERMOSTAT_MODE_CC_ID, THERMOSTAT_MODE_REPORT_ID, MODE_COOL}};
    zwave_frame_parser cool_parser {cool_report.data(), static_cast<uint16_t>(cool_report.size())};
    command_class_thermostat_mode_core::report_received_args cool_args {endpoint, cool_parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_mode_report_received(cool_args));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_report_group_attributes_t::THERMOSTAT_MODE_REPORT_GROUP));
    EXPECT_EQ(MODE_COOL, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_mode_report_group_attributes_t::mode)).reported<uint8_t>());
}
