/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <gtest/gtest.h>

#include <array>
#include <functional>

#include <nlohmann/json.hpp>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
#include "command_class_schedule.hpp"
#include "command_class_schedule_attribute_store.hpp"
#include "command_class_schedule_mqtt.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_schedule_types;

namespace
{
    // Schedule CC command identifiers from the Z-Wave command class specification.
    constexpr uint8_t SCHEDULE_COMMAND_CLASS_ID                  = 0x53;
    constexpr uint8_t SCHEDULE_SUPPORTED_GET_COMMAND_ID          = 0x01;
    constexpr uint8_t SCHEDULE_SUPPORTED_REPORT_COMMAND_ID       = 0x02;
    constexpr uint8_t COMMAND_SCHEDULE_SET_COMMAND_ID            = 0x03;
    constexpr uint8_t COMMAND_SCHEDULE_GET_COMMAND_ID            = 0x04;
    constexpr uint8_t SCHEDULE_REMOVE_COMMAND_ID                 = 0x06;
    constexpr uint8_t SCHEDULE_STATE_SET_COMMAND_ID              = 0x07;
    constexpr uint8_t SCHEDULE_STATE_GET_COMMAND_ID              = 0x08;
    constexpr uint8_t SCHEDULE_SUPPORTED_COMMANDS_GET_COMMAND_ID = 0x0A;

    // Schedule Supported Report values explain the selected block, capabilities, and test CC entries.
    constexpr uint8_t SUPPORTED_SCHEDULE_COUNT         = 5;
    constexpr uint8_t START_TIME_SUPPORT_V2            = 0x05;  // Start now and calendar time (bits 0 and 2).
    constexpr uint8_t START_TIME_SUPPORT_V3            = 0x15;  // v2 start options plus Time from now (bit 4).
    constexpr uint8_t SUPPORTED_CC_COUNT               = 2;
    constexpr uint8_t SUPPORTED_CC_SWITCH_MULTILEVEL   = 0x20;
    constexpr uint8_t SUPPORTED_CC_SWITCH_BINARY       = 0x25;
    constexpr uint8_t SUPPORTED_CC_GET_SET_FLAGS       = 0x03;
    constexpr uint8_t SUPPORTED_CC_SET_FLAG            = 0x04;
    constexpr uint8_t SUPPORTED_OVERRIDE_TYPES         = 0x12;
    constexpr uint8_t SUPPORTED_SCHEDULE_BLOCK_COUNT   = 3;
    constexpr uint8_t SUPPORTED_COMMAND_LIST_LENGTH    = 2;
    constexpr uint8_t SUPPORTED_COMMAND_CLASS_COUNT_1  = 2;
    constexpr uint8_t SUPPORTED_COMMAND_CLASS_COUNT_2  = 1;
    constexpr uint8_t SWITCH_MULTILEVEL_SET_COMMAND    = 0x01;
    constexpr uint8_t SWITCH_MULTILEVEL_GET_COMMAND    = 0x02;
    constexpr uint8_t SWITCH_BINARY_SET_COMMAND        = 0x03;
    constexpr uint8_t SCHEDULE_ID_FOR_SET_TEST         = 3;
    constexpr uint8_t SCHEDULE_START_YEAR              = 26;
    constexpr uint8_t SCHEDULE_START_MONTH             = 4;
    constexpr uint8_t SCHEDULE_RECURRENCE_OFFSET       = 2;
    constexpr uint8_t SCHEDULE_START_DAY               = 9;
    constexpr uint8_t SCHEDULE_RECURRENCE_MODE         = 1;
    constexpr uint8_t SCHEDULE_START_WEEKDAY           = 5;
    constexpr uint8_t SCHEDULE_START_HOUR              = 10;
    constexpr uint8_t SCHEDULE_DURATION_TYPE           = 2;
    constexpr uint8_t SCHEDULE_START_MINUTE            = 12;
    constexpr uint8_t SCHEDULE_RELATIVE_FLAG           = 1;
    constexpr uint16_t SCHEDULE_DURATION               = 90;
    constexpr uint8_t SCHEDULE_REPORTS_TO_FOLLOW       = 0;
    constexpr uint8_t SCHEDULE_COMMANDS_TO_FOLLOW      = 1;
    constexpr uint8_t SCHEDULE_START_SECOND            = 0;
    constexpr uint8_t NO_RECURRENCE_OFFSET             = 0;
    constexpr uint8_t NO_RECURRENCE_MODE               = 0;
    constexpr uint8_t RELATIVE_START_DISABLED          = 0;
    constexpr uint8_t NESTED_COMMAND_LENGTH            = 2;
    constexpr uint8_t NESTED_COMMAND_CLASS             = 0x20;
    constexpr uint8_t NESTED_COMMAND_ID                = 0x02;
    constexpr uint8_t V3_PACKED_MONTH_AND_RECURRENCE   = 0x24;
    constexpr uint8_t V3_PACKED_DAY_AND_MODE           = 0x29;
    constexpr uint8_t PACKED_DURATION_TYPE_AND_HOUR    = 0x4A;
    constexpr uint8_t V3_PACKED_RELATIVE_AND_MINUTE    = 0x4C;
    constexpr uint8_t V2_PACKED_START_MINUTE           = 0x0C;
    constexpr uint8_t SUPPORTED_REPORT_PROPERTIES_V1   = START_TIME_SUPPORT_V2;
    constexpr uint8_t SUPPORTED_REPORT_CC_COUNT_NONE   = 0;
    constexpr uint8_t SUPPORTED_REPORT_OVERRIDE_FLAGS  = 0x12;
    constexpr uint8_t SCHEDULE_DEFAULT_VALUE           = 0;
    constexpr uint16_t WRONG_TYPE_SCHEDULE_ID_COUNT    = 5;
    constexpr uint16_t WRONG_TYPE_START_TIME_SUPPORT   = 1;
    constexpr uint16_t WRONG_TYPE_SUPPORTED_CC_COUNT   = 2;
    constexpr uint16_t WRONG_TYPE_BLOCK_COUNT          = 3;
    constexpr uint16_t WRONG_TYPE_COMMAND_LIST_LENGTH  = 2;
    constexpr size_t REQUIRED_SCHEDULE_SET_FIELD_COUNT = 16;
    constexpr size_t SCHEDULE_FRAME_CAPACITY           = 32;
    constexpr uint8_t NO_FOLLOWING_SCHEDULE_COMMANDS   = 0;

