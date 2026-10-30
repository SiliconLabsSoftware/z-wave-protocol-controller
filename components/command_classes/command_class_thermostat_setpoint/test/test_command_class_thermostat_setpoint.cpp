/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include <gtest/gtest.h>

#include <array>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_thermostat_setpoint.hpp"
#include "command_class_thermostat_setpoint_attribute_store.hpp"
#include "command_class_thermostat_setpoint_constants.hpp"
#include "command_class_thermostat_setpoint_mqtt.hpp"
#include "command_class_thermostat_mode_events.hpp"
#include "command_class_thermostat_mode_types.hpp"
#include "component_connector.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_network_management_remove_failed_report.h"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_thermostat_setpoint_types;

namespace
{
    constexpr uint8_t THERMOSTAT_SETPOINT_CC_ID                        = 0x43;
    constexpr uint8_t THERMOSTAT_SETPOINT_SET_ID                       = 0x01;
    constexpr uint8_t THERMOSTAT_SETPOINT_GET_ID                       = 0x02;
    constexpr uint8_t THERMOSTAT_SETPOINT_SUPPORTED_GET_ID             = 0x04;
    constexpr uint8_t THERMOSTAT_SETPOINT_CAPABILITIES_GET_ID          = 0x09;
    constexpr uint8_t SETPOINT_TYPE_HEATING                            = 0x01;
    constexpr uint8_t SETPOINT_TYPE_COOLING                            = 0x02;
    constexpr uint8_t SUPPORTED_HEATING_AND_COOLING_MASK               = 0x06;
    constexpr uint8_t SUPPORTED_HEATING_ONLY_MASK                      = 0x02;
    constexpr uint8_t THERMOSTAT_MODE_HEAT_VALUE                       = 0x01;
    constexpr uint8_t SETPOINT_SIZE_TWO_BYTES                          = 2;
    constexpr uint8_t SETPOINT_SCALE_CELSIUS                           = 0;
    constexpr uint8_t SETPOINT_SCALE_FAHRENHEIT                        = 1;
    constexpr uint8_t SETPOINT_PRECISION_ONE_DECIMAL                   = 1;
    constexpr uint8_t SETPOINT_SIZE_INVALID                            = 3;
    constexpr uint8_t SETPOINT_SCALE_INVALID                           = 2;
    constexpr uint8_t SETPOINT_LEVEL_HEATING                           = SETPOINT_TYPE_HEATING;
    constexpr uint8_t SETPOINT_LEVEL2_TWO_BYTES_ONE_DECIMAL            = 0x22;
    constexpr uint8_t SETPOINT_LEVEL2_TWO_BYTES_ONE_DECIMAL_FAHRENHEIT = 0x2A;
    constexpr uint8_t VERSION_1                                        = 1;
    constexpr uint8_t VERSION_2                                        = 2;
    constexpr uint8_t VERSION_3                                        = 3;
    constexpr size_t FRAME_CAPACITY                                    = 12;
    constexpr uint16_t ONE_BYTE_SET_FRAME_LENGTH                       = 5;
    constexpr uint16_t FOUR_BYTE_SET_FRAME_LENGTH                      = 8;
    constexpr size_t FIRST_SETPOINT_VALUE_OFFSET                       = 4;
    constexpr uint8_t TEST_HOME_ID[]                                   = {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID                                    = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                                 = 0;
    constexpr std::array<uint8_t, 2> HEATING_SETPOINT_VALUE_25_C       = {0x00, 0xFA};
}  // namespace

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class TestableThermostatSetpointMqtt : public command_class_thermostat_setpoint_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class ThermostatSetpointTest : public ::testing::Test
{
    protected:
        static command_class_thermostat_setpoint &component()
        {
            static command_class_thermostat_setpoint instance;
            return instance;
        }

        static TestableThermostatSetpointMqtt &mqtt()
        {
            static TestableThermostatSetpointMqtt instance;
            return instance;
        }

        void SetUp() override
        {
            // Create a fresh RAM-only endpoint so report groups and requests cannot leak between tests.
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

        void set_version(uint8_t version)
        {
            // Parsing and Set range checks use the peer's negotiated command-class version.
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_thermostat_setpoint_attributes_t::supported_version)).set_reported(version);
        }

        attribute_store::attribute endpoint;
};

// Spec 2026B §2.2.112, Thermostat Setpoint interview v1.
// Verifies v1 starts its supported-type query and requires the returned bit mask.
TEST_F(ThermostatSetpointTest, InterviewV1CreatesSupportedReportRequirement)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_1);
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_report_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_REPORT_GROUP));
    ASSERT_TRUE(report.is_valid());
    EXPECT_TRUE(report.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_report_group_attributes_t::bit_mask)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_get_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, Thermostat Setpoint interview v2.
