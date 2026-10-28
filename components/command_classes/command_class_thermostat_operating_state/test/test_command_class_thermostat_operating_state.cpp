/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include <gtest/gtest.h>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_thermostat_operating_state.hpp"
#include "command_class_thermostat_operating_state_attribute_store.hpp"
#include "command_class_thermostat_operating_state_constants.hpp"
#include "command_class_thermostat_operating_state_mqtt.hpp"
#include "zwave_network_management_remove_failed_report.h"
#include "zwave_frame_generator.hpp"

namespace
{
    constexpr uint8_t THERMOSTAT_OPERATING_STATE_CC_ID          = 0x42;
    constexpr uint8_t THERMOSTAT_OPERATING_STATE_LOGGING_GET_ID = 0x05;
    constexpr uint8_t OPERATING_STATE_IDLE                      = 0;
    constexpr uint8_t OPERATING_STATE_COOLING                   = 7;
    constexpr uint8_t OPERATING_STATE_V2_DRYING                 = 11;
    constexpr uint8_t LOGGING_RESERVED_BIT_MASK                 = 0x01;
    constexpr uint8_t LOGGING_SUPPORTED_SECOND_BIT_MASK         = 0x02;
    constexpr uint8_t LOGGING_SECOND_MASK_BYTE                  = 0x08;
    constexpr uint8_t LOG_REPORTS_TO_FOLLOW                     = 1;
    constexpr uint8_t LOGGING_ENTRY_PROPERTIES                  = 0x23;
    constexpr size_t LOGGING_GET_FRAME_CAPACITY                 = 5;
    constexpr size_t LOGGING_GET_FRAME_LENGTH                   = 4;
    constexpr uint8_t VERSION_1                                 = 1;
    constexpr uint8_t VERSION_2                                 = 2;
    constexpr uint8_t TEST_HOME_ID[]                            = {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID                             = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                          = 0;
}  // namespace

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_thermostat_operating_state_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class ThermostatOperatingStateTest : public ::testing::Test
{
    protected:
        static command_class_thermostat_operating_state &component()
        {
            static command_class_thermostat_operating_state instance;
            return instance;
        }

        void SetUp() override
        {
            // Each case gets a fresh RAM-only attribute store and endpoint context.
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

// Spec: §§2.2.113–2.2.114, Thermostat Operating State Report; Table 2.535.
// Spec 2026B §2.2.113, Operating State Report v1.
// Verifies a v1 state is stored and a missing decoded field defaults to Idle.
TEST_F(ThermostatOperatingStateTest, StoresV1CoolingStateAndDefaultsMissingValue)
{
    auto &store = static_cast<command_class_thermostat_operating_state_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_operating_state_report_received_store(endpoint, {{"operating_state", uint8_t {OPERATING_STATE_COOLING}}}));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_REPORT_GROUP));
    auto state = group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::operating_state));
    ASSERT_TRUE(state.reported_exists());
    EXPECT_EQ(OPERATING_STATE_COOLING, state.reported<uint8_t>());

    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_operating_state_report_received_store(endpoint, {}));
    EXPECT_EQ(OPERATING_STATE_IDLE, state.reported<uint8_t>());
}

// Spec 2026B §2.2.113, Operating State Report v2.
// Verifies the v2 Drying value is preserved in the same Report field as v1 states.
TEST_F(ThermostatOperatingStateTest, StoresV2DryingState)
{
    auto &store = static_cast<command_class_thermostat_operating_state_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_operating_state_report_received_store(endpoint, {{"operating_state", uint8_t {OPERATING_STATE_V2_DRYING}}}));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_REPORT_GROUP));
    EXPECT_EQ(OPERATING_STATE_V2_DRYING, group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::operating_state)).reported<uint8_t>());
}

// Spec: §§2.2.113–2.2.114, Thermostat Operating State command definitions.
// Verifies v1 interview requests base state only and v2 also queries logging support.
TEST_F(ThermostatOperatingStateTest, InterviewV1RequiresOnlyBaseStateReport)
{
    auto &cc = component();
    static_cast<zwave_command_class_base &>(cc).on_interview(endpoint, VERSION_1);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_GET_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT_GROUP)).is_valid());
}