    // The implemented Schedule CC versions and IDs used to exercise their block support.
    constexpr uint8_t SCHEDULE_VERSION_1        = 1;
    constexpr uint8_t SCHEDULE_VERSION_2        = 2;
    constexpr uint8_t SCHEDULE_VERSION_3        = 3;
    constexpr uint8_t SCHEDULE_VERSION_4        = 4;
    constexpr uint8_t DEFAULT_SCHEDULE_ID_BLOCK = 0;
    constexpr uint8_t SCHEDULE_ID_BLOCK_1       = 1;
    constexpr uint8_t SCHEDULE_ID_BLOCK_2       = 2;
    constexpr uint8_t SCHEDULE_ID_BLOCK_3       = 3;
    constexpr uint8_t TEST_HOME_ID_BYTES[]      = {0x12, 0x34, 0x56, 0x78};
    constexpr uint16_t TEST_NODE_ID             = 2;
    constexpr uint8_t TEST_ENDPOINT_ID          = 0;
    constexpr uint32_t FIRST_CHILD_INDEX        = 0;
}  // namespace

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class TestableScheduleMqtt : public command_class_schedule_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class ScheduleCommandClassTest : public ::testing::Test
{
    protected:
        static command_class_schedule &component()
        {
            static command_class_schedule instance;
            return instance;
        }

        static TestableScheduleMqtt &mqtt()
        {
            static TestableScheduleMqtt instance;
            return instance;
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto root        = attribute_store::attribute::root();
            auto home        = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(std::begin(TEST_HOME_ID_BYTES), std::end(TEST_HOME_ID_BYTES)));
            auto node        = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint         = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            endpoint_node_id = attribute_store_get_node_child_by_type(attribute_store_get_node_child_by_type(attribute_store_get_node_child_by_type(attribute_store_get_root(), ATTRIBUTE_HOME_ID, FIRST_CHILD_INDEX), ATTRIBUTE_NODE_ID, FIRST_CHILD_INDEX), ATTRIBUTE_ENDPOINT_ID, FIRST_CHILD_INDEX);
            (void)component();
            (void)mqtt();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        template<typename AttributeEnum, typename Value> void desired(attribute_store::attribute group, AttributeEnum type, Value value)
        {
            group.emplace_node(static_cast<attribute_store_type_t>(type)).set_desired(value);
        }

        attribute_store::attribute endpoint;
        attribute_store_node_t endpoint_node_id = ATTRIBUTE_STORE_INVALID_NODE;

        void finish_interview(uint8_t supported_version)
        {
            // Advertise Schedule CC on the endpoint and record the peer's negotiated version.
            endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t> {SCHEDULE_COMMAND_CLASS_ID});
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_schedule_attributes_t::supported_version)).set_reported(supported_version);

            // Deliver interview completion synchronously so the test can inspect queued groups.
            component_connector connector;
            const component_connector_interview_done_payload_t payload {endpoint_node_id, SL_STATUS_OK};
            auto status = connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload);
            connector.run();
            ASSERT_EQ(SL_STATUS_OK, status.get());
        }

        void expect_schedule_set_frame(uint8_t peer_version, uint8_t schedule_id_block, uint8_t recurrence_offset, uint8_t recurrence_mode, uint8_t relative, const std::vector<uint8_t> &expected_frame)
        {
            // Fill a valid Set request: Schedule ID 3, calendar start fields, 90-minute duration, and one nested command.
            auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::COMMAND_SCHEDULE_SET_GROUP));
            desired(group, command_schedule_set_group_attributes_t::schedule_id, SCHEDULE_ID_FOR_SET_TEST);
            desired(group, command_schedule_set_group_attributes_t::schedule_id_block, schedule_id_block);
            desired(group, command_schedule_set_group_attributes_t::start_year, SCHEDULE_START_YEAR);
            desired(group, command_schedule_set_group_attributes_t::start_month, SCHEDULE_START_MONTH);
            desired(group, command_schedule_set_group_attributes_t::recurrence_offset, recurrence_offset);
            desired(group, command_schedule_set_group_attributes_t::start_day_of_month, SCHEDULE_START_DAY);
            desired(group, command_schedule_set_group_attributes_t::recurrence_mode, recurrence_mode);
            desired(group, command_schedule_set_group_attributes_t::start_weekday, SCHEDULE_START_WEEKDAY);
            desired(group, command_schedule_set_group_attributes_t::start_hour, SCHEDULE_START_HOUR);
            desired(group, command_schedule_set_group_attributes_t::duration_type, SCHEDULE_DURATION_TYPE);
            desired(group, command_schedule_set_group_attributes_t::start_minute, SCHEDULE_START_MINUTE);
            desired(group, command_schedule_set_group_attributes_t::relative, relative);
            desired(group, command_schedule_set_group_attributes_t::duration_byte, SCHEDULE_DURATION);
            desired(group, command_schedule_set_group_attributes_t::reports_to_follow, SCHEDULE_REPORTS_TO_FOLLOW);
            desired(group, command_schedule_set_group_attributes_t::number_of_cmd_to_follow, SCHEDULE_COMMANDS_TO_FOLLOW);
            desired(group, command_schedule_set_group_attributes_t::vg1, std::vector<uint8_t> {NESTED_COMMAND_LENGTH, NESTED_COMMAND_CLASS, NESTED_COMMAND_ID});

            // Record the peer version so the test documents the wire definition being asserted.
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_schedule_attributes_t::supported_version)).set_reported(peer_version);

            // Assemble one Schedule Set and compare each byte in protocol order.
            constexpr size_t SCHEDULE_SET_FRAME_CAPACITY = 32;
            uint8_t data[SCHEDULE_SET_FRAME_CAPACITY] {};
            uint16_t length = 0;
            zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
            generator.initialize_frame(COMMAND_SCHEDULE_SET_COMMAND_ID, data, &length);
            command_class_schedule_core::set_requested_args args {group, &generator};
            ASSERT_EQ(SL_STATUS_OK, component().on_command_schedule_set_requested(args, data, &length));
            EXPECT_EQ(expected_frame, std::vector<uint8_t>(data, data + length));
        }

        sl_status_t receive_supported_report_frame(const uint8_t *frame, uint16_t frame_length)
        {
            // Parse a complete CC+Command frame through the same entry point used by received reports.
            zwave_frame_parser parser {frame, frame_length};
            command_class_schedule_core::report_received_args args {endpoint, parser, nullptr};
            return component().on_schedule_supported_report_received(args);
        }

        void expect_supported_get_frame_for_peer_version(uint8_t peer_version)
        {
            // Create a Supported Get for block zero, which the v2 specification defines as the default block.
            auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP));
            desired(group, schedule_supported_get_group_attributes_t::schedule_id_block, DEFAULT_SCHEDULE_ID_BLOCK);

            // Record the destination's version while assembling the current v2 command format.
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_schedule_attributes_t::supported_version)).set_reported(peer_version);

            // The v2 block selector trails the v1 no-payload command, so the older peer sees its valid v1 prefix.
            constexpr size_t SUPPORTED_GET_FRAME_LENGTH = 3;
            uint8_t data[SUPPORTED_GET_FRAME_LENGTH] {};
            uint16_t length = 0;
            zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
            generator.initialize_frame(SCHEDULE_SUPPORTED_GET_COMMAND_ID, data, &length);
            command_class_schedule_core::get_requested_args args {group, &generator};
            ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_get_requested(args, data, &length));

            // Check CC ID, Get ID, then the default-block selector appended in v2.
            const std::vector<uint8_t> expected_frame {SCHEDULE_COMMAND_CLASS_ID, SCHEDULE_SUPPORTED_GET_COMMAND_ID, DEFAULT_SCHEDULE_ID_BLOCK};
            EXPECT_EQ(expected_frame, std::vector<uint8_t>(data, data + length));
        }
};