// Confirms v2 uses the same Supported Get before requesting per-type values.
TEST_F(ThermostatSetpointTest, InterviewV2CreatesSupportedReportRequirement)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_2);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_report_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_REPORT_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_get_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, Thermostat Setpoint interview v3.
// Confirms the v3 interview also starts at the supported-type mask before capabilities discovery.
TEST_F(ThermostatSetpointTest, InterviewV3CreatesSupportedReportRequirement)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_3);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_report_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_REPORT_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_get_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Get v1.
// Verifies the requested Heating type is encoded in the Setpoint Get properties byte.
TEST_F(ThermostatSetpointTest, GetV1RequiresAndEncodesHeatingType)
{
    // Create a Get group without a type first to confirm the assembler waits for the request value.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_GET_ID, data, &length);
    command_class_thermostat_setpoint_core::get_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_thermostat_setpoint_get_requested(args, data, &length));

    // Add Heating, which is setpoint type 1 in the v1 type encoding, and assemble the request.
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    generator.initialize_frame(THERMOSTAT_SETPOINT_GET_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_SETPOINT_CC_ID, THERMOSTAT_SETPOINT_GET_ID, SETPOINT_LEVEL_HEATING};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.112, Thermostat Setpoint Supported Get.
// Confirms the supported-type query is sent without a payload.
TEST_F(ThermostatSetpointTest, SupportedGetBuildsEmptyPayloadFrame)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_supported_get_command(endpoint, "{}"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_get_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_GET_GROUP));
    ASSERT_TRUE(group.is_valid());

    // Supported Get consists of the CC and command identifiers only.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SUPPORTED_GET_ID, data, &length);
    command_class_thermostat_setpoint_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_supported_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_SETPOINT_CC_ID, THERMOSTAT_SETPOINT_SUPPORTED_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.112, Thermostat Setpoint Capabilities Get v3.
// Verifies the requested setpoint type is encoded after the Capabilities Get command identifier.
TEST_F(ThermostatSetpointTest, CapabilitiesGetV3RequiresAndEncodesHeatingType)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_CAPABILITIES_GET_ID, data, &length);
    command_class_thermostat_setpoint_core::get_requested_args args {group, &generator};

    // Capabilities Get needs a setpoint type before it can form Properties1.
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_thermostat_setpoint_capabilities_get_requested(args, data, &length));
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);

    // Heating uses type value 1 in the low bits of Properties1.
    generator.initialize_frame(THERMOSTAT_SETPOINT_CAPABILITIES_GET_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_capabilities_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_SETPOINT_CC_ID, THERMOSTAT_SETPOINT_CAPABILITIES_GET_ID, SETPOINT_TYPE_HEATING};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.112, Thermostat Setpoint Set v1–2.
// Checks a signed big-endian 25.0°C Heating value and confirms Set starts a follow-up Get.
TEST_F(ThermostatSetpointTest, SetV2EncodesHeatingTemperatureAndRequestsReport)
{
    set_version(VERSION_2);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));

    // Set the five required request fields: Heating type, two value bytes, Celsius, one decimal, and 25.0°C raw value 250.
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(SETPOINT_SIZE_TWO_BYTES);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(SETPOINT_PRECISION_ONE_DECIMAL);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t>(HEATING_SETPOINT_VALUE_25_C.begin(), HEATING_SETPOINT_VALUE_25_C.end()));

    // Assemble CC ID, Set ID, Heating type, size/scale/precision, then the big-endian 16-bit Value.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_set_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_SETPOINT_CC_ID, THERMOSTAT_SETPOINT_SET_ID, SETPOINT_LEVEL_HEATING, SETPOINT_LEVEL2_TWO_BYTES_ONE_DECIMAL, HEATING_SETPOINT_VALUE_25_C[0], HEATING_SETPOINT_VALUE_25_C[1]};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));

    // Set requests are followed by a Get for the same setpoint type so the reported value can be refreshed.
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Set v3.
// Rejects a value above the advertised maximum after scaling both values to the same precision.
TEST_F(ThermostatSetpointTest, SetV3RejectsValueOutsideReportedCapabilities)
{
    set_version(VERSION_3);

    // Report Heating capability range 20.0–30.0°C; each signed two-byte limit has one decimal place.
    auto &store = static_cast<command_class_thermostat_setpoint_core &>(component());
    ASSERT_EQ(SL_STATUS_OK,
              store.on_thermostat_setpoint_capabilities_report_received_store(endpoint,
                                                                              {
                                                                                {"setpoint_type", uint8_t {SETPOINT_TYPE_HEATING}},
                                                                                {"min_value", std::vector<uint8_t> {0x00, 0xC8}},
                                                                                {"maxvalue", std::vector<uint8_t> {0x01, 0x2C}},
                                                                                {"precision1", uint8_t {0x20}},
                                                                                {"precision2", uint8_t {0x20}},
                                                                              }));
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));

    // 31.0°C uses raw value 310, one degree above the advertised upper bound of 30.0°C.
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(SETPOINT_SIZE_TWO_BYTES);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(SETPOINT_PRECISION_ONE_DECIMAL);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {0x01, 0x36});

    // Out-of-range values must fail before the Set frame can be transmitted.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_FAIL, component().on_thermostat_setpoint_set_requested(args, data, &length));
}