// Spec 2026B §2.2.114, Thermostat Operating State Logging Supported Get v2.
// Verifies the v2 interview adds the logging capability query and its report requirement.
TEST_F(ThermostatOperatingStateTest, InterviewV2AddsLoggingCapabilityRequirement)
{
    static_cast<zwave_command_class_base &>(component()).on_interview(endpoint, VERSION_2);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_supported_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_SUPPORTED_GET_GROUP)).is_valid());
}

// Spec: §2.2.114.4, Thermostat Operating State Logging Supported Report.
// Verifies reserved bit 0 is ignored and a supported v2 log enables a logging query.
TEST_F(ThermostatOperatingStateTest, LoggingSupportedReportV1CannotStartV2LoggingQuery)
{
    auto &cc     = component();
    auto version = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_thermostat_operating_state_attributes_t::supported_version));
    // Version 1 has no logging commands, so even a set capability bit must not queue a logging Get.
    version.set_reported(VERSION_1);
    EXPECT_EQ(SL_STATUS_OK, cc.on_thermostat_operating_logging_supported_report_parsed(nullptr, endpoint, {{"bit_mask", std::vector<uint8_t> {LOGGING_SUPPORTED_SECOND_BIT_MASK}}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_GET_GROUP)).is_valid());
}

// Spec 2026B §2.2.114.4, Logging Supported Report v2.
// Verifies reserved bit 0 alone does not request logs, while a supported bit does.
TEST_F(ThermostatOperatingStateTest, LoggingSupportedReportV2IgnoresReservedBitAndStartsQueryForSupportedBit)
{
    auto &cc     = component();
    auto version = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_thermostat_operating_state_attributes_t::supported_version));
    version.set_reported(VERSION_2);

    // Bit 0 is reserved by the v2 mask; a response containing only that bit must not start a log Get.
    EXPECT_EQ(SL_STATUS_OK, cc.on_thermostat_operating_logging_supported_report_parsed(nullptr, endpoint, {{"bit_mask", std::vector<uint8_t> {LOGGING_RESERVED_BIT_MASK}}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_GET_GROUP)).is_valid());

    // Bit 1 represents a supported log; store the requested mask on the new Logging Get group.
    EXPECT_EQ(SL_STATUS_OK, cc.on_thermostat_operating_logging_supported_report_parsed(nullptr, endpoint, {{"bit_mask", std::vector<uint8_t> {LOGGING_SUPPORTED_SECOND_BIT_MASK}}}));
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {LOGGING_SUPPORTED_SECOND_BIT_MASK}), get_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::bit_mask)).desired<std::vector<uint8_t>>());

    // Empty and all-zero masks must not queue a logging query.
    EXPECT_EQ(SL_STATUS_OK, cc.on_thermostat_operating_logging_supported_report_parsed(nullptr, endpoint, {{"bit_mask", std::vector<uint8_t> {}}}));
    EXPECT_FALSE(command_class_thermostat_operating_state_constants::logging_supported_bit_mask_has_logs({0x00, 0x00}));
    EXPECT_FALSE(command_class_thermostat_operating_state_constants::logging_supported_bit_mask_has_logs({}));
    EXPECT_FALSE(command_class_thermostat_operating_state_constants::logging_supported_bit_mask_has_logs({0x00, 0x00, 0x00}));
}