// CC:0053.02.02.11.001, CC:0053.02.02.11.002
// Verifies a version 2 Supported Report stores block count and packed supported-CC entries.
TEST_F(ScheduleCommandClassTest, SupportedReportV2StoresBlockCapabilities)
{
    // Provide two supported-CC entries; each wire entry is a Command Class ID followed by its properties byte.
    schedule_supported_report_vg1_t supported_ccs(2);
    supported_ccs[0].supported_cc      = SUPPORTED_CC_SWITCH_MULTILEVEL;
    supported_ccs[0].properties2.value = SUPPORTED_CC_GET_SET_FLAGS;
    supported_ccs[1].supported_cc      = SUPPORTED_CC_SWITCH_BINARY;
    supported_ccs[1].properties2.value = SUPPORTED_CC_SET_FLAG;

    // Set each Supported Report field: block 2 has five regular IDs, two supported CCs,
    // three total blocks, and the specified start-time, fallback, and override capabilities.
    command_class_schedule_attribute_map_t report {
      {"schedule_id_block", SCHEDULE_ID_BLOCK_2},                               // This report describes the second Schedule ID Block.
      {"number_of_supported_schedule_id", SUPPORTED_SCHEDULE_COUNT},            // This block contains five schedule IDs.
      {"start_time_support", START_TIME_SUPPORT_V2},                            // Advertises Start now and Calendar time in v2.
      {"fallback_support", uint8_t {1}},                                        // Bit 0 says this block supports a Fall Back Schedule.
      {"support_enable_disable", uint8_t {1}},                                  // Bit 0 says schedules can be enabled or disabled.
      {"number_of_supported_cc", SUPPORTED_CC_COUNT},                           // Two command classes follow in variable group 1.
      {"vg1", supported_ccs},                                                   // The two entries and their per-class Get/Set properties are above.
      {"supported_override_types", SUPPORTED_OVERRIDE_TYPES},                   // Bitmask advertises Time and No Override types.
      {"override_support", uint8_t {1}},                                        // Bit 0 says the node supports Override Schedules.
      {"number_of_supported_schedule_blocks", SUPPORTED_SCHEDULE_BLOCK_COUNT},  // Total blocks advertised by the node.
    };

    // Store the decoded report and locate the group identified by Schedule ID Block 2.
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_report_received_store(endpoint, report));
    auto group = command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_2);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SUPPORTED_SCHEDULE_COUNT, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_id)).reported<uint8_t>());
    EXPECT_EQ(START_TIME_SUPPORT_V2, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::start_time_support)).reported<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {SUPPORTED_CC_SWITCH_MULTILEVEL, SUPPORTED_CC_GET_SET_FLAGS, SUPPORTED_CC_SWITCH_BINARY, SUPPORTED_CC_SET_FLAG}), group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>());
    EXPECT_EQ(SUPPORTED_OVERRIDE_TYPES, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::supported_override_types)).reported<uint8_t>());
    EXPECT_EQ(SUPPORTED_SCHEDULE_BLOCK_COUNT, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_blocks)).reported<uint8_t>());
    EXPECT_TRUE(group == command_class_schedule_attribute_store::find_or_create_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_2));
}

// CC:0053.03.02.11.003
// Verifies a version 3 Supported Report preserves the Time from now start-time bit.
TEST_F(ScheduleCommandClassTest, SupportedReportV3StoresTimeFromNowCapability)
{
    // Version 3 assigns bit 4 of Start Time Support to the Time from now option.
    command_class_schedule_attribute_map_t report {
      {"schedule_id_block", SCHEDULE_ID_BLOCK_1},                     // Store this report under the default block.
      {"number_of_supported_schedule_id", SUPPORTED_SCHEDULE_COUNT},  // Advertise the fixture's five schedules.
      {"start_time_support", START_TIME_SUPPORT_V3},                  // Bits 0, 2, and 4 advertise all three supported start options.
    };

    // Store the report and verify the v3 bitmask remains unchanged.
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_report_received_store(endpoint, report));
    const auto group = command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_1);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(START_TIME_SUPPORT_V3, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::start_time_support)).reported<uint8_t>());
}

// CC:0053.04.0B.11.001
// Verifies v4 supported-command entries are stored as class, byte count, and command bytes.
TEST_F(ScheduleCommandClassTest, StoresSupportedCommandsReportAndUsesDefaultBlock)
{
    // Each Supported Commands entry contains its class ID, number of command bytes, and command IDs.
    schedule_supported_commands_report_vg1_t commands {
      {SUPPORTED_CC_SWITCH_MULTILEVEL, SUPPORTED_COMMAND_CLASS_COUNT_1, {SWITCH_MULTILEVEL_SET_COMMAND, SWITCH_MULTILEVEL_GET_COMMAND}},
      {SUPPORTED_CC_SWITCH_BINARY, SUPPORTED_COMMAND_CLASS_COUNT_2, {SWITCH_BINARY_SET_COMMAND}},
    };
    // Omitting the optional block selects the default block, and list length counts the two class entries.
    command_class_schedule_attribute_map_t report {{"command_class_list_length", SUPPORTED_COMMAND_LIST_LENGTH}, {"vg1", commands}};

    // Store the v4 report and find its implicit default-block group.
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_commands_report_received_store(endpoint, report));
    auto group = command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SUPPORTED_COMMAND_LIST_LENGTH, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::command_class_list_length)).reported<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {SUPPORTED_CC_SWITCH_MULTILEVEL, SUPPORTED_COMMAND_CLASS_COUNT_1, SWITCH_MULTILEVEL_SET_COMMAND, SWITCH_MULTILEVEL_GET_COMMAND, SUPPORTED_CC_SWITCH_BINARY, SUPPORTED_COMMAND_CLASS_COUNT_2, SWITCH_BINARY_SET_COMMAND}),
              group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>());
    EXPECT_TRUE(group == command_class_schedule_attribute_store::find_or_create_supported_commands_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK));
}

// Verifies version 2 Supported Report fields with decoded types wider than one byte use defaults.
TEST_F(ScheduleCommandClassTest, SupportedReportV2WrongTypesUseDefaults)
{
    // Supply 16-bit values for one-byte Supported Report fields; the map reader must reject each mismatched type.
    command_class_schedule_attribute_map_t supported_report {
      {"schedule_id_block", WRONG_TYPE_BLOCK_COUNT},
      {"number_of_supported_schedule_id", WRONG_TYPE_SCHEDULE_ID_COUNT},
      {"start_time_support", WRONG_TYPE_START_TIME_SUPPORT},
      {"fallback_support", WRONG_TYPE_START_TIME_SUPPORT},
      {"support_enable_disable", WRONG_TYPE_START_TIME_SUPPORT},
      {"number_of_supported_cc", WRONG_TYPE_SUPPORTED_CC_COUNT},
      {"vg1", uint8_t {SUPPORTED_SCHEDULE_COUNT}},  // vg1 expects its typed collection, not one byte.
      {"supported_override_types", WRONG_TYPE_START_TIME_SUPPORT},
      {"override_support", WRONG_TYPE_START_TIME_SUPPORT},
      {"number_of_supported_schedule_blocks", WRONG_TYPE_BLOCK_COUNT},
    };

    // Store the map and verify the wrong-typed count and variable group fall back to defaults.
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_report_received_store(endpoint, supported_report));
    auto group = command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SCHEDULE_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_id)).reported<uint8_t>());
    EXPECT_TRUE(group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>().empty());
}

// Verifies version 4 Supported Commands fields with invalid decoded types use defaults.
TEST_F(ScheduleCommandClassTest, SupportedCommandsReportV4WrongTypesUseDefaults)
{
    // A two-byte integer is the wrong decoded type for each one-byte report field and the variable group.
    command_class_schedule_attribute_map_t commands_report {
      {"schedule_id_block", WRONG_TYPE_BLOCK_COUNT},
      {"command_class_list_length", WRONG_TYPE_COMMAND_LIST_LENGTH},
      {"vg1", uint8_t {SUPPORTED_COMMAND_LIST_LENGTH}},
    };

    // Store the invalid map and confirm the count and command list default to empty values.
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_commands_report_received_store(endpoint, commands_report));
    auto commands = command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK);
    ASSERT_TRUE(commands.is_valid());
    EXPECT_EQ(SCHEDULE_DEFAULT_VALUE, commands.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::command_class_list_length)).reported<uint8_t>());
    EXPECT_TRUE(commands.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>().empty());
}