// Spec 2026B §2.2.112, Thermostat Setpoint Report v1–3.
// Verifies the report's type, size, scale, precision, and value bytes are grouped by setpoint type.
TEST_F(ThermostatSetpointTest, ReportV1StoresHeatingValueFields)
{
    auto &store = static_cast<command_class_thermostat_setpoint_core &>(component());
    ASSERT_EQ(SL_STATUS_OK,
              store.on_thermostat_setpoint_report_received_store(endpoint,
                                                                 {
                                                                   {"setpoint_type", uint8_t {SETPOINT_TYPE_HEATING}},
                                                                   {"size", uint8_t {SETPOINT_SIZE_TWO_BYTES}},
                                                                   {"scale", uint8_t {SETPOINT_SCALE_CELSIUS}},
                                                                   {"precision", uint8_t {SETPOINT_PRECISION_ONE_DECIMAL}},
                                                                   {"value", std::vector<uint8_t> {HEATING_SETPOINT_VALUE_25_C[0], HEATING_SETPOINT_VALUE_25_C[1]}},
                                                                 }));

    // The report group is keyed by Heating and keeps the raw value bytes big-endian for later interpretation.
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::THERMOSTAT_SETPOINT_REPORT_GROUP));
    ASSERT_TRUE(report.is_valid());
    EXPECT_EQ(SETPOINT_SIZE_TWO_BYTES, report.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::size)).reported<uint8_t>());
    EXPECT_EQ(SETPOINT_SCALE_CELSIUS, report.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::scale)).reported<uint8_t>());
    EXPECT_EQ(SETPOINT_PRECISION_ONE_DECIMAL, report.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::precision)).reported<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {HEATING_SETPOINT_VALUE_25_C[0], HEATING_SETPOINT_VALUE_25_C[1]}), report.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::value)).reported<std::vector<uint8_t>>());
}

// CC:0043.03.05.11.003 — Spec 2026B §2.2.112, Supported Report v1–3.
// Checks v1 starts with a Setpoint Get while v3 starts with a Capabilities Get for the first supported type.
TEST_F(ThermostatSetpointTest, SupportedReportV1StartsSetpointGetForFirstType)
{
    set_version(VERSION_1);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // In the supported mask, bit 1 identifies Heating and bit 2 identifies Cooling; Heating is discovered first.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {0x06}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_parsed(nullptr, endpoint, {}));
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Supported Report v3.
// Confirms v3 requests capabilities for the first supported type before reading its value.
TEST_F(ThermostatSetpointTest, SupportedReportV3StartsCapabilitiesGetForFirstType)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // A bit mask with Heating and Cooling supported establishes the ordered type discovery list.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {0x06}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_parsed(nullptr, endpoint, {}));
    auto capabilities = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    ASSERT_TRUE(capabilities.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, capabilities.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint v3 discovery.
// Confirms a Heating report advances to the next supported type and requests Cooling capabilities when absent.
TEST_F(ThermostatSetpointTest, ReportV3RequestsNextTypeCapabilities)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Advertise Heating and Cooling and mark Heating as the outstanding value request.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {0x06}}}));
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);

    // The matching Heating report should advance the discovery sequence to Cooling.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", uint8_t {SETPOINT_TYPE_HEATING}}}));
    auto capabilities = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    ASSERT_TRUE(capabilities.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_COOLING, capabilities.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Set MQTT payload.
// Verifies MQTT stores valid Size/Scale/value fields and rejects unsupported Size and Scale values.
TEST_F(ThermostatSetpointTest, MqttSetValidatesSizeAndScale)
{
    // Missing required level, Properties2, or Value fields are rejected before a Set group is created.
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_set_command(endpoint, "{}"));

    // The parser rejects Size values outside the protocol's 1, 2, or 4-byte choices.
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_set_command(endpoint, R"({"level":{"setpoint_type":1},"level2":{"size":3,"scale":0,"precision":1},"value":[0,250]})"));

    // The parser also rejects scales other than Celsius and Fahrenheit.
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_set_command(endpoint, R"({"level":{"setpoint_type":1},"level2":{"size":2,"scale":2,"precision":1},"value":[0,250]})"));

    // A valid Celsius Heating setpoint stores its bit fields and raw big-endian value bytes as desired data.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_set_command(endpoint, R"({"level":{"setpoint_type":1},"level2":{"size":2,"scale":0,"precision":1},"value":[0,250]})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).desired<uint8_t>());
    EXPECT_EQ(SETPOINT_SIZE_TWO_BYTES, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).desired<uint8_t>());
    EXPECT_EQ(SETPOINT_SCALE_CELSIUS, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {HEATING_SETPOINT_VALUE_25_C[0], HEATING_SETPOINT_VALUE_25_C[1]}), group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).desired<std::vector<uint8_t>>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Get MQTT properties.
// Checks the required setpoint type is validated before a request group is created.
TEST_F(ThermostatSetpointTest, MqttGetRequiresSetpointType)
{
    // The Get command needs its Setpoint Type in Properties1 to know which temperature to query.
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_get_command(endpoint, "{}"));

    // Type 1 is Heating, which is part of the original Setpoint type list.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_get_command(endpoint, R"({"level":{"setpoint_type":1}})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Capabilities Get v3 MQTT properties.
