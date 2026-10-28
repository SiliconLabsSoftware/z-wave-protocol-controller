/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include <gtest/gtest.h>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_thermostat_setback.hpp"
#include "command_class_thermostat_setback_attribute_store.hpp"
#include "command_class_thermostat_setback_mqtt.hpp"
#include "zwave_network_management_remove_failed_report.h"
#include "zwave_frame_generator.hpp"

namespace
{
    constexpr uint8_t THERMOSTAT_SETBACK_CC_ID        = 0x47;
    constexpr uint8_t THERMOSTAT_SETBACK_SET_ID       = 0x01;
    constexpr uint8_t THERMOSTAT_SETBACK_GET_ID       = 0x02;
    constexpr uint8_t SETBACK_TYPE_PERMANENT_OVERRIDE = 2;
    constexpr uint8_t SETBACK_STATE_ENERGY_SAVING     = 0x7A;
    constexpr uint8_t DEFAULT_SETBACK_VALUE           = 0;
    constexpr size_t SETBACK_FRAME_CAPACITY           = 4;
    constexpr size_t SETBACK_FRAME_LENGTH             = 4;
    constexpr uint8_t VERSION_1                       = 1;
    constexpr uint8_t TEST_HOME_ID[]                  = {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID                   = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                = 0;
}  // namespace

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_thermostat_setback_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class ThermostatSetbackTest : public ::testing::Test
{
    protected:
        static command_class_thermostat_setback &component()
        {
            static command_class_thermostat_setback instance;
            return instance;
        }

        void SetUp() override
        {
            // Build a minimal RAM-only node tree so tests exercise handlers without a datastore.
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
};

// Spec: §2.2.115.3, Thermostat Setback Report.
// Verifies the reported Permanent Override type and Energy Saving state are retained, including missing-field defaults.
TEST_F(ThermostatSetbackTest, StoresReportFieldsAndDefaultsMissingValues)
{
    auto &store = static_cast<command_class_thermostat_setback_core &>(component());
    // Setback Type 2 selects Permanent Override; State 0x7A is the specified Energy Saving Mode value.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_setback_report_received_store(endpoint, {{"setback_type", uint8_t {SETBACK_TYPE_PERMANENT_OVERRIDE}}, {"setback_state", uint8_t {SETBACK_STATE_ENERGY_SAVING}}}));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::THERMOSTAT_SETBACK_REPORT_GROUP));
    EXPECT_EQ(SETBACK_TYPE_PERMANENT_OVERRIDE, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_type)).reported<uint8_t>());
    EXPECT_EQ(SETBACK_STATE_ENERGY_SAVING, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_state)).reported<uint8_t>());

    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_setback_report_received_store(endpoint, {}));
    EXPECT_EQ(DEFAULT_SETBACK_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_type)).reported<uint8_t>());
    EXPECT_EQ(DEFAULT_SETBACK_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_state)).reported<uint8_t>());
}

// Spec: §2.2.115.1, Thermostat Setback Set; Tables 2.536–2.537.
// Verifies the type and setback state are encoded in the Set frame.
TEST_F(ThermostatSetbackTest, SetFrameRequiresFieldsAndEncodesOverrideType)
{
    auto &cc   = component();
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::THERMOSTAT_SETBACK_SET_GROUP));
    uint8_t data[SETBACK_FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETBACK_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETBACK_SET_ID, data, &length);
    command_class_thermostat_setback_core::set_requested_args args {group, &generator};
    // The frame needs both desired fields; supplying only the type is still incomplete.
    EXPECT_EQ(SL_STATUS_NOT_READY, cc.on_thermostat_setback_set_requested(args, data, &length));

    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_type)).set_desired(SETBACK_TYPE_PERMANENT_OVERRIDE);
    generator.initialize_frame(THERMOSTAT_SETBACK_SET_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, cc.on_thermostat_setback_set_requested(args, data, &length));
    // Energy Saving Mode is encoded as 0x7A; verify this named state reaches the wire unchanged.
    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_state)).set_desired(SETBACK_STATE_ENERGY_SAVING);
    generator.initialize_frame(THERMOSTAT_SETBACK_SET_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, cc.on_thermostat_setback_set_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_SETBACK_CC_ID, THERMOSTAT_SETBACK_SET_ID, SETBACK_TYPE_PERMANENT_OVERRIDE, SETBACK_STATE_ENERGY_SAVING};
    EXPECT_EQ(SETBACK_FRAME_LENGTH, length);
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B §2.2.115.2, Thermostat Setback Get v1.
// Verifies the Get command emits only its command-class and command identifiers.
TEST_F(ThermostatSetbackTest, GetV1BuildsEmptyPayloadFrame)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_get_command(endpoint, "{}"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_get_group_attributes_t::THERMOSTAT_SETBACK_GET_GROUP));
    ASSERT_TRUE(group.is_valid());

    // Setback Get has no fields after the two-byte command header.
    uint8_t data[SETBACK_FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_SETBACK_CC_ID};
    generator.initialize_frame(THERMOSTAT_SETBACK_GET_ID, data, &length);
    command_class_thermostat_setback_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_thermostat_setback_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_SETBACK_CC_ID, THERMOSTAT_SETBACK_GET_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec: §§2.2.115.1–2.2.115.2, Thermostat Setback Set and Get.
// Verifies Get requests a read and Set is rejected until both command fields are supplied.
TEST_F(ThermostatSetbackTest, MqttGetCreatesRequestGroup)
{
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_get_command(endpoint, "{}"));
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_get_group_attributes_t::THERMOSTAT_SETBACK_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.115.1, Thermostat Setback Set MQTT fields.
// Verifies malformed input fails and a complete payload stores both the override type and state.
TEST_F(ThermostatSetbackTest, MqttSetRejectsIncompletePayloadAndStoresValidFields)
{
    // The Set parser requires the nested type and the top-level setback state.
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_set_command(endpoint, R"({"setback_state":122})"));
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_set_command(endpoint, "not-json"));

    // A complete payload uses Properties1.setback_type and the Energy Saving Mode state code.
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_setback_mqtt::mqtt_on_thermostat_setback_set_command(endpoint, R"({"properties1":{"setback_type":2},"setback_state":122})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::THERMOSTAT_SETBACK_SET_GROUP));
    EXPECT_EQ(SETBACK_TYPE_PERMANENT_OVERRIDE, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_type)).desired<uint8_t>());
    EXPECT_EQ(SETBACK_STATE_ENERGY_SAVING, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_set_group_attributes_t::setback_state)).desired<uint8_t>());
}

// Spec: §§2.2.115.2–2.2.115.3, Thermostat Setback Get and Report.
// Verifies interview schedules a Get and requires both fields returned by the Report.
TEST_F(ThermostatSetbackTest, InterviewCreatesReportRequirementsAndGetGroup)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_1);
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::THERMOSTAT_SETBACK_REPORT_GROUP));
    EXPECT_TRUE(report.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_type)).is_valid());
    EXPECT_TRUE(report.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_report_group_attributes_t::setback_state)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_setback_get_group_attributes_t::THERMOSTAT_SETBACK_GET_GROUP)).is_valid());
}