// Verifies both report-group lookups skip incomplete, unreported, and nonmatching groups.
TEST_F(ScheduleCommandClassTest, ReportGroupLookupsSkipIncompleteAndNonmatchingGroups)
{
    const auto supported_type = static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::SCHEDULE_SUPPORTED_REPORT_GROUP);
    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK).is_valid());
    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK).is_valid());

    auto incomplete = endpoint.add_node(supported_type);
    incomplete.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::schedule_id_block));
    auto unreported = endpoint.add_node(supported_type);
    unreported.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::schedule_id_block));
    auto different = endpoint.add_node(supported_type);
    different.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::schedule_id_block)).set_reported(uint8_t {1});

    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK).is_valid());
    auto created = command_class_schedule_attribute_store::find_or_create_supported_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK);
    ASSERT_TRUE(created.is_valid());
    EXPECT_TRUE(created == command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK));

    const auto commands_type = static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_REPORT_GROUP);
    auto commands_unreported = endpoint.add_node(commands_type);
    commands_unreported.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::schedule_id_block));
    auto commands_other = endpoint.add_node(commands_type);
    commands_other.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::schedule_id_block)).set_reported(uint8_t {1});
    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK).is_valid());
    auto commands_created = command_class_schedule_attribute_store::find_or_create_supported_commands_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK);
    ASSERT_TRUE(commands_created.is_valid());
    EXPECT_TRUE(commands_created == command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK));
}

// Verifies orphan report groups without a block child are skipped and never reused as a keyed report.
TEST_F(ScheduleCommandClassTest, ReportGroupLookupsSkipGroupsWithoutBlockChild)
{
    const auto supported_type   = static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::SCHEDULE_SUPPORTED_REPORT_GROUP);
    const auto commands_type    = static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_REPORT_GROUP);
    const auto orphan_supported = endpoint.add_node(supported_type);
    const auto orphan_commands  = endpoint.add_node(commands_type);

    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_3).is_valid());
    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_3).is_valid());

    const auto supported = command_class_schedule_attribute_store::find_or_create_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_3);
    const auto commands  = command_class_schedule_attribute_store::find_or_create_supported_commands_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_3);
    ASSERT_TRUE(supported.is_valid());
    ASSERT_TRUE(commands.is_valid());
    EXPECT_FALSE(supported == orphan_supported);
    EXPECT_FALSE(commands == orphan_commands);
    EXPECT_EQ(supported, command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_3));
    EXPECT_EQ(commands, command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_3));
    EXPECT_EQ(2, endpoint.children(supported_type).size());
    EXPECT_EQ(2, endpoint.children(commands_type).size());
}

// CC:0053.01.01.11.001
// Verifies a version 1 interview requests the default Schedule ID Block capabilities.
TEST_F(ScheduleCommandClassTest, InterviewV1RequestsDefaultBlockCapabilities)
{
    // Version 1 has only the base Supported Get/Report exchange.
    finish_interview(SCHEDULE_VERSION_1);

    // The interview stores block zero locally to represent v1's implicit default block.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
}

// CC:0053.02.01.11.001
// Verifies a version 2 interview still queries the default block after ID Blocks are introduced.
TEST_F(ScheduleCommandClassTest, InterviewV2RequestsDefaultBlockCapabilities)
{
    // Version 2 adds block selection, while the interview starts with the default block.
    finish_interview(SCHEDULE_VERSION_2);

    // Version 2 does not define the Supported Commands Get introduced in version 4.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
}

// CC:0053.03.02.11.001
// Verifies a version 3 interview queries the default block without sending a v4-only command.
TEST_F(ScheduleCommandClassTest, InterviewV3RequestsDefaultBlockCapabilities)
{
    // Version 3 adds recurrence options but retains the same interview capability request.
    finish_interview(SCHEDULE_VERSION_3);

    // Supported Commands Get is only available beginning with version 4.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
}

// CC:0053.04.0A.11.001
// Verifies a version 4 interview also requests the new Supported Commands report.
TEST_F(ScheduleCommandClassTest, InterviewV4RequestsSupportedCommands)
{
    // Version 4 adds the Supported Commands Get/Report exchange.
    finish_interview(SCHEDULE_VERSION_4);

    // Both capability queries are scheduled for the default block.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
}

// Verifies each registered MQTT route dispatches to its corresponding Schedule command handler.
TEST_F(ScheduleCommandClassTest, RegisteredMqttCommandsDispatch)
{
    // These payloads describe a valid v4 Schedule Set and valid block-aware requests.
    const std::vector<std::pair<std::string, std::string>> commands {
      {"ScheduleSupportedGet", R"({"schedule_id_block":2})"},
      {"CommandScheduleSet",
       R"({"schedule_id":3,"schedule_id_block":1,"start_year":26,"duration_byte":90,"reports_to_follow":0,"properties1":{"start_month":4,"recurrence_offset":2},"properties2":{"start_day_of_month":9,"recurrence_mode":1},"properties3":{"start_weekday":5},"properties4":{"start_hour":10,"duration_type":2},"properties5":{"start_minute":12,"relative":1},"vg1":[]})"},
      {"CommandScheduleGet", R"({"schedule_id":7,"schedule_id_block":2,"properties1":{"aid_ro_ctl":1}})"},
      {"ScheduleRemove", R"({"schedule_id":7,"schedule_id_block":2})"},
      {"ScheduleStateSet", R"({"schedule_id":7,"schedule_state":1,"schedule_id_block":2})"},
      {"ScheduleStateGet", R"({"schedule_id_block":2})"},
      {"ScheduleSupportedCommandsGet", R"({"schedule_id_block":2})"},
    };

    // Dispatch every supported command name through the MQTT handler.
    for (const auto &[name, payload]: commands) {
        mqtt().mqtt_command_handler(endpoint, name, payload);
    }

    // Repeat the same requests to exercise updates to existing attribute groups.
    for (const auto &[name, payload]: commands) {
        mqtt().mqtt_command_handler(endpoint, name, payload);
    }

    // Confirm each MQTT route created the matching resolution group.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::COMMAND_SCHEDULE_SET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::COMMAND_SCHEDULE_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::SCHEDULE_REMOVE_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::SCHEDULE_STATE_SET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_state_get_group_attributes_t::SCHEDULE_STATE_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
}

// CC:0053.02.02.11.003
// Verifies a Supported Report starts the next block query only while another block remains.
TEST_F(ScheduleCommandClassTest, SupportedReportAdvancesBlockInterviewUntilLastBlock)
{
    // A report advertising three blocks advances from block zero to one, then stops at block two.
    const auto receive_supported_report = [this](uint8_t block, uint8_t number_of_blocks) {
        const uint8_t frame[] = {0x53, 0x02, 0, 0, 0, 0, block, number_of_blocks};
        zwave_frame_parser parser {frame, sizeof(frame)};
        command_class_schedule_core::report_received_args args {endpoint, parser, nullptr};
        return component().on_schedule_supported_report_received(args);
    };
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(0, 3));
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(1, 3));
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(2, 3));
    EXPECT_TRUE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 1).is_valid());
    EXPECT_TRUE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 2).is_valid());
    EXPECT_EQ(SL_STATUS_OK, receive_supported_report(0, 0));
}

// CC:0053.01.02.11.001, CC:0053.01.02.11.005
// Verifies the parser accepts the shorter version 1 Supported Report without v2 block fields.
TEST_F(ScheduleCommandClassTest, SupportedReportV1ParsesShortLegacyFrame)
{
    // A v1 Supported Report ends after Properties 3; it has no Schedule ID Block or block count.
    const uint8_t frame[] {
      SCHEDULE_COMMAND_CLASS_ID,
      SCHEDULE_SUPPORTED_REPORT_COMMAND_ID,
      SUPPORTED_SCHEDULE_COUNT,         // Number of regular Schedule IDs.
      SUPPORTED_REPORT_PROPERTIES_V1,   // Start now and calendar time are supported.
      SUPPORTED_REPORT_CC_COUNT_NONE,   // No scheduled Command Classes are listed.
      SUPPORTED_REPORT_OVERRIDE_FLAGS,  // Override-type and Override-support flags.
    };

    // Parse the legacy frame and verify its version 1 fields were retained in the default group.
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report_frame(frame, sizeof(frame)));
    const auto group = command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, DEFAULT_SCHEDULE_ID_BLOCK);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SUPPORTED_SCHEDULE_COUNT, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_id)).reported<uint8_t>());
    EXPECT_EQ(START_TIME_SUPPORT_V2, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::start_time_support)).reported<uint8_t>());
    EXPECT_EQ(SCHEDULE_DEFAULT_VALUE, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_blocks)).reported<uint8_t>());
}