// Checks the required type is validated and a valid request stores the requested type.
TEST_F(ThermostatSetpointTest, MqttCapabilitiesGetV3RequiresSetpointType)
{
    // A Capabilities Get without Properties1.setpoint_type cannot identify the requested limits.
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_capabilities_get_command(endpoint, "{}"));

    // Type 1 selects Heating; this command is introduced in version 3.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_capabilities_get_command(endpoint, R"({"properties1":{"setpoint_type":1}})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint report MQTT endpoints.
// Confirms unsupported report publishing endpoints return success without changing stored state.
TEST_F(ThermostatSetpointTest, MqttReportEndpointsAcceptPayloads)
{
    // These payloads are intentionally ignored by the current MQTT report handlers.
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_report_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_supported_report_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_setpoint_mqtt::mqtt_on_thermostat_setpoint_capabilities_report_command(endpoint, "{}"));
}

// Spec 2026B §2.2.112, Thermostat Setpoint MQTT command routes.
// Verifies every registered Setpoint MQTT route reaches its handler and accepts a valid payload.
TEST_F(ThermostatSetpointTest, RegisteredMqttRoutesDispatch)
{
    // Dispatch a Heating Get and its supported-type queries using command names published by this CC.
    mqtt().mqtt_command_handler(endpoint, "ThermostatSetpointGet", R"({"level":{"setpoint_type":1}})");
    mqtt().mqtt_command_handler(endpoint, "ThermostatSetpointSupportedGet", "{}");
    mqtt().mqtt_command_handler(endpoint, "ThermostatSetpointCapabilitiesGet", R"({"properties1":{"setpoint_type":1}})");

    // Dispatch Set with a valid two-byte, Celsius, one-decimal Heating value of 25.0 degrees.
    mqtt().mqtt_command_handler(endpoint, "ThermostatSetpointSet", R"({"level":{"setpoint_type":1},"level2":{"size":2,"scale":0,"precision":1},"value":[0,250]})");

    // The three report routes are accepted by the current handlers even though publishing is not implemented.
    mqtt().mqtt_command_handler(endpoint, "ThermostatSetpointReport", "{}");
    mqtt().mqtt_command_handler(endpoint, "ThermostatSetpointSupportedReport", "{}");
    mqtt().mqtt_command_handler(endpoint, "ThermostatSetpointCapabilitiesReport", "{}");

    // Confirm dispatch created request groups for Get, Supported Get, Capabilities Get, and Set.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_get_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Supported Report v2.
// Checks that an empty supported-type mask produces no follow-up Get for a v2 peer.
TEST_F(ThermostatSetpointTest, SupportedReportV2WithNoTypesStartsNoSetpointQuery)
{
    // Version 2 uses the same supported-type mask as v1 to choose its next Get request.
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());
    set_version(VERSION_2);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, Supported Report v1–3.
// Confirms an absent mask and versions that cannot select a discovery flow do not start a type query.
TEST_F(ThermostatSetpointTest, SupportedReportWithoutUsableMaskStartsNoQuery)
{
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // With no Supported Report stored, the handler has no type bits to inspect for a v1 peer.
    set_version(VERSION_1);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());

    // A group without a Bit Mask child is also unusable; a mode change must not start a Get from it.
    endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_report_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_REPORT_GROUP));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());

    // A v3 peer with no advertised types also has nothing to query for capabilities.
    set_version(VERSION_3);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());

    // Version zero is not a supported peer version, so no v1–2 or v3 discovery command is selected.
    set_version(0);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_AND_COOLING_MASK}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Capabilities Report v3.
// Exercises discovery when a report is out of sequence, when the scale is missing, and when the next type needs querying.
TEST_F(ThermostatSetpointTest, CapabilitiesReportV3ContinuesDiscoveryInOrder)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // An unrelated report is ignored while the outstanding Capabilities Get asks for Heating.
    auto capabilities_get = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    capabilities_get.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_COOLING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());

    // A matching report without a known scale starts the Get needed to learn that type's scale.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());

    // Record Heating's Report scale; discovery then moves to Cooling's missing capabilities.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_received_store(endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}, {"scale", SETPOINT_SCALE_CELSIUS}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_AND_COOLING_MASK}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_received_store(endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}, {"min_value", std::vector<uint8_t> {0x00}}, {"maxvalue", std::vector<uint8_t> {0x64}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    auto next_capabilities_get = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    ASSERT_TRUE(next_capabilities_get.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_COOLING, next_capabilities_get.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// CC:0043.03.0A.11.001 — Spec 2026B §2.2.112, Capabilities Report v3.
