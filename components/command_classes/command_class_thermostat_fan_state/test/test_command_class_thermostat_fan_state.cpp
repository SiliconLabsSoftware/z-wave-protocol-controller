/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include <gtest/gtest.h>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_thermostat_fan_state.hpp"
#include "command_class_thermostat_fan_state_attribute_store.hpp"
#include "command_class_thermostat_fan_state_mqtt.hpp"
#include "zwave_network_management_remove_failed_report.h"
#include "zwave_frame_generator.hpp"

namespace
{
    constexpr uint8_t THERMOSTAT_FAN_STATE_CC_ID  = 0x45;
    constexpr uint8_t THERMOSTAT_FAN_STATE_GET_ID = 0x02;
    constexpr uint8_t FAN_STATE_IDLE              = 0;
    constexpr uint8_t FAN_STATE_RUNNING_HIGH_V1   = 2;
    constexpr uint8_t FAN_STATE_CIRCULATION_V2    = 4;
    constexpr size_t GET_FRAME_LENGTH             = 2;
    constexpr uint8_t VERSION_1                   = 1;
    constexpr uint8_t VERSION_2                   = 2;
    constexpr uint8_t TEST_HOME_ID[]              = {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID               = 2;
    constexpr uint8_t TEST_ENDPOINT_ID            = 0;
}  // namespace

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_thermostat_fan_state_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class ThermostatFanStateTest : public ::testing::Test
{
    protected:
        static command_class_thermostat_fan_state &component()
        {
            static command_class_thermostat_fan_state instance;
            return instance;
        }

        void SetUp() override
        {
            // Build the minimum RAM-only node tree needed by command-class handlers.
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto home_id = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(std::begin(TEST_HOME_ID), std::end(TEST_HOME_ID)));
            auto node_id = home_id.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint     = node_id.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            (void)component();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        attribute_store::attribute endpoint;

        void set_version(uint8_t version)
        {
            // Set the peer version for the version-specific command case being exercised.
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_thermostat_fan_state_attributes_t::supported_version)).set_reported(version);
        }
};

// Spec: §2.2.110, Thermostat Fan State Report; Table 2.533.
// Verifies v1-defined Running High is stored and an absent decoded state uses Idle.
TEST_F(ThermostatFanStateTest, StoresV1RunningHighAndDefaultsMissingState)
{
    set_version(VERSION_1);
    auto &store = static_cast<command_class_thermostat_fan_state_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_fan_state_report_received_store(endpoint, {{"fan_operating_state", uint8_t {FAN_STATE_RUNNING_HIGH_V1}}}));
    auto state = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::THERMOSTAT_FAN_STATE_REPORT_GROUP)).child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::fan_operating_state));
    ASSERT_TRUE(state.reported_exists());
    EXPECT_EQ(FAN_STATE_RUNNING_HIGH_V1, state.reported<uint8_t>());

    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_fan_state_report_received_store(endpoint, {}));
    EXPECT_EQ(FAN_STATE_IDLE, state.reported<uint8_t>());
}

// Spec 2026B §2.2.110, Fan Operating State encoding v2.
// Verifies the v2 Circulation Mode state is preserved and missing state defaults to Idle.
TEST_F(ThermostatFanStateTest, StoresV2CirculationStateAndDefaultsMissingState)
{
    set_version(VERSION_2);
    auto &store = static_cast<command_class_thermostat_fan_state_core &>(component());

    // Circulation Mode is assigned value 4 beginning in version 2.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_fan_state_report_received_store(endpoint, {{"fan_operating_state", uint8_t {FAN_STATE_CIRCULATION_V2}}}));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::THERMOSTAT_FAN_STATE_REPORT_GROUP));
    auto state = group.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::fan_operating_state));
    ASSERT_TRUE(state.reported_exists());
    EXPECT_EQ(FAN_STATE_CIRCULATION_V2, state.reported<uint8_t>());

    // An empty parsed report must replace the prior state with the protocol's Idle default.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_fan_state_report_received_store(endpoint, {}));
    EXPECT_EQ(FAN_STATE_IDLE, state.reported<uint8_t>());
}

// Spec: §2.2.110, Thermostat Fan State Get.
// Verifies a Get request has the expected command bytes and no payload.
TEST_F(ThermostatFanStateTest, GetV1CreatesResolutionGroupAndBuildsFrame)
{
    set_version(VERSION_1);
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_fan_state_mqtt::mqtt_on_thermostat_fan_state_get_command(endpoint, "{}"));
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_get_group_attributes_t::THERMOSTAT_FAN_STATE_GET_GROUP)).is_valid());

    uint8_t data[GET_FRAME_LENGTH] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_STATE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_STATE_GET_ID, data, &length);
    command_class_thermostat_fan_state_core::get_requested_args args {endpoint, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_state_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_FAN_STATE_CC_ID, THERMOSTAT_FAN_STATE_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.110, Thermostat Fan State Get v2.
// Confirms the Get command remains payload-free in the version that adds new report values.
TEST_F(ThermostatFanStateTest, GetV2KeepsSameFrameLayout)
{
    set_version(VERSION_2);

    // Create the request group through the same MQTT handler used by an application client.
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_fan_state_mqtt::mqtt_on_thermostat_fan_state_get_command(endpoint, "{}"));
    uint8_t data[GET_FRAME_LENGTH] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_FAN_STATE_CC_ID};
    generator.initialize_frame(THERMOSTAT_FAN_STATE_GET_ID, data, &length);
    command_class_thermostat_fan_state_core::get_requested_args args {endpoint, &generator};

    // Version 2 changes only report-state definitions, so Get retains its two-byte frame.
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_fan_state_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_FAN_STATE_CC_ID, THERMOSTAT_FAN_STATE_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec: §2.2.110, Thermostat Fan State Get and Report.
// Verifies interview schedules a Get and requires the returned fan operating state.
TEST_F(ThermostatFanStateTest, InterviewV1CreatesReportRequirementAndGetGroup)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_1);
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::THERMOSTAT_FAN_STATE_REPORT_GROUP));
    EXPECT_TRUE(report.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::fan_operating_state)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_get_group_attributes_t::THERMOSTAT_FAN_STATE_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.110, Thermostat Fan State interview v2.
// Confirms v2 continues to request the same Report and Get after adding report-state values.
TEST_F(ThermostatFanStateTest, InterviewV2CreatesReportRequirementAndGetGroup)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_2);
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::THERMOSTAT_FAN_STATE_REPORT_GROUP));
    EXPECT_TRUE(report.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_report_group_attributes_t::fan_operating_state)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_fan_state_get_group_attributes_t::THERMOSTAT_FAN_STATE_GET_GROUP)).is_valid());
}