// CC:0053.02.02.11.001, CC:0053.02.02.11.003
// Verifies the parser reads the v2 block fields appended after the version 1 Supported Report prefix.
TEST_F(ScheduleCommandClassTest, SupportedReportV2ParsesExtendedFrameAndRequestsNextBlock)
{
    // Version 2 appends the actual block ID and total block count after the version 1 fields.
    const uint8_t frame[] {
      SCHEDULE_COMMAND_CLASS_ID,
      SCHEDULE_SUPPORTED_REPORT_COMMAND_ID,
      SUPPORTED_SCHEDULE_COUNT,         // Number of regular Schedule IDs in this block.
      SUPPORTED_REPORT_PROPERTIES_V1,   // The two advertised version 1 start-time options.
      SUPPORTED_REPORT_CC_COUNT_NONE,   // No scheduled Command Classes are listed.
      SUPPORTED_REPORT_OVERRIDE_FLAGS,  // Override-type and Override-support flags.
      SCHEDULE_ID_BLOCK_1,              // The report describes block one.
      SUPPORTED_SCHEDULE_BLOCK_COUNT,   // The node supports three Schedule ID Blocks.
    };

    // Parse the complete v2 frame and verify the report is associated with block one.
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report_frame(frame, sizeof(frame)));
    const auto group = command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_1);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(SUPPORTED_SCHEDULE_BLOCK_COUNT, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_blocks)).reported<uint8_t>());

    // Block two is the next unreported block, so interview creates its report group and Get request.
    EXPECT_TRUE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, SCHEDULE_ID_BLOCK_2).is_valid());
    const auto next_get = endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP));
    ASSERT_TRUE(next_get.is_valid());
    EXPECT_EQ(SCHEDULE_ID_BLOCK_2, next_get.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::schedule_id_block)).desired<uint8_t>());
}

// Verifies a Supported Report stops when no blocks exist or the current block is the last one.
TEST_F(ScheduleCommandClassTest, SupportedReportDoesNotAdvancePastZeroOrLastBlock)
{
    const auto receive_supported_report = [this](uint8_t block, uint8_t number_of_blocks) {
        const uint8_t frame[] = {0x53, 0x02, 0, 0, 0, 0, block, number_of_blocks};
        zwave_frame_parser parser {frame, sizeof(frame)};
        command_class_schedule_core::report_received_args args {endpoint, parser, nullptr};
        return component().on_schedule_supported_report_received(args);
    };

    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(0, 0));
    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 1).is_valid());
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(1, 2));
    EXPECT_FALSE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 2).is_valid());
}

// CC:0000.00.00.11.002, CC:0053.02.01.11.006
// Verifies a version 1 peer receives the current Supported Get with its valid v1 no-payload prefix.
TEST_F(ScheduleCommandClassTest, SupportedGetV1PeerReceivesCurrentFormat)
{
    // Block zero asks for the default block in v2 and is safely ignored as trailing data by a v1 peer.
    expect_supported_get_frame_for_peer_version(SCHEDULE_VERSION_1);
}

// CC:0053.02.01.11.005
// Verifies a version 2 Supported Get encodes the default Schedule ID Block selector.
TEST_F(ScheduleCommandClassTest, SupportedGetV2EncodesDefaultBlock)
{
    // Version 2 defines selector zero as a request for the default block, which is block one.
    expect_supported_get_frame_for_peer_version(SCHEDULE_VERSION_2);
}

// CC:0053.02.01.11.001
// Verifies a Supported Get request is not ready until its required block selector is present.
TEST_F(ScheduleCommandClassTest, SupportedGetRequiresBlockSelector)
{
    // Create a request group without the required v2 Schedule ID Block selector.
    auto group                                    = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP));
    constexpr size_t SUPPORTED_GET_FRAME_CAPACITY = 3;
    uint8_t data[SUPPORTED_GET_FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
    generator.initialize_frame(SCHEDULE_SUPPORTED_GET_COMMAND_ID, data, &length);
    command_class_schedule_core::get_requested_args args {group, &generator};

    // Without a desired selector, the frame assembler must leave the request unready.
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_supported_get_requested(args, data, &length));
}

// CC:0000.00.00.11.002, CC:0053.01.03.11.001
// Verifies a version 1 peer receives the current Schedule Set format and can interpret its v1-defined prefix.
TEST_F(ScheduleCommandClassTest, CommandScheduleSetV1PeerReceivesCurrentFormat)
{
    // Schedule Set exists in v1, so compatibility keeps the current appended block field in the outgoing frame.
    // Recurrence-only bits are zero, leaving fields introduced later without values the v1 peer cannot interpret.
    const std::vector<uint8_t> expected_frame {
      SCHEDULE_COMMAND_CLASS_ID,
      COMMAND_SCHEDULE_SET_COMMAND_ID,
      SCHEDULE_ID_FOR_SET_TEST,
      SCHEDULE_ID_BLOCK_1,
      SCHEDULE_START_YEAR,
      SCHEDULE_START_MONTH,
      SCHEDULE_START_DAY,
      SCHEDULE_START_WEEKDAY,
      PACKED_DURATION_TYPE_AND_HOUR,
      V2_PACKED_START_MINUTE,
      SCHEDULE_START_SECOND,
      static_cast<uint8_t>(SCHEDULE_DURATION),
      SCHEDULE_REPORTS_TO_FOLLOW,
      SCHEDULE_COMMANDS_TO_FOLLOW,
      NESTED_COMMAND_LENGTH,
      NESTED_COMMAND_CLASS,
      NESTED_COMMAND_ID,
    };

    // The current block field is present; the zero recurrence inputs preserve a valid version 1 prefix.
    expect_schedule_set_frame(SCHEDULE_VERSION_1, SCHEDULE_ID_BLOCK_1, NO_RECURRENCE_OFFSET, NO_RECURRENCE_MODE, RELATIVE_START_DISABLED, expected_frame);
}

// CC:0053.02.03.11.004
// Verifies a version 2 peer receives Schedule ID Block after Schedule ID and before Start Year.
TEST_F(ScheduleCommandClassTest, CommandScheduleSetV2PeerIncludesScheduleIdBlock)
{
    // Version 2 inserts the selected block after Schedule ID; v3-only recurrence bits stay zero.
    const std::vector<uint8_t> expected_frame {
      SCHEDULE_COMMAND_CLASS_ID,
      COMMAND_SCHEDULE_SET_COMMAND_ID,
      SCHEDULE_ID_FOR_SET_TEST,
      SCHEDULE_ID_BLOCK_1,
      SCHEDULE_START_YEAR,
      SCHEDULE_START_MONTH,
      SCHEDULE_START_DAY,
      SCHEDULE_START_WEEKDAY,
      PACKED_DURATION_TYPE_AND_HOUR,
      V2_PACKED_START_MINUTE,
      SCHEDULE_START_SECOND,
      static_cast<uint8_t>(SCHEDULE_DURATION),
      SCHEDULE_REPORTS_TO_FOLLOW,
      SCHEDULE_COMMANDS_TO_FOLLOW,
      NESTED_COMMAND_LENGTH,
      NESTED_COMMAND_CLASS,
      NESTED_COMMAND_ID,
    };

    // Block 1 is the default block and is a valid choice for a v2 peer.
    expect_schedule_set_frame(SCHEDULE_VERSION_2, SCHEDULE_ID_BLOCK_1, NO_RECURRENCE_OFFSET, NO_RECURRENCE_MODE, RELATIVE_START_DISABLED, expected_frame);
}