// Confirms only a matching v3 response advances discovery when the requested type field is valid.
TEST_F(ThermostatSetpointTest, CapabilitiesReportRequiresMatchingV3Request)
{
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // A v2 peer does not use the v3 Capabilities Report discovery callback.
    set_version(VERSION_2);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());

    set_version(VERSION_3);
    // Without an outstanding Capabilities Get, an unsolicited report must not start another request.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());

    // A request group without its desired type is incomplete and cannot match the response.
    endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());

    // A desired Heating type rejects a Cooling response, then accepts the matching Heating response.
    auto capabilities_get = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    capabilities_get.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_COOLING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// CC:0043.03.03.11.001 — Spec 2026B §2.2.112, Setpoint Report v1–3.
// Confirms v3 ignores unsolicited reports and only matching requests advance discovery.
TEST_F(ThermostatSetpointTest, ReportV3RequiresMatchingGetRequest)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // No outstanding Get means a valid-looking report must not create a Capabilities Get.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());

    // A Get group lacking its desired type is not a matching request.
    endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());

    // A pending Heating Get ignores Cooling, then a matching Heating report completes when there are no later types.
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_ONLY_MASK}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_COOLING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP)).is_valid());
}

// CC:0043.03.03.11.001 — Spec 2026B §2.2.112, Thermostat Setpoint Report v1–2 discovery.
// Confirms legacy discovery ignores reports without a pending type and stops after the final supported type.
TEST_F(ThermostatSetpointTest, ReportV1RequiresPendingTypeAndStopsAfterLastType)
{
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Version zero is outside the supported v1–2 discovery flow, even if a report names Heating.
    set_version(0);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());

    set_version(VERSION_1);
    // A report with no pending Get cannot establish which type should be queried next.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_FALSE(get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired_exists());

    // Cooling is supported and already has a report scale, so it does not need another legacy Get.
    get_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_AND_COOLING_MASK}}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_received_store(endpoint, {{"setpoint_type", SETPOINT_TYPE_COOLING}, {"scale", SETPOINT_SCALE_CELSIUS}}));
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_EQ(SETPOINT_TYPE_HEATING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// CC:0043.03.0A.11.001 — Spec 2026B §2.2.112, Thermostat Setpoint Capabilities Report v3.
// Confirms discovery finishes when every later supported type already has its capabilities and scale stored.
TEST_F(ThermostatSetpointTest, CapabilitiesReportV3CompletesWhenAllTypesAreKnown)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Advertise Heating and Cooling, then store complete limits and Report scales for both supported types.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_AND_COOLING_MASK}}}));
    for (const uint8_t type: {SETPOINT_TYPE_HEATING, SETPOINT_TYPE_COOLING}) {
        ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_received_store(endpoint, {{"setpoint_type", type}, {"scale", SETPOINT_SCALE_CELSIUS}}));
        ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_received_store(endpoint, {{"setpoint_type", type}, {"min_value", std::vector<uint8_t> {0x00}}, {"maxvalue", std::vector<uint8_t> {0x64}}}));
    }

    // The outstanding Heating response leaves no unqueried supported types, so no new Get is needed.
    auto capabilities_get = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
    capabilities_get.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_EQ(SETPOINT_TYPE_HEATING, capabilities_get.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, stored Setpoint Report and Capabilities Report fields.
// Checks lookup helpers distinguish missing groups, incomplete fields, empty values, and complete reports.
TEST_F(ThermostatSetpointTest, ReportLookupRequiresCompleteStoredFields)
{
    uint8_t scale = SETPOINT_SCALE_CELSIUS;
    std::vector<uint8_t> minimum;
    std::vector<uint8_t> maximum;
    uint8_t minimum_precision = 0;
    uint8_t maximum_precision = 0;

    // With no matching report groups, none of the lookup helpers can return a stored value.
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_scale_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, scale));
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum, maximum));
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_precisions_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum_precision, maximum_precision));

    // A matching Report without Scale remains incomplete; storing Scale makes that lookup succeed.
    // Distinct report groups let lookup skip a group with no type, an unreported type, and a different reported type.
    auto no_type_report = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::THERMOSTAT_SETPOINT_REPORT_GROUP));
    no_type_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::scale)).set_reported(SETPOINT_SCALE_CELSIUS);
    auto desired_type_report = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::THERMOSTAT_SETPOINT_REPORT_GROUP));
    desired_type_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    auto different_type_report = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::THERMOSTAT_SETPOINT_REPORT_GROUP));
    different_type_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::setpoint_type)).set_reported(SETPOINT_TYPE_COOLING);
    auto incomplete_report = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::THERMOSTAT_SETPOINT_REPORT_GROUP));
    incomplete_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::setpoint_type)).set_reported(SETPOINT_TYPE_HEATING);
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_scale_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, scale));
    incomplete_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_report_group_attributes_t::scale)).set_reported(SETPOINT_SCALE_CELSIUS);
    EXPECT_TRUE(command_class_thermostat_setpoint_attribute_store::get_reported_scale_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, scale));

    // Capabilities are incomplete until both limits are stored; empty values are rejected as unusable bounds.
    // Skip separate groups with no type, an unreported type, and a different reported type.
    auto no_type_capabilities = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_REPORT_GROUP));
    no_type_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::min_value)).set_reported(std::vector<uint8_t> {0x00});
    auto desired_type_capabilities = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_REPORT_GROUP));
    desired_type_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    auto different_type_capabilities = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_REPORT_GROUP));
    different_type_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::setpoint_type)).set_reported(SETPOINT_TYPE_COOLING);
    auto incomplete_capabilities = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_REPORT_GROUP));
    incomplete_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::setpoint_type)).set_reported(SETPOINT_TYPE_HEATING);
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum, maximum));
    incomplete_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::min_value)).set_reported(std::vector<uint8_t> {});
    incomplete_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::maxvalue)).set_desired(std::vector<uint8_t> {});
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum, maximum));
    incomplete_capabilities.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::maxvalue)).set_reported(std::vector<uint8_t> {});
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum, maximum));

    // One-byte limits and both precision fields form a complete v3 Capabilities Report.
    incomplete_capabilities.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::min_value)).set_reported(std::vector<uint8_t> {0x00});
    incomplete_capabilities.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::maxvalue)).set_reported(std::vector<uint8_t> {0x64});
    incomplete_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::precision1)).set_reported(SETPOINT_PRECISION_ONE_DECIMAL);
    incomplete_capabilities.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::precision2)).set_desired(SETPOINT_PRECISION_ONE_DECIMAL);
    EXPECT_FALSE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_precisions_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum_precision, maximum_precision));
    incomplete_capabilities.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::precision2)).set_reported(SETPOINT_PRECISION_ONE_DECIMAL);
    EXPECT_TRUE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum, maximum));
    EXPECT_TRUE(command_class_thermostat_setpoint_attribute_store::get_reported_capabilities_precisions_for_setpoint_type(endpoint, SETPOINT_TYPE_HEATING, minimum_precision, maximum_precision));
    EXPECT_EQ(SETPOINT_PRECISION_ONE_DECIMAL, minimum_precision);
    EXPECT_EQ(SETPOINT_PRECISION_ONE_DECIMAL, maximum_precision);
}