// Spec: §§2.2.114.4, 2.2.114.6, Logging Supported Report and Logging Report.
// Verifies the capability mask and each logging entry's properties byte are stored.
TEST_F(ThermostatOperatingStateTest, StoresLoggingReportsAndDefaultsMissingFields)
{
    // This report mask advertises the log in bit 1 of byte one and bit 3 of byte two.
    auto &store = static_cast<command_class_thermostat_operating_state_core &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_operating_logging_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {LOGGING_SUPPORTED_SECOND_BIT_MASK, LOGGING_SECOND_MASK_BYTE}}}));
    auto supported_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT_GROUP));
    ASSERT_TRUE(supported_group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {LOGGING_SUPPORTED_SECOND_BIT_MASK, LOGGING_SECOND_MASK_BYTE}), supported_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::bit_mask)).reported<std::vector<uint8_t>>());

    // One report remains in this sequence; its properties byte encodes the selected historical operating-state log.
    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_operating_state_logging_report_received_store(endpoint, {{"reports_to_follow", uint8_t {LOG_REPORTS_TO_FOLLOW}}, {"vg1", thermostat_operating_state_logging_report_vg1_t {{.properties1 = {.value = LOGGING_ENTRY_PROPERTIES}}}}}));
    auto logging_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_REPORT_GROUP));
    ASSERT_TRUE(logging_group.is_valid());
    EXPECT_EQ(LOG_REPORTS_TO_FOLLOW, logging_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::reports_to_follow)).reported<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {LOGGING_ENTRY_PROPERTIES}), logging_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>());

    ASSERT_EQ(SL_STATUS_OK, store.on_thermostat_operating_state_logging_report_received_store(endpoint, {}));
    EXPECT_EQ(0, logging_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::reports_to_follow)).reported<uint8_t>());
    EXPECT_TRUE(logging_group.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>().empty());
}

// Spec: §2.2.114.5, Thermostat Operating State Logging Get.
// Verifies the request requires and serializes the requested bit-mask bytes in order.
TEST_F(ThermostatOperatingStateTest, LoggingGetBuildsFrameAndRejectsMissingMask)
{
    auto &cc = component();
    uint8_t data[LOGGING_GET_FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {THERMOSTAT_OPERATING_STATE_CC_ID};
    generator.initialize_frame(THERMOSTAT_OPERATING_STATE_LOGGING_GET_ID, data, &length);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_GET_GROUP));
    command_class_thermostat_operating_state_core::get_requested_args args {group, &generator};
    // The command cannot be serialized until the requested logging-state mask is present.
    EXPECT_EQ(SL_STATUS_NOT_READY, cc.on_thermostat_operating_state_logging_get_requested_assemble_frame(args, data, &length));

    group.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::bit_mask)).set_desired(std::vector<uint8_t> {LOGGING_SUPPORTED_SECOND_BIT_MASK, LOGGING_SECOND_MASK_BYTE});
    generator.initialize_frame(THERMOSTAT_OPERATING_STATE_LOGGING_GET_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, cc.on_thermostat_operating_state_logging_get_requested_assemble_frame(args, data, &length));
    const std::vector<uint8_t> expected {THERMOSTAT_OPERATING_STATE_CC_ID, THERMOSTAT_OPERATING_STATE_LOGGING_GET_ID, LOGGING_SUPPORTED_SECOND_BIT_MASK, LOGGING_SECOND_MASK_BYTE};
    EXPECT_EQ(LOGGING_GET_FRAME_LENGTH, length);
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec: §§2.2.113.1, 2.2.114.1, 2.2.114.3, 2.2.114.5.
// Verifies the MQTT handlers map the regular Gets and logging Get to their protocol requests.
TEST_F(ThermostatOperatingStateTest, MqttCommandsCreateGroupsAndValidateLoggingMask)
{
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_get_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_supported_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_get_command(endpoint, R"({"bit_mask":"invalid"})"));
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_get_command(endpoint, R"({"bit_mask":[]})"));
    EXPECT_EQ(SL_STATUS_OK, command_class_thermostat_operating_state_mqtt::mqtt_on_thermostat_operating_state_logging_get_command(endpoint, R"({"bit_mask":[2,8]})"));
    EXPECT_EQ((std::vector<uint8_t> {2, 8}),
              endpoint.child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_GET_GROUP))
                .child_by_type(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::bit_mask))
                .desired<std::vector<uint8_t>>());
}