// CC:0053.03.03.11.005, CC:0053.03.03.11.009
// Verifies a version 3 peer receives packed Recurrence Offset, Recurrence Mode, and Relative fields.
TEST_F(ScheduleCommandClassTest, CommandScheduleSetV3PeerPacksRecurrenceFields)
{
    // Version 3 uses the upper four month bits for recurrence offset, two day bits for recurrence mode,
    // and the high minute bit for Relative; the remaining field positions match version 2.
    const std::vector<uint8_t> expected_frame {
      SCHEDULE_COMMAND_CLASS_ID,
      COMMAND_SCHEDULE_SET_COMMAND_ID,
      SCHEDULE_ID_FOR_SET_TEST,
      SCHEDULE_ID_BLOCK_1,
      SCHEDULE_START_YEAR,
      V3_PACKED_MONTH_AND_RECURRENCE,
      V3_PACKED_DAY_AND_MODE,
      SCHEDULE_START_WEEKDAY,
      PACKED_DURATION_TYPE_AND_HOUR,
      V3_PACKED_RELATIVE_AND_MINUTE,
      SCHEDULE_START_SECOND,
      static_cast<uint8_t>(SCHEDULE_DURATION),
      SCHEDULE_REPORTS_TO_FOLLOW,
      SCHEDULE_COMMANDS_TO_FOLLOW,
      NESTED_COMMAND_LENGTH,
      NESTED_COMMAND_CLASS,
      NESTED_COMMAND_ID,
    };

    // Offset 2 in Recurrence Mode 1 represents a two-day recurrence; Relative is enabled.
    expect_schedule_set_frame(SCHEDULE_VERSION_3, SCHEDULE_ID_BLOCK_1, SCHEDULE_RECURRENCE_OFFSET, SCHEDULE_RECURRENCE_MODE, SCHEDULE_RELATIVE_FLAG, expected_frame);
}

// CC:0053.04.03.11.005, CC:0053.04.03.11.006
// Verifies a version 4 peer receives the v3-compatible Set layout with obsolete Reports to Follow set to zero.
TEST_F(ScheduleCommandClassTest, CommandScheduleSetV4PeerUsesCurrentFieldLayout)
{
    // Version 4 keeps the v3 Schedule Set fields and requires the obsolete Reports to Follow byte to be zero.
    const std::vector<uint8_t> expected_frame {
      SCHEDULE_COMMAND_CLASS_ID,
      COMMAND_SCHEDULE_SET_COMMAND_ID,
      SCHEDULE_ID_FOR_SET_TEST,
      SCHEDULE_ID_BLOCK_1,
      SCHEDULE_START_YEAR,
      V3_PACKED_MONTH_AND_RECURRENCE,
      V3_PACKED_DAY_AND_MODE,
      SCHEDULE_START_WEEKDAY,
      PACKED_DURATION_TYPE_AND_HOUR,
      V3_PACKED_RELATIVE_AND_MINUTE,
      SCHEDULE_START_SECOND,
      static_cast<uint8_t>(SCHEDULE_DURATION),
      SCHEDULE_REPORTS_TO_FOLLOW,
      SCHEDULE_COMMANDS_TO_FOLLOW,
      NESTED_COMMAND_LENGTH,
      NESTED_COMMAND_CLASS,
      NESTED_COMMAND_ID,
    };

    // Reuse v3-valid values: the current version leaves this existing command's field layout unchanged.
    expect_schedule_set_frame(SCHEDULE_VERSION_4, SCHEDULE_ID_BLOCK_1, SCHEDULE_RECURRENCE_OFFSET, SCHEDULE_RECURRENCE_MODE, SCHEDULE_RELATIVE_FLAG, expected_frame);
}

// CC:0053.02.03.11.001, CC:0053.03.03.11.001
// Verifies every required Schedule Set field is checked before a frame is assembled.
TEST_F(ScheduleCommandClassTest, CommandScheduleSetRequiresEveryField)
{
    using group_type = command_schedule_set_group_attributes_t;
    const std::array<std::function<void(attribute_store::attribute)>, REQUIRED_SCHEDULE_SET_FIELD_COUNT> setters {{
      [this](auto group) { desired(group, group_type::schedule_id, SCHEDULE_ID_FOR_SET_TEST); },
      [this](auto group) { desired(group, group_type::schedule_id_block, SCHEDULE_ID_BLOCK_1); },
      [this](auto group) { desired(group, group_type::start_year, SCHEDULE_START_YEAR); },
      [this](auto group) { desired(group, group_type::start_month, SCHEDULE_START_MONTH); },
      [this](auto group) { desired(group, group_type::recurrence_offset, SCHEDULE_RECURRENCE_OFFSET); },
      [this](auto group) { desired(group, group_type::start_day_of_month, SCHEDULE_START_DAY); },
      [this](auto group) { desired(group, group_type::recurrence_mode, SCHEDULE_RECURRENCE_MODE); },
      [this](auto group) { desired(group, group_type::start_weekday, SCHEDULE_START_WEEKDAY); },
      [this](auto group) { desired(group, group_type::start_hour, SCHEDULE_START_HOUR); },
      [this](auto group) { desired(group, group_type::duration_type, SCHEDULE_DURATION_TYPE); },
      [this](auto group) { desired(group, group_type::start_minute, SCHEDULE_START_MINUTE); },
      [this](auto group) { desired(group, group_type::relative, SCHEDULE_RELATIVE_FLAG); },
      [this](auto group) { desired(group, group_type::duration_byte, SCHEDULE_DURATION); },
      [this](auto group) { desired(group, group_type::reports_to_follow, SCHEDULE_REPORTS_TO_FOLLOW); },
      [this](auto group) { desired(group, group_type::number_of_cmd_to_follow, NO_FOLLOWING_SCHEDULE_COMMANDS); },
      [this](auto group) { desired(group, group_type::vg1, std::vector<uint8_t> {}); },
    }};

    uint8_t data[SCHEDULE_FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
    for (size_t missing = 0; missing < setters.size(); ++missing) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::COMMAND_SCHEDULE_SET_GROUP));
        for (size_t field = 0; field < setters.size(); ++field) {
            if (field != missing) {
                setters[field](group);
            }
        }
        generator.initialize_frame(COMMAND_SCHEDULE_SET_COMMAND_ID, data, &length);
        command_class_schedule_core::set_requested_args args {group, &generator};
        EXPECT_EQ(SL_STATUS_NOT_READY, component().on_command_schedule_set_requested(args, data, &length)) << "missing field index " << missing;
    }
}