// Spec 2026B §2.2.112, Thermostat Setpoint Set v2.
// Verifies the assembler waits for required fields and rejects invalid Size or Scale encodings.
TEST_F(ThermostatSetpointTest, SetV2RequiresAllFieldsAndValidEncoding)
{
    set_version(VERSION_2);
    auto make_group = [&]() {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(SETPOINT_SIZE_TWO_BYTES);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(SETPOINT_PRECISION_ONE_DECIMAL);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {HEATING_SETPOINT_VALUE_25_C[0], HEATING_SETPOINT_VALUE_25_C[1]});
        return group;
    };

    // A Set with only its Setpoint Type is incomplete and must wait for the remaining Properties2 and Value fields.
    auto incomplete_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
    incomplete_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args incomplete_args {incomplete_group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_thermostat_setpoint_set_requested(incomplete_args, data, &length));

    // Size 3 is reserved; Setpoint Set only permits one, two, or four value bytes.
    auto invalid_size_group = make_group();
    invalid_size_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(SETPOINT_SIZE_INVALID);
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args invalid_size_args {invalid_size_group, &generator};
    EXPECT_EQ(SL_STATUS_FAIL, component().on_thermostat_setpoint_set_requested(invalid_size_args, data, &length));

    // Scale 2 is reserved; only Celsius and Fahrenheit are valid Setpoint scales.
    auto invalid_scale_group = make_group();
    invalid_scale_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_INVALID);
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args invalid_scale_args {invalid_scale_group, &generator};
    EXPECT_EQ(SL_STATUS_FAIL, component().on_thermostat_setpoint_set_requested(invalid_scale_args, data, &length));
}

// CC:0043.03.01.11.001 — Spec 2026B §2.2.112, Thermostat Setpoint Set v2.
// Confirms Set waits until every required field is available, including each individual field.
TEST_F(ThermostatSetpointTest, SetV2WaitsForEachRequiredField)
{
    set_version(VERSION_2);

    // Build a complete request, then remove one desired field at a time to isolate each readiness guard.
    const std::array<thermostat_setpoint_set_group_attributes_t, 5> required_fields {
      thermostat_setpoint_set_group_attributes_t::setpoint_type,
      thermostat_setpoint_set_group_attributes_t::size,
      thermostat_setpoint_set_group_attributes_t::scale,
      thermostat_setpoint_set_group_attributes_t::precision,
      thermostat_setpoint_set_group_attributes_t::value,
    };

    // A request missing any one of Setpoint Type, Size, Scale, Precision, or Value must remain unresolved.
    for (const auto missing_field: required_fields) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
        if (missing_field != thermostat_setpoint_set_group_attributes_t::setpoint_type) {
            group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
        }
        if (missing_field != thermostat_setpoint_set_group_attributes_t::size) {
            group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(SETPOINT_SIZE_TWO_BYTES);
        }
        if (missing_field != thermostat_setpoint_set_group_attributes_t::scale) {
            group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
        }
        if (missing_field != thermostat_setpoint_set_group_attributes_t::precision) {
            group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(SETPOINT_PRECISION_ONE_DECIMAL);
        }
        if (missing_field != thermostat_setpoint_set_group_attributes_t::value) {
            group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {HEATING_SETPOINT_VALUE_25_C[0], HEATING_SETPOINT_VALUE_25_C[1]});
        }

        // Each incomplete group returns NOT_READY before producing an outbound Set frame.
        uint8_t data[FRAME_CAPACITY] {};
        uint16_t length = 0;
        zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
        generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
        command_class_thermostat_setpoint_core::set_requested_args args {group, &generator};
        EXPECT_EQ(SL_STATUS_NOT_READY, component().on_thermostat_setpoint_set_requested(args, data, &length));
    }
}

// Spec 2026B §2.2.112, Thermostat Setpoint Report v1.
// Confirms a report for the requested type schedules a Get for the next supported type without a known scale.
TEST_F(ThermostatSetpointTest, ReportV1ContinuesToNextSupportedType)
{
    set_version(VERSION_1);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Heating is the outstanding request; the mask also advertises Cooling as the next type to discover.
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_AND_COOLING_MASK}}}));

    // The next type has no Report scale yet, so the callback advances the Get request to Cooling.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_EQ(SETPOINT_TYPE_COOLING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Report v2.
// Confirms v2 retains the v1 type-discovery sequence for an existing Report command.
TEST_F(ThermostatSetpointTest, ReportV2ContinuesToNextSupportedType)
{
    set_version(VERSION_2);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Heating is the outstanding request; the mask also advertises Cooling as the next type to discover.
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_AND_COOLING_MASK}}}));

    // The next type has no Report scale yet, so v2 also advances the Get request to Cooling.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_parsed(nullptr, endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}}));
    EXPECT_EQ(SETPOINT_TYPE_COOLING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Set v3 signed one-byte value.
// Checks a one-byte value within the reported range is accepted using the signed wire representation.
TEST_F(ThermostatSetpointTest, SetV3AcceptsOneByteValueWithinCapabilities)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Report Heating limits from 0 through 100 with precision 0, so raw Value 50 is valid.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_capabilities_report_received_store(endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}, {"min_value", std::vector<uint8_t> {0x00}}, {"maxvalue", std::vector<uint8_t> {0x64}}, {"precision1", uint8_t {0}}, {"precision2", uint8_t {0}}}));
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(uint8_t {1});
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(uint8_t {0});
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {0x32});

    // Assemble and verify that the valid one-byte Heating value reaches the wire.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_set_requested(args, data, &length));
    EXPECT_EQ(ONE_BYTE_SET_FRAME_LENGTH, length);
    EXPECT_EQ(0x32, data[FIRST_SETPOINT_VALUE_OFFSET]);
}

// Spec 2026B §2.2.112, Thermostat Setpoint Set v3 signed four-byte value.
// Checks a four-byte big-endian value within the reported range is accepted.
TEST_F(ThermostatSetpointTest, SetV3AcceptsFourByteValueWithinCapabilities)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Report Heating limits from 0 through 1000 with precision 0 for a four-byte raw value of 500.
    ASSERT_EQ(
      SL_STATUS_OK,
      core.on_thermostat_setpoint_capabilities_report_received_store(endpoint,
                                                                     {{"setpoint_type", SETPOINT_TYPE_HEATING}, {"min_value", std::vector<uint8_t> {0x00, 0x00, 0x00, 0x00}}, {"maxvalue", std::vector<uint8_t> {0x00, 0x00, 0x03, 0xE8}}, {"precision1", uint8_t {0}}, {"precision2", uint8_t {0}}}));
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(uint8_t {4});
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(uint8_t {0});
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {0x00, 0x00, 0x01, 0xF4});

    // Assemble to confirm the size field and all four value bytes are preserved in network order.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_set_requested(args, data, &length));
    EXPECT_EQ(FOUR_BYTE_SET_FRAME_LENGTH, length);
    EXPECT_EQ((std::vector<uint8_t> {0x00, 0x00, 0x01, 0xF4}), std::vector<uint8_t>(data + FIRST_SETPOINT_VALUE_OFFSET, data + length));
}

// Spec 2026B §2.2.112, setpoint refresh after Thermostat Mode changes.
// Verifies the first supported type is queried synchronously when the Mode Changed event arrives.
TEST_F(ThermostatSetpointTest, ModeChangeRequestsFirstSupportedSetpoint)
{
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_HEATING_AND_COOLING_MASK}}}));

    // Publish the event through the component connector queue and process one event without timing delays.
    component_connector connector;
    command_class_thermostat_mode_types::thermostat_mode_changed_payload_t payload {endpoint, THERMOSTAT_MODE_HEAT_VALUE};
    auto result = connector.fire_event_async(static_cast<uint32_t>(command_class_thermostat_mode_events_t::THERMOSTAT_MODE_CHANGED), payload);
    connector.run();
    ASSERT_EQ(SL_STATUS_OK, result.get());

    // Heating is the first advertised type, so Mode Changed starts its Setpoint Get.
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SETPOINT_TYPE_HEATING, get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type)).desired<uint8_t>());
}