// CC:0053.02.04.11.001
// Verifies a Schedule Get requires its ID, block, and override control, then encodes those fields in order.
TEST_F(ScheduleCommandClassTest, ScheduleGetEncodesScheduleBlockAndOverrideControl)
{
    constexpr uint8_t SCHEDULE_ID_FOR_GET_TEST = 7;
    constexpr uint8_t OVERRIDE_CONTROL_ENABLED = 1;
    constexpr uint8_t PACKED_OVERRIDE_CONTROL  = 0x80;
    constexpr size_t FRAME_CAPACITY            = 8;
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};

    // Build the Get group incrementally to verify each required field blocks frame assembly.
    auto get = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::COMMAND_SCHEDULE_GET_GROUP));
    command_class_schedule_core::get_requested_args get_args {get, &generator};
    generator.initialize_frame(COMMAND_SCHEDULE_GET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_command_schedule_get_requested(get_args, data, &length));
    desired(get, command_schedule_get_group_attributes_t::schedule_id, SCHEDULE_ID_FOR_GET_TEST);
    generator.initialize_frame(COMMAND_SCHEDULE_GET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_command_schedule_get_requested(get_args, data, &length));
    desired(get, command_schedule_get_group_attributes_t::schedule_id_block, SCHEDULE_ID_BLOCK_2);
    generator.initialize_frame(COMMAND_SCHEDULE_GET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_command_schedule_get_requested(get_args, data, &length));
    desired(get, command_schedule_get_group_attributes_t::aid_ro_ctl, OVERRIDE_CONTROL_ENABLED);
    generator.initialize_frame(COMMAND_SCHEDULE_GET_COMMAND_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_command_schedule_get_requested(get_args, data, &length));
    // Bit 7 carries the override-control flag in Properties1; the other output bytes are IDs.
    const std::vector<uint8_t> expected_frame {SCHEDULE_COMMAND_CLASS_ID, COMMAND_SCHEDULE_GET_COMMAND_ID, SCHEDULE_ID_FOR_GET_TEST, SCHEDULE_ID_BLOCK_2, PACKED_OVERRIDE_CONTROL};
    EXPECT_EQ(expected_frame, std::vector<uint8_t>(data, data + length));
}

// CC:0053.02.06.11.001
// Verifies Schedule Remove waits for both the schedule ID and the block selector before encoding.
TEST_F(ScheduleCommandClassTest, ScheduleRemoveV2EncodesScheduleAndBlockIds)
{
    constexpr uint8_t SCHEDULE_ID_FOR_REMOVE_TEST = 7;
    constexpr size_t FRAME_CAPACITY               = 8;
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::SCHEDULE_REMOVE_GROUP));
    command_class_schedule_core::set_requested_args args {group, &generator};

    // Schedule Remove requires a schedule ID and block ID; confirm the partial request is not emitted.
    generator.initialize_frame(SCHEDULE_REMOVE_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_remove_requested(args, data, &length));
    desired(group, schedule_remove_group_attributes_t::schedule_id, SCHEDULE_ID_FOR_REMOVE_TEST);
    generator.initialize_frame(SCHEDULE_REMOVE_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_remove_requested(args, data, &length));
    desired(group, schedule_remove_group_attributes_t::schedule_id_block, SCHEDULE_ID_BLOCK_2);

    // The complete frame is CC ID, Remove command ID, schedule ID, and schedule ID block.
    generator.initialize_frame(SCHEDULE_REMOVE_COMMAND_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_remove_requested(args, data, &length));
    const std::vector<uint8_t> expected_frame {SCHEDULE_COMMAND_CLASS_ID, SCHEDULE_REMOVE_COMMAND_ID, SCHEDULE_ID_FOR_REMOVE_TEST, SCHEDULE_ID_BLOCK_2};
    EXPECT_EQ(expected_frame, std::vector<uint8_t>(data, data + length));
}

// CC:0053.02.07.11.001
// Verifies Schedule State Set waits for its schedule ID, state value, and block selector.
TEST_F(ScheduleCommandClassTest, ScheduleStateSetV2EncodesScheduleStateAndBlock)
{
    constexpr uint8_t SCHEDULE_ID_FOR_STATE_TEST = 7;
    constexpr uint8_t SCHEDULE_STATE_ENABLED     = 1;
    constexpr size_t FRAME_CAPACITY              = 8;
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::SCHEDULE_STATE_SET_GROUP));
    command_class_schedule_core::set_requested_args args {group, &generator};

    // Add one required field at a time to check each incomplete State Set remains pending.
    generator.initialize_frame(SCHEDULE_STATE_SET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_state_set_requested(args, data, &length));
    desired(group, schedule_state_set_group_attributes_t::schedule_id, SCHEDULE_ID_FOR_STATE_TEST);
    generator.initialize_frame(SCHEDULE_STATE_SET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_state_set_requested(args, data, &length));
    desired(group, schedule_state_set_group_attributes_t::schedule_state, SCHEDULE_STATE_ENABLED);
    generator.initialize_frame(SCHEDULE_STATE_SET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_state_set_requested(args, data, &length));
    desired(group, schedule_state_set_group_attributes_t::schedule_id_block, SCHEDULE_ID_BLOCK_2);

    // Once complete, State Set encodes the schedule ID, enabled state, and block ID after its command byte.
    generator.initialize_frame(SCHEDULE_STATE_SET_COMMAND_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_state_set_requested(args, data, &length));
    const std::vector<uint8_t> expected_frame {SCHEDULE_COMMAND_CLASS_ID, SCHEDULE_STATE_SET_COMMAND_ID, SCHEDULE_ID_FOR_STATE_TEST, SCHEDULE_STATE_ENABLED, SCHEDULE_ID_BLOCK_2};
    EXPECT_EQ(expected_frame, std::vector<uint8_t>(data, data + length));
}

// CC:0053.02.08.11.001
// Verifies the v2 State Get request requires and encodes the selected schedule block.
TEST_F(ScheduleCommandClassTest, ScheduleStateGetV2EncodesBlockSelector)
{
    constexpr size_t FRAME_CAPACITY = 8;
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_state_get_group_attributes_t::SCHEDULE_STATE_GET_GROUP));
    command_class_schedule_core::get_requested_args args {group, &generator};

    // No block selector means there is not enough information to build a State Get.
    generator.initialize_frame(SCHEDULE_STATE_GET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_state_get_requested(args, data, &length));
    desired(group, schedule_state_get_group_attributes_t::schedule_id_block, SCHEDULE_ID_BLOCK_3);

    // The completed v2 request contains only the command header and selected block ID.
    generator.initialize_frame(SCHEDULE_STATE_GET_COMMAND_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_state_get_requested(args, data, &length));
    const std::vector<uint8_t> expected_frame {SCHEDULE_COMMAND_CLASS_ID, SCHEDULE_STATE_GET_COMMAND_ID, SCHEDULE_ID_BLOCK_3};
    EXPECT_EQ(expected_frame, std::vector<uint8_t>(data, data + length));
}

// CC:0053.04.0A.11.001
// Verifies the v4 Supported Commands Get request requires and encodes the selected block.
TEST_F(ScheduleCommandClassTest, SupportedCommandsGetV4EncodesBlockSelector)
{
    constexpr size_t FRAME_CAPACITY = 8;
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = 0;
    zwave_frame_generator generator {SCHEDULE_COMMAND_CLASS_ID};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP));
    command_class_schedule_core::get_requested_args args {group, &generator};

    // V4 requires a block selector for the Supported Commands request.
    generator.initialize_frame(SCHEDULE_SUPPORTED_COMMANDS_GET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_supported_commands_get_requested(args, data, &length));
    desired(group, schedule_supported_commands_get_group_attributes_t::schedule_id_block, SCHEDULE_ID_BLOCK_3);

    // The encoded request carries the block ID after the CC and command identifiers.
    generator.initialize_frame(SCHEDULE_SUPPORTED_COMMANDS_GET_COMMAND_ID, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_commands_get_requested(args, data, &length));
    const std::vector<uint8_t> expected_frame {SCHEDULE_COMMAND_CLASS_ID, SCHEDULE_SUPPORTED_COMMANDS_GET_COMMAND_ID, SCHEDULE_ID_BLOCK_3};
    EXPECT_EQ(expected_frame, std::vector<uint8_t>(data, data + length));
}

// CC:0053.02.01.11.001, CC:0053.02.03.11.001, CC:0053.02.04.11.001
// Verifies MQTT parses required Schedule fields and stores desired values before starting resolutions.
TEST_F(ScheduleCommandClassTest, MqttCommandsCreateGroupsAndRejectMalformedPayloads)
{
    // Commands with missing required properties fail without creating a valid request group.
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_remove_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_set_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_commands_get_command(endpoint, "{}"));

    // Fail each command's parser at each required field, after all preceding fields parsed successfully.
    const auto rejects_missing = [](nlohmann::json complete, const std::vector<std::vector<std::string>> &paths, const auto &handler) {
        for (const auto &path: paths) {
            auto payload = complete;
            auto *parent = &payload;
            for (size_t i = 0; i + 1 < path.size(); ++i) {
                parent = &(*parent)[path[i]];
            }
            parent->erase(path.back());
            EXPECT_NE(SL_STATUS_OK, handler(payload.dump())) << path.back();
        }
    };
    rejects_missing(nlohmann::json::parse(R"({"schedule_id_block":2})"), {{"schedule_id_block"}}, [this](const auto &p) { return command_class_schedule_mqtt::mqtt_on_schedule_supported_get_command(endpoint, p); });
    rejects_missing(nlohmann::json::parse(R"({"schedule_id":7,"schedule_id_block":2,"properties1":{"aid_ro_ctl":1}})"), {{"schedule_id"}, {"schedule_id_block"}, {"properties1", "aid_ro_ctl"}}, [this](const auto &p) {
        return command_class_schedule_mqtt::mqtt_on_command_schedule_get_command(endpoint, p);
    });
    rejects_missing(nlohmann::json::parse(R"({"schedule_id":7,"schedule_id_block":2})"), {{"schedule_id"}, {"schedule_id_block"}}, [this](const auto &p) { return command_class_schedule_mqtt::mqtt_on_schedule_remove_command(endpoint, p); });
    rejects_missing(nlohmann::json::parse(R"({"schedule_id":7,"schedule_state":1,"schedule_id_block":2})"), {{"schedule_id"}, {"schedule_state"}, {"schedule_id_block"}}, [this](const auto &p) { return command_class_schedule_mqtt::mqtt_on_schedule_state_set_command(endpoint, p); });
    rejects_missing(nlohmann::json::parse(R"({"schedule_id_block":2})"), {{"schedule_id_block"}}, [this](const auto &p) { return command_class_schedule_mqtt::mqtt_on_schedule_state_get_command(endpoint, p); });
    rejects_missing(nlohmann::json::parse(R"({"schedule_id_block":2})"), {{"schedule_id_block"}}, [this](const auto &p) { return command_class_schedule_mqtt::mqtt_on_schedule_supported_commands_get_command(endpoint, p); });

    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_get_command(endpoint, R"({"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_get_command(endpoint, R"({"schedule_id":7,"schedule_id_block":2,"properties1":{"aid_ro_ctl":1}})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_remove_command(endpoint, R"({"schedule_id":7,"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_set_command(endpoint, R"({"schedule_id":7,"schedule_state":1,"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_get_command(endpoint, R"({"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_commands_get_command(endpoint, R"({"schedule_id_block":2})"));

    EXPECT_EQ(2, endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::schedule_id_block)).desired<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::SCHEDULE_STATE_SET_GROUP)).child_by_type(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_state)).desired<uint8_t>());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
}

// CC:0053.02.03.11.001, CC:0053.03.03.11.001
// Verifies MQTT Schedule Set parses nested properties and length-prefixed nested command arrays.
TEST_F(ScheduleCommandClassTest, MqttCommandScheduleSetStoresPackedNestedCommand)
{
    // Provide all required fields and two nested commands using the documented MQTT names.
    const std::string payload
      = R"({"schedule_id":3,"schedule_id_block":1,"start_year":26,"duration_byte":90,"reports_to_follow":0,"properties1":{"start_month":4,"recurrence_offset":2},"properties2":{"start_day_of_month":9,"recurrence_mode":1},"properties3":{"start_weekday":5},"properties4":{"start_hour":10,"duration_type":2},"properties5":{"start_minute":12,"relative":1},"vg1":[{"cmd_byte":[32,1]},{"cmd_byte":[37,3,4]}]})";
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint, payload));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::COMMAND_SCHEDULE_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(3, group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::schedule_id)).desired<uint8_t>());
    EXPECT_EQ(4, group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_month)).desired<uint8_t>());
    EXPECT_EQ(2, group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::number_of_cmd_to_follow)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {2, 32, 1, 3, 37, 3, 4}), group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::vg1)).desired<std::vector<uint8_t>>());
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint, "{}"));
}

// Verifies nested Schedule Set parsing rejects wrong JSON shapes and incomplete grouped fields.
TEST_F(ScheduleCommandClassTest, MqttScheduleSetRejectsMalformedNestedFields)
{
    const auto complete = nlohmann::json::parse(
      R"({"schedule_id":3,"schedule_id_block":1,"start_year":26,"duration_byte":90,"reports_to_follow":0,"properties1":{"start_month":4,"recurrence_offset":2},"properties2":{"start_day_of_month":9,"recurrence_mode":1},"properties3":{"start_weekday":5},"properties4":{"start_hour":10,"duration_type":2},"properties5":{"start_minute":12,"relative":1},"vg1":[]})");
    const std::vector<std::vector<std::string>> required_paths {
      {"schedule_id"},
      {"schedule_id_block"},
      {"start_year"},
      {"duration_byte"},
      {"reports_to_follow"},
      {"properties1", "start_month"},
      {"properties1", "recurrence_offset"},
      {"properties2", "start_day_of_month"},
      {"properties2", "recurrence_mode"},
      {"properties3", "start_weekday"},
      {"properties4", "start_hour"},
      {"properties4", "duration_type"},
      {"properties5", "start_minute"},
      {"properties5", "relative"},
      {"vg1"},
    };
    // Remove one required key at a time so every successful parser prefix and missing-field return is exercised.
    for (const auto &path: required_paths) {
        auto payload = complete;
        auto *parent = &payload;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            parent = &(*parent)[path[i]];
        }
        parent->erase(path.back());
        EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint, payload.dump())) << path.back();
    }

    // A present field with the wrong JSON type must fail at the same parse step as a missing field.
    for (const auto &path: required_paths) {
        auto payload = complete;
        auto *parent = &payload;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            parent = &(*parent)[path[i]];
        }
        (*parent)[path.back()] = "invalid";
        EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint, payload.dump())) << path.back();
    }

    const std::vector<std::string> invalid_payloads {
      "not-json",
      R"({"schedule_id":"3"})",
      R"({"properties1":[]})",
      R"({"properties1":{"start_month":"April"}})",
      R"({"properties2":null})",
      R"({"properties3":{"start_weekday":"Monday"}})",
      R"({"properties4":{"start_hour":10,"duration_type":"daily"}})",
      R"({"properties5":{"start_minute":30,"relative":"yes"}})",
      R"({"vg1":[{}]})",
      R"({"vg1":[{"cmd_byte":"not-an-array"}]})",
    };
    for (const auto &payload: invalid_payloads) {
        EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint, payload)) << payload;
    }

    // The array parser must discard the valid prefix if a later variable group is malformed.
    EXPECT_NE(
      SL_STATUS_OK,
      command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(
        endpoint,
        R"({"schedule_id":3,"schedule_id_block":1,"start_year":26,"duration_byte":90,"reports_to_follow":0,"properties1":{"start_month":4,"recurrence_offset":2},"properties2":{"start_day_of_month":9,"recurrence_mode":1},"properties3":{"start_weekday":5},"properties4":{"start_hour":10,"duration_type":2},"properties5":{"start_minute":12,"relative":1},"vg1":[{"cmd_byte":[1,2]},{"cmd_byte":"invalid"}]})"));
}