// Spec 2026B §2.2.112, setpoint refresh after Thermostat Mode changes.
// Confirms a mode change does not create a Get when the Supported Report advertises no types.
TEST_F(ThermostatSetpointTest, ModeChangeWithNoSupportedTypesStartsNoGet)
{
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {}}}));

    // The event still succeeds when the endpoint has no supported setpoint types to refresh.
    component_connector connector;
    command_class_thermostat_mode_types::thermostat_mode_changed_payload_t payload {endpoint, THERMOSTAT_MODE_HEAT_VALUE};
    auto result = connector.fire_event_async(static_cast<uint32_t>(command_class_thermostat_mode_events_t::THERMOSTAT_MODE_CHANGED), payload);
    connector.run();
    EXPECT_EQ(SL_STATUS_OK, result.get());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.112, Thermostat Setpoint Set v3 capabilities range.
// Confirms a value below the reported minimum is rejected using signed decimal precision.
TEST_F(ThermostatSetpointTest, SetV3RejectsValueBelowReportedMinimum)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Heating limits are 20.0 through 30.0: raw 200 and 300 with one decimal place.
    ASSERT_EQ(SL_STATUS_OK,
              core.on_thermostat_setpoint_capabilities_report_received_store(endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}, {"min_value", std::vector<uint8_t> {0x00, 0xC8}}, {"maxvalue", std::vector<uint8_t> {0x01, 0x2C}}, {"precision1", uint8_t {0x20}}, {"precision2", uint8_t {0x20}}}));
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(SETPOINT_SIZE_TWO_BYTES);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(SETPOINT_PRECISION_ONE_DECIMAL);
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {0x00, 0x96});  // Raw 150 means 15.0, below the reported minimum 20.0.

    // A below-minimum Set is invalid even though its type, scale, precision, and byte count are valid.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_FAIL, component().on_thermostat_setpoint_set_requested(args, data, &length));
}

// CC:0043.03.03.11.001 — Spec 2026B §2.2.112, Setpoint Set v3 range and scale handling.
// Confirms incomplete capability data skips local range validation, and a known report scale is used for encoding.
TEST_F(ThermostatSetpointTest, SetV3HandlesIncompleteCapabilitiesAndReportedScale)
{
    set_version(VERSION_3);
    auto &core = static_cast<command_class_thermostat_setpoint_core &>(component());

    // Store Heating's reported Fahrenheit scale; Set must use this known scale when assembling its Properties2 byte.
    ASSERT_EQ(SL_STATUS_OK, core.on_thermostat_setpoint_report_received_store(endpoint, {{"setpoint_type", SETPOINT_TYPE_HEATING}, {"scale", SETPOINT_SCALE_FAHRENHEIT}}));

    auto make_group = [&]() {
        auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::THERMOSTAT_SETPOINT_SET_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type)).set_desired(SETPOINT_TYPE_HEATING);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size)).set_desired(SETPOINT_SIZE_TWO_BYTES);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale)).set_desired(SETPOINT_SCALE_CELSIUS);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision)).set_desired(SETPOINT_PRECISION_ONE_DECIMAL);
        group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {HEATING_SETPOINT_VALUE_25_C[0], HEATING_SETPOINT_VALUE_25_C[1]});
        return group;
    };

    // No Capabilities Report means the controller has no advertised range to check, so it can send the requested value.
    auto without_capabilities = make_group();
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETPOINT_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args without_capabilities_args {without_capabilities, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_set_requested(without_capabilities_args, data, &length));
    EXPECT_EQ(SETPOINT_LEVEL2_TWO_BYTES_ONE_DECIMAL_FAHRENHEIT, data[3]);  // Properties2 encodes two-byte Size, Fahrenheit Scale, and one decimal place.

    // Capabilities without precision data are incomplete for decimal comparison; preserve device-side range validation.
    auto capabilities_report = endpoint.add_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_REPORT_GROUP));
    capabilities_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::setpoint_type)).set_reported(SETPOINT_TYPE_HEATING);
    capabilities_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::min_value)).set_reported(std::vector<uint8_t> {0x00, 0x00});
    capabilities_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_report_group_attributes_t::maxvalue)).set_reported(std::vector<uint8_t> {0x03, 0xE8});
    auto without_precisions = make_group();
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args without_precisions_args {without_precisions, &generator};
    EXPECT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_set_requested(without_precisions_args, data, &length));

    // If the supplied Value byte count disagrees with Size, skip comparing it to the reported bounds.
    auto mismatched_value_size = make_group();
    mismatched_value_size.child_by_type(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value)).set_desired(std::vector<uint8_t> {0xFA});
    generator.initialize_frame(THERMOSTAT_SETPOINT_SET_ID, data, &length);
    command_class_thermostat_setpoint_core::set_requested_args mismatched_value_size_args {mismatched_value_size, &generator};
    EXPECT_EQ(SL_STATUS_OK, component().on_thermostat_setpoint_set_requested(mismatched_value_size_args, data, &length));
}
