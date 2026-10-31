/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************/
#include <gtest/gtest.h>
#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
#include "command_class_association_grp_info.hpp"
#include "command_class_association_grp_info_attributes.hpp"
#include "command_class_association_grp_info_constants.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_generated_types.hpp"
#include "command_class_association_grp_info_mqtt.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_generator_standalone.hpp"
#include "zwave_frame_parser.hpp"

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_association_grp_info_types;

namespace
{
    constexpr uint8_t AGI_CC                             = 0x59;
    constexpr uint16_t NODE_ID                           = 2;
    constexpr uint8_t ENDPOINT_ID                        = 0;
    constexpr uint8_t LIFELINE_GROUP                     = 1;
    constexpr uint8_t GROUP_TWO                          = 2;
    constexpr uint8_t GROUP_THREE                        = 3;
    constexpr uint8_t FLAG_SET                           = 1;
    constexpr uint8_t FRAME_CAPACITY                     = 32;
    constexpr uint8_t AGI_DESTINATION_A                  = 7;
    constexpr uint8_t AGI_DESTINATION_B                  = 9;
    constexpr uint8_t AGI_DESTINATION_C                  = 11;
    constexpr uint8_t TEST_GROUP_CAPACITY                = 4;
    constexpr uint8_t HOME_ID_BYTE_COUNT                 = 4;
    constexpr uint8_t HOME_ID_BYTE_VALUE                 = 0;
    constexpr uint16_t INITIAL_FRAME_LENGTH              = 0;
    constexpr uint8_t NO_FLAGS                           = 0;
    constexpr uint8_t NO_COMMAND_PAIRS                   = 0;
    constexpr uint8_t GROUP_NAME_LENGTH_LIFELINE         = 8;
    constexpr uint8_t GROUP_NAME_LENGTH_TEST_NAME        = 3;
    constexpr uint16_t GENERAL_GROUP_PROFILE             = 2;
    constexpr uint16_t TEST_EVENT_CODE                   = 0x1234;
    constexpr uint8_t ONE_COMMAND_PAIR_LENGTH            = 2;
    constexpr uint8_t GROUP_RECORD_ONE_OFFSET            = 3;
    constexpr uint8_t GROUP_RECORD_TWO_OFFSET            = 10;
    constexpr uint8_t GROUP_RECORD_THREE_OFFSET          = 17;
    constexpr uint8_t AGI_GROUPING_IDENTIFIER_OFFSET     = 2;
    constexpr uint8_t AGI_PROPERTIES1_OFFSET             = 2;
    constexpr uint8_t ONE_GROUP_COUNT                    = 1;
    constexpr uint8_t TWO_GROUP_COUNT                    = 2;
    constexpr uint8_t INFO_REPORT_SINGLE_GROUP_SIZE      = 10;
    constexpr uint8_t LIST_MODE_AND_THREE_GROUPS         = 0x83;
    constexpr uint8_t LIST_MODE_AND_ONE_GROUP            = 0x81;
    constexpr uint8_t LIST_MODE_AND_REFRESH_CACHE        = 0xC0;
    constexpr uint8_t ALLOW_CACHE_FLAG                   = 0x80;
    constexpr uint8_t ENDPOINT_PROPERTIES_ENDPOINT_ONE   = 0x81;
    constexpr uint8_t ENDPOINT_PROPERTIES_ENDPOINT_TWO   = 0x02;
    constexpr uint8_t ENDPOINT_PROPERTIES_ENDPOINT_THREE = 0x82;
    constexpr uint8_t UNKNOWN_GROUP_ID                   = 9;
    constexpr uint8_t TEST_SWITCH_MULTILEVEL_CC          = 0x20;
    constexpr uint8_t COMMAND_SWITCH_MULTILEVEL_SET      = 0x01;
    constexpr auto OBSERVER_TIMEOUT                      = std::chrono::seconds(1);

    struct MembershipObserver {
            std::mutex mutex;
            std::condition_variable condition;
            bool received = false;
            bool found    = false;
    };

    MembershipObserver &membership_observer()
    {
        static MembershipObserver observer;
        return observer;
    }

    attribute_store::attribute seed_agi_group(attribute_store::attribute home, uint8_t id, uint8_t max_nodes)
    {
        auto group = home.emplace_node(AGI_ZPC_GROUP, id);
        group.emplace_node(max_nodes_supported).set_reported<uint8_t>(max_nodes);
        return group;
    }
}  // namespace

class TestableAssociationGroupInfoMqtt : public command_class_association_grp_info_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class AssociationGroupInfoTest : public ::testing::Test
{
    protected:
        static command_class_association_grp_info &cc()
        {
            static command_class_association_grp_info instance;
            return instance;
        }

        static TestableAssociationGroupInfoMqtt &mqtt()
        {
            static TestableAssociationGroupInfoMqtt instance;
            return instance;
        }

        static void connect_observer()
        {
            static const bool connected = [] {
                component_connector registry;
                auto *observer = &membership_observer();
                registry.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_check_command_list_result_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST_RESULT, [observer](const auto &result) {
                    {
                        std::lock_guard<std::mutex> lock(observer->mutex);
                        observer->found    = result.found;
                        observer->received = true;
                    }
                    observer->condition.notify_one();
                    return SL_STATUS_OK;
                });
                return true;
            }();
            (void)connected;
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            connector.start();
            auto root = attribute_store::attribute::root();
            home      = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(HOME_ID_BYTE_COUNT, HOME_ID_BYTE_VALUE));
            endpoint  = home.emplace_node(ATTRIBUTE_NODE_ID, NODE_ID).emplace_node(ATTRIBUTE_ENDPOINT_ID, ENDPOINT_ID);
            (void)cc();
            (void)mqtt();
            connect_observer();
            seed_agi_group(home, LIFELINE_GROUP, command_class_association_grp_info_constants::MAX_NODES_PER_GROUP);
        }

        void TearDown() override
        {
            connector.stop();
            attribute_store_teardown();
        }

        bool check_membership(uint8_t command_class_id, uint8_t command_id)
        {
            auto &observer = membership_observer();
            {
                std::lock_guard<std::mutex> lock(observer.mutex);
                observer.received = false;
            }
            const component_connector_agi_check_command_list_payload_t request {endpoint, command_class_id, command_id};
            EXPECT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST), request).get());
            std::unique_lock<std::mutex> lock(observer.mutex);
            EXPECT_TRUE(observer.condition.wait_for(lock, OBSERVER_TIMEOUT, [&observer] { return observer.received; }));
            return observer.found;
        }

        attribute_store::attribute home;
        attribute_store::attribute endpoint;
        component_connector connector;
};

// Verifies that Connector name get stores requested group.
// Requirement: CC:0059.01.01.11.001.
TEST_F(AssociationGroupInfoTest, ConnectorNameGetStoresRequestedGroup)
{
    const component_connector_agi_group_name_get_payload_t request {endpoint, GROUP_TWO};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_NAME_GET), request).get());
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::ASSOCIATION_GROUP_NAME_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
}

// Verifies that Connector info get stores group and flags.
// Requirement: CC:0059.01.03.11.001.
TEST_F(AssociationGroupInfoTest, ConnectorInfoGetStoresGroupAndFlags)
{
    const component_connector_agi_group_info_get_payload_t request {endpoint, GROUP_TWO, FLAG_SET, FLAG_SET};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_INFO_GET), request).get());
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::ASSOCIATION_GROUP_INFO_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
    EXPECT_EQ(FLAG_SET, group.child_by_type(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::list_mode)).desired<uint8_t>());
    EXPECT_EQ(FLAG_SET, group.child_by_type(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::refresh_cache)).desired<uint8_t>());
}

// Verifies that Connector command list get stores group and cache flag.
// Requirement: CC:0059.01.05.11.001.
TEST_F(AssociationGroupInfoTest, ConnectorCommandListGetStoresGroupAndCacheFlag)
{
    const component_connector_agi_group_command_list_get_payload_t request {endpoint, GROUP_TWO, FLAG_SET};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_GET), request).get());
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
    EXPECT_EQ(FLAG_SET, group.child_by_type(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::allow_cache)).desired<uint8_t>());
}

// Verifies that Name report version one stores utf8 name.
// Requirement: CC:0059.01.02.11.004.
TEST_F(AssociationGroupInfoTest, NameReportVersionOneStoresUtf8Name)
{
    // The ASCII digit is part of the stored test name "Group 2".
    const std::vector<uint8_t> name {'G', 'r', 'o', 'u', 'p', ' ', '2'};
    // Fixture fields: grouping_identifier selects the association group; length_of_name gives the UTF-8 name byte count; name carries the UTF-8 name bytes.
    const command_class_association_grp_info_attribute_map_t report {{"grouping_identifier", GROUP_TWO}, {"length_of_name", static_cast<uint8_t>(name.size())}, {"name", name}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_name_report_received_store(endpoint, report));
    auto record = endpoint.child_by_type_and_value(AGI_NODE_GROUP, GROUP_TWO);
    ASSERT_TRUE(record.is_valid());
    EXPECT_EQ(name, record.child_by_type(group_name).reported<std::vector<uint8_t>>());
}

// Verifies that Info report version one stores group count and flags.
// Requirement: CC:0059.01.04.11.001.
TEST_F(AssociationGroupInfoTest, InfoReportVersionOneStoresGroupCountAndFlags)
{
    constexpr uint8_t count = 2;
    // Fixture fields: group_count gives the number of groups; dynamic_info marks dynamic group data; list_mode identifies single-group or all-groups mode.
    const command_class_association_grp_info_attribute_map_t report {{"group_count", count}, {"dynamic_info", uint8_t {NO_FLAGS}}, {"list_mode", uint8_t {NO_FLAGS}}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_info_report_received_store(endpoint, report));
    auto record = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::ASSOCIATION_GROUP_INFO_REPORT_GROUP));
    ASSERT_TRUE(record.is_valid());
    EXPECT_EQ(count, record.child_by_type(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::group_count)).reported<uint8_t>());
}

// Verifies that Info report version three stores profiles and event codes.
// Requirement: CC:0059.01.04.11.001.
TEST_F(AssociationGroupInfoTest, InfoReportVersionThreeStoresProfilesAndEventCodes)
{
    const association_group_info_report_vg1_t entries {{GROUP_TWO, NO_FLAGS, NO_FLAGS, ONE_GROUP_COUNT, NO_FLAGS, TEST_EVENT_CODE}};
    // Fixture fields: group_count gives the number of groups; dynamic_info marks dynamic group data; list_mode identifies single-group or all-groups mode; vg1 carries the version 3 group profile entries.
    const command_class_association_grp_info_attribute_map_t report {{"group_count", uint8_t {ONE_GROUP_COUNT}}, {"dynamic_info", uint8_t {NO_FLAGS}}, {"list_mode", uint8_t {NO_FLAGS}}, {"vg1", entries}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_info_report_received_store(endpoint, report));
    auto record = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::ASSOCIATION_GROUP_INFO_REPORT_GROUP));
    ASSERT_TRUE(record.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {GROUP_TWO, NO_FLAGS, NO_FLAGS, ONE_GROUP_COUNT, NO_FLAGS, static_cast<uint8_t>(TEST_EVENT_CODE >> 8), static_cast<uint8_t>(TEST_EVENT_CODE)}),
              record.child_by_type(static_cast<attribute_store_type_t>(association_group_info_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>());
    EXPECT_EQ(ONE_GROUP_COUNT, endpoint.child_by_type_and_value(AGI_NODE_GROUP, GROUP_TWO).child_by_type(group_profile).reported<uint16_t>());
}

// Verifies that Command list report stores command pairs.
// Requirement: CC:0059.01.06.11.001.
TEST_F(AssociationGroupInfoTest, CommandListReportStoresCommandPairs)
{
    const std::vector<uint8_t> commands {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET};
    // Fixture fields: grouping_identifier selects the association group; list_length gives the command-pair byte count; command carries the advertised command-class/command pairs.
    const command_class_association_grp_info_attribute_map_t report {{"grouping_identifier", GROUP_TWO}, {"list_length", static_cast<uint8_t>(commands.size())}, {"command", commands}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_command_list_report_received_store(endpoint, report));
    auto record = endpoint.child_by_type_and_value(AGI_NODE_GROUP, GROUP_TWO);
    ASSERT_TRUE(record.is_valid());
    EXPECT_EQ(commands, record.child_by_type(group_command_list).reported<std::vector<uint8_t>>());
}

// Verifies that Name get version one encodes group identifier.
// Requirement: CC:0059.01.01.11.001.
TEST_F(AssociationGroupInfoTest, NameGetVersionOneEncodesGroupIdentifier)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_NAME_GET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::ASSOCIATION_GROUP_NAME_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_name_get_requested(args, bytes, &length));
    // V1 Name Get frame is CC, command, and grouping identifier.
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, command, GROUP_TWO}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Name get waits until group identifier is known.
// Requirement: CC:0059.01.01.11.001.
TEST_F(AssociationGroupInfoTest, NameGetWaitsUntilGroupIdentifierIsKnown)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_NAME_GET);
    auto group                = endpoint.add_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::ASSOCIATION_GROUP_NAME_GET_GROUP));
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_IS_WAITING, cc().command_class_association_grp_info_core::on_association_group_name_get_requested(args, bytes, &length));
}

// Verifies that Info get version three encodes flags and group identifier.
// Requirement: CC:0059.01.03.11.001.
TEST_F(AssociationGroupInfoTest, InfoGetVersionThreeEncodesFlagsAndGroupIdentifier)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_INFO_GET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::ASSOCIATION_GROUP_INFO_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::list_mode)).set_desired<uint8_t>(FLAG_SET);
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::refresh_cache)).set_desired<uint8_t>(FLAG_SET);
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_info_get_requested(args, bytes, &length));
    // V3 properties byte sets List Mode (0x40) and Refresh Cache (ALLOW_CACHE_FLAG); the final byte selects group 2.
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, command, LIST_MODE_AND_REFRESH_CACHE, GROUP_TWO}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Info get defaults optional flags to zero.
// Requirement: CC:0059.01.03.11.001.
TEST_F(AssociationGroupInfoTest, InfoGetDefaultsOptionalFlagsToZero)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_INFO_GET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::ASSOCIATION_GROUP_INFO_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_info_get_requested(args, bytes, &length));
    // Both optional properties are clear, followed by the requested grouping identifier.
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, command, NO_FLAGS, GROUP_TWO}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Info get waits for group identifier.
// Requirement: CC:0059.01.03.11.001.
TEST_F(AssociationGroupInfoTest, InfoGetWaitsForGroupIdentifier)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_INFO_GET);
    auto group                = endpoint.add_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::ASSOCIATION_GROUP_INFO_GET_GROUP));
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_IS_WAITING, cc().command_class_association_grp_info_core::on_association_group_info_get_requested(args, bytes, &length));
}

// Verifies that Command list get encodes allow cache and group.
// Requirement: CC:0059.01.05.11.001.
TEST_F(AssociationGroupInfoTest, CommandListGetEncodesAllowCacheAndGroup)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_COMMAND_LIST_GET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::allow_cache)).set_desired<uint8_t>(FLAG_SET);
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_command_list_get_requested(args, bytes, &length));
    // Properties bit 7 (0x80) requests cache use; the last byte selects GROUP_TWO.
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, command, ALLOW_CACHE_FLAG, GROUP_TWO}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Command List Get clears Allow Cache when the optional field is absent.
// Requirement: CC:0059.01.05.11.001.
TEST_F(AssociationGroupInfoTest, CommandListGetDefaultsAllowCache)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_COMMAND_LIST_GET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_command_list_get_requested(args, bytes, &length));
    EXPECT_EQ(NO_FLAGS, bytes[AGI_PROPERTIES1_OFFSET]);
}

// Verifies that Command List Get waits until its grouping identifier is available.
// Requirement: CC:0059.01.05.11.001.
TEST_F(AssociationGroupInfoTest, CommandListGetWaitsForGroupIdentifier)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_COMMAND_LIST_GET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_GET_GROUP));
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {AGI_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_grp_info_core::get_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_IS_WAITING, cc().command_class_association_grp_info_core::on_association_group_command_list_get_requested(args, bytes, &length));
}

// Verifies that Mqtt name get requires and stores group identifier.
// Requirement: CC:0059.01.01.11.001.
TEST_F(AssociationGroupInfoTest, MqttNameGetRequiresAndStoresGroupIdentifier)
{
    mqtt().mqtt_command_handler(endpoint, "AssociationGroupNameGet", fmt::format(R"({{"grouping_identifier":"{}"}})", GROUP_TWO));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::ASSOCIATION_GROUP_NAME_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
}

// Verifies that Mqtt info get defaults optional flags.
// Requirement: CC:0059.01.03.11.001.
TEST_F(AssociationGroupInfoTest, MqttInfoGetDefaultsOptionalFlags)
{
    mqtt().mqtt_command_handler(endpoint, "AssociationGroupInfoGet", fmt::format(R"({{"grouping_identifier":"{}"}})", GROUP_TWO));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::ASSOCIATION_GROUP_INFO_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(NO_FLAGS, group.child_by_type(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::list_mode)).desired<uint8_t>());
    EXPECT_EQ(NO_FLAGS, group.child_by_type(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::refresh_cache)).desired<uint8_t>());
}

// Verifies that Mqtt command list get defaults allow cache.
// Requirement: CC:0059.01.05.11.001.
TEST_F(AssociationGroupInfoTest, MqttCommandListGetDefaultsAllowCache)
{
    mqtt().mqtt_command_handler(endpoint, "AssociationGroupCommandListGet", fmt::format(R"({{"grouping_identifier":"{}"}})", GROUP_TWO));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(NO_FLAGS, group.child_by_type(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::allow_cache)).desired<uint8_t>());
}

// Verifies that Lifeline add merges unique node and endpoint destinations.
TEST_F(AssociationGroupInfoTest, LifelineAddMergesUniqueNodeAndEndpointDestinations)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    lifeline.emplace_node(node_ids).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_A});
    lifeline.emplace_node(endpoint_associations).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE});
    const component_connector_agi_lifeline_update_payload_t request {{AGI_DESTINATION_A, AGI_DESTINATION_B}, {{AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE}, {AGI_DESTINATION_B, ENDPOINT_PROPERTIES_ENDPOINT_TWO}}};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE), request).get());
    EXPECT_EQ((std::vector<uint8_t> {AGI_DESTINATION_A, AGI_DESTINATION_B}), lifeline.child_by_type(node_ids).reported<std::vector<uint8_t>>());
    EXPECT_EQ((std::vector<uint8_t> {AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE, AGI_DESTINATION_B, ENDPOINT_PROPERTIES_ENDPOINT_TWO}), lifeline.child_by_type(endpoint_associations).reported<std::vector<uint8_t>>());
}

// Verifies that Lifeline remove deletes selected destinations.
TEST_F(AssociationGroupInfoTest, LifelineRemoveDeletesSelectedDestinations)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    lifeline.emplace_node(node_ids).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_A, AGI_DESTINATION_B});
    lifeline.emplace_node(endpoint_associations).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE, AGI_DESTINATION_B, ENDPOINT_PROPERTIES_ENDPOINT_TWO});
    const component_connector_agi_lifeline_update_payload_t request {{AGI_DESTINATION_A}, {{AGI_DESTINATION_B, ENDPOINT_PROPERTIES_ENDPOINT_TWO}}};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE), request).get());
    EXPECT_EQ((std::vector<uint8_t> {AGI_DESTINATION_B}), lifeline.child_by_type(node_ids).reported<std::vector<uint8_t>>());
    EXPECT_EQ((std::vector<uint8_t> {AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE}), lifeline.child_by_type(endpoint_associations).reported<std::vector<uint8_t>>());
}

// Verifies that adding the same Lifeline command twice advertises one pair.
TEST_F(AssociationGroupInfoTest, LifelineCommandRegistryDeduplicatesPair)
{
    const component_connector_agi_lifeline_command_payload_t command {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET};
    const auto add_event = static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND);
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(add_event, command).get());
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(add_event, command).get());
    const component_connector_agi_empty_payload_t request;
    auto [status, list] = connector.fire_event_async<component_connector_agi_empty_payload_t, component_connector_agi_lifeline_command_list_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_COMMAND_LIST), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    ASSERT_EQ((std::vector<uint8_t> {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET}), list.commands);
}

// Verifies that removing a registered Lifeline command removes the pair from its report.
TEST_F(AssociationGroupInfoTest, LifelineCommandRegistryRemovesPair)
{
    const component_connector_agi_lifeline_command_payload_t command {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET};
    const auto add_event    = static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND);
    const auto remove_event = static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_COMMAND);
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(add_event, command).get());
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(remove_event, command).get());
    const component_connector_agi_empty_payload_t request;
    auto [status, list] = connector.fire_event_async<component_connector_agi_empty_payload_t, component_connector_agi_lifeline_command_list_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_COMMAND_LIST), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_TRUE(list.commands.empty());
}

// Verifies that membership lookup finds a command pair reported by a group.
TEST_F(AssociationGroupInfoTest, GroupCommandListMembershipFindsPair)
{
    // Fixture fields: grouping_identifier selects the association group; list_length gives the command-pair byte count; command carries the advertised command-class/command pairs.
    const command_class_association_grp_info_attribute_map_t report {{"grouping_identifier", GROUP_TWO}, {"list_length", uint8_t {ONE_COMMAND_PAIR_LENGTH}}, {"command", std::vector<uint8_t> {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET}}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_command_list_report_received_store(endpoint, report));
    EXPECT_TRUE(check_membership(TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET));
}

// Verifies that Supported groupings query counts zpc groups.
TEST_F(AssociationGroupInfoTest, SupportedGroupingsQueryCountsZpcGroups)
{
    seed_agi_group(home, GROUP_TWO, TEST_GROUP_CAPACITY);
    const component_connector_agi_empty_payload_t request;
    auto [status, count] = connector.fire_event_async<component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(TWO_GROUP_COUNT, count);
}

// Verifies that the Group Capacity query returns the reported capacity.
TEST_F(AssociationGroupInfoTest, GroupCapacityQueryReturnsReportedLimit)
{
    seed_agi_group(home, GROUP_TWO, TEST_GROUP_CAPACITY);
    const component_connector_agi_group_max_nodes_query_t group_two {GROUP_TWO};
    auto [known_status, known_limit] = connector.fire_event_async<component_connector_agi_group_max_nodes_query_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES), group_two).get();
    ASSERT_EQ(SL_STATUS_OK, known_status);
    EXPECT_EQ(TEST_GROUP_CAPACITY, known_limit);
}

// Verifies that an unknown group has zero reported capacity.
TEST_F(AssociationGroupInfoTest, GroupCapacityQueryReturnsZeroForUnknownGroup)
{
    const component_connector_agi_group_max_nodes_query_t unknown {GROUP_THREE};
    auto [status, capacity] = connector.fire_event_async<component_connector_agi_group_max_nodes_query_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES), unknown).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_FLAGS, capacity);
}

// Verifies that Group destinations query returns reported values.
TEST_F(AssociationGroupInfoTest, GroupDestinationsQueryReturnsReportedValues)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    lifeline.emplace_node(node_ids).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_A, AGI_DESTINATION_B});
    lifeline.emplace_node(endpoint_associations).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_C, ENDPOINT_PROPERTIES_ENDPOINT_THREE});
    const component_connector_agi_group_destinations_query_t request {LIFELINE_GROUP};
    auto [status, values] = connector.fire_event_async<component_connector_agi_group_destinations_query_t, component_connector_agi_group_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ((std::vector<uint8_t> {AGI_DESTINATION_A, AGI_DESTINATION_B}), values.node_ids);
    EXPECT_EQ((std::vector<std::pair<uint8_t, uint8_t>> {{AGI_DESTINATION_C, ENDPOINT_PROPERTIES_ENDPOINT_THREE}}), values.endpoint_associations);
}

// Verifies that Lifeline query returns reported values.
TEST_F(AssociationGroupInfoTest, LifelineQueryReturnsReportedValues)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    lifeline.emplace_node(node_ids).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_A});
    const component_connector_agi_empty_payload_t request;
    auto [status, values] = connector.fire_event_async<component_connector_agi_empty_payload_t, component_connector_agi_lifeline_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ((std::vector<uint8_t> {AGI_DESTINATION_A}), values.node_ids);
}

// Verifies that Lifeline query returns reported endpoint associations.
TEST_F(AssociationGroupInfoTest, LifelineQueryReturnsReportedEndpointAssociations)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    lifeline.emplace_node(endpoint_associations).set_reported<std::vector<uint8_t>>({AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE});
    const component_connector_agi_empty_payload_t request;
    auto [status, values] = connector.fire_event_async<component_connector_agi_empty_payload_t, component_connector_agi_lifeline_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ((std::vector<std::pair<uint8_t, uint8_t>> {{AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE}}), values.endpoint_associations);
}

// Verifies that Name report version three uses lifeline name.
// Requirement: CC:0059.01.02.11.001.
TEST_F(AssociationGroupInfoTest, NameReportVersionThreeUsesLifelineName)
{
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_NAME_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_name_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, report_command, LIFELINE_GROUP, GROUP_NAME_LENGTH_LIFELINE, 'L', 'i', 'f', 'e', 'l', 'i', 'n', 'e'}), bytes);
}

// Verifies that Name get normalizes unknown group to lifeline.
// Requirement: CC:0059.01.01.11.001.
TEST_F(AssociationGroupInfoTest, NameGetNormalizesUnknownGroupToLifeline)
{
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_NAME_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", uint8_t {UNKNOWN_GROUP_ID}}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_name_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ(LIFELINE_GROUP, bytes[AGI_GROUPING_IDENTIFIER_OFFSET]);
}

// Verifies that Info get version three reports lifeline profile.
// Requirement: CC:0059.01.03.11.001.
TEST_F(AssociationGroupInfoTest, InfoGetVersionThreeReportsLifelineProfile)
{
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_INFO_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group; list_mode identifies single-group or all-groups mode.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"list_mode", uint8_t {NO_FLAGS}}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_info_get_support_requested(nullptr, request, report, bytes));
    ASSERT_EQ(INFO_REPORT_SINGLE_GROUP_SIZE, bytes.size());
    EXPECT_EQ(ONE_GROUP_COUNT, bytes[AGI_PROPERTIES1_OFFSET] & 0x3F);
    EXPECT_EQ(LIFELINE_GROUP, bytes[GROUP_RECORD_ONE_OFFSET]);
}

// Verifies that Info get list mode sorts groups and sets list bit.
// Requirement: CC:0059.01.03.11.001.
TEST_F(AssociationGroupInfoTest, InfoGetListModeSortsGroupsAndSetsListBit)
{
    seed_agi_group(home, GROUP_THREE, TEST_GROUP_CAPACITY);
    seed_agi_group(home, GROUP_TWO, TEST_GROUP_CAPACITY);
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_INFO_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group; list_mode identifies single-group or all-groups mode.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", GROUP_THREE}, {"list_mode", FLAG_SET}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_info_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ(LIST_MODE_AND_THREE_GROUPS, bytes[AGI_PROPERTIES1_OFFSET]);
    EXPECT_EQ(LIFELINE_GROUP, bytes[GROUP_RECORD_ONE_OFFSET]);
    EXPECT_EQ(GROUP_TWO, bytes[GROUP_RECORD_TWO_OFFSET]);
    EXPECT_EQ(GROUP_THREE, bytes[GROUP_RECORD_THREE_OFFSET]);
}

// Verifies that Command list report version three contains registered lifeline command.
// Requirement: CC:0059.01.06.11.001.
TEST_F(AssociationGroupInfoTest, CommandListReportVersionThreeContainsRegisteredLifelineCommand)
{
    const component_connector_agi_lifeline_command_payload_t command {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND), command).get());
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_COMMAND_LIST_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_command_list_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, report_command, LIFELINE_GROUP, ONE_COMMAND_PAIR_LENGTH, TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET}), bytes);
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_COMMAND), command).get());
}

// Verifies that Name report version one parses group name.
// Requirement: CC:0059.01.02.11.001.
TEST_F(AssociationGroupInfoTest, NameReportVersionOneParsesGroupName)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_NAME_REPORT);
    // V1 Name Report fields: group identifier, name length, then the three name bytes.
    const std::vector<uint8_t> frame {AGI_CC, command, GROUP_TWO, GROUP_NAME_LENGTH_TEST_NAME, 'A', 'G', 'I'};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_association_grp_info_core::report_received_args args {endpoint, parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_name_report_received(args));
    const auto record = endpoint.child_by_type_and_value(AGI_NODE_GROUP, GROUP_TWO);
    ASSERT_TRUE(record.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {'A', 'G', 'I'}), record.child_by_type(group_name).reported<std::vector<uint8_t>>());
}

// Verifies that Info report version one parses single group entry.
// Requirement: CC:0059.01.04.11.001.
TEST_F(AssociationGroupInfoTest, InfoReportVersionOneParsesSingleGroupEntry)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_INFO_REPORT);
    // V1 Info Report fields: one group, identifier, profile, capacity, and name length.
    const std::vector<uint8_t> frame {AGI_CC, command, ONE_GROUP_COUNT, GROUP_TWO, NO_FLAGS, NO_FLAGS, GROUP_TWO, NO_FLAGS, NO_FLAGS, NO_FLAGS};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_association_grp_info_core::report_received_args args {endpoint, parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_info_report_received(args));
    const auto record = endpoint.child_by_type_and_value(AGI_NODE_GROUP, GROUP_TWO);
    ASSERT_TRUE(record.is_valid());
    EXPECT_EQ(GENERAL_GROUP_PROFILE, record.child_by_type(group_profile).reported<uint16_t>());
}

// Verifies that Command list report version one parses command pair.
// Requirement: CC:0059.01.06.11.001.
TEST_F(AssociationGroupInfoTest, CommandListReportVersionOneParsesCommandPair)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_COMMAND_LIST_REPORT);
    const std::vector<uint8_t> frame {AGI_CC, command, GROUP_TWO, ONE_COMMAND_PAIR_LENGTH, TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_association_grp_info_core::report_received_args args {endpoint, parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_group_command_list_report_received(args));
    const auto record = endpoint.child_by_type_and_value(AGI_NODE_GROUP, GROUP_TWO);
    EXPECT_EQ((std::vector<uint8_t> {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET}), record.child_by_type(group_command_list).reported<std::vector<uint8_t>>());
}

// Verifies that Lifeline remove is safe when destination attributes are absent.
TEST_F(AssociationGroupInfoTest, LifelineRemoveIsSafeWhenDestinationAttributesAreAbsent)
{
    const component_connector_agi_lifeline_update_payload_t request {{AGI_DESTINATION_A}, {{AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE}}};
    EXPECT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE), request).get());
}

// Verifies that an empty Lifeline Add request succeeds without changing destinations.
TEST_F(AssociationGroupInfoTest, LifelineAddAcceptsEmptyPayload)
{
    const component_connector_agi_lifeline_update_payload_t empty;
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE), empty).get());
}

// Verifies that an empty Lifeline Remove request succeeds without changing destinations.
TEST_F(AssociationGroupInfoTest, LifelineRemoveAcceptsEmptyPayload)
{
    const component_connector_agi_lifeline_update_payload_t empty;
    EXPECT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE), empty).get());
}

// Verifies that membership lookup ignores a group with no command-list attribute.
TEST_F(AssociationGroupInfoTest, MembershipQuerySkipsMissingList)
{
    endpoint.emplace_node(AGI_NODE_GROUP, GROUP_TWO);
    EXPECT_FALSE(check_membership(TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET));
}

// Verifies that membership lookup ignores a desired-only command list.
TEST_F(AssociationGroupInfoTest, MembershipQuerySkipsDesiredOnlyList)
{
    auto group = endpoint.emplace_node(AGI_NODE_GROUP, GROUP_TWO);
    group.emplace_node(group_command_list).set_desired<std::vector<uint8_t>>({TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET});
    EXPECT_FALSE(check_membership(TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET));
}

// Verifies that Group capacity is zero when maximum is unreported.
TEST_F(AssociationGroupInfoTest, GroupCapacityIsZeroWhenMaximumIsUnreported)
{
    home.emplace_node(AGI_ZPC_GROUP, GROUP_TWO);
    const component_connector_agi_group_max_nodes_query_t request {GROUP_TWO};
    auto [status, maximum] = connector.fire_event_async<component_connector_agi_group_max_nodes_query_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_FLAGS, maximum);
}

// Verifies that Group destinations ignore desired only values.
TEST_F(AssociationGroupInfoTest, GroupDestinationsIgnoreDesiredOnlyValues)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    lifeline.emplace_node(node_ids).set_desired<std::vector<uint8_t>>({AGI_DESTINATION_A});
    const component_connector_agi_group_destinations_query_t request {LIFELINE_GROUP};
    auto [status, values] = connector.fire_event_async<component_connector_agi_group_destinations_query_t, component_connector_agi_group_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_TRUE(values.node_ids.empty());
}

// Verifies that Lifeline destination query fails when lifeline group is missing.
TEST_F(AssociationGroupInfoTest, LifelineDestinationQueryFailsWhenLifelineGroupIsMissing)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    ASSERT_TRUE(lifeline.is_valid());
    lifeline.delete_node();
    const component_connector_agi_empty_payload_t request;
    auto [status, values] = connector.fire_event_async<component_connector_agi_empty_payload_t, component_connector_agi_lifeline_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS), request).get();
    EXPECT_NE(SL_STATUS_OK, status);
    EXPECT_TRUE(values.node_ids.empty());
}

// Verifies that Supported group count query fails when home is missing.
TEST_F(AssociationGroupInfoTest, SupportedGroupCountQueryFailsWhenHomeIsMissing)
{
    home.delete_node();
    const component_connector_agi_empty_payload_t request;
    auto [status, count] = connector.fire_event_async<component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), request).get();
    EXPECT_NE(SL_STATUS_OK, status);
    EXPECT_EQ(NO_FLAGS, count);
}

// Verifies that Group info list mode falls back to lifeline when no home groups exist.
TEST_F(AssociationGroupInfoTest, GroupInfoListModeFallsBackToLifelineWhenNoHomeGroupsExist)
{
    home.delete_node();
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_INFO_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group; list_mode identifies single-group or all-groups mode.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"list_mode", FLAG_SET}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_info_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ(LIST_MODE_AND_ONE_GROUP, bytes[AGI_PROPERTIES1_OFFSET]);
    EXPECT_EQ(LIFELINE_GROUP, bytes[GROUP_RECORD_ONE_OFFSET]);
}

// Verifies that Command list report for non lifeline group is empty.
// Requirement: CC:0059.01.06.11.001.
TEST_F(AssociationGroupInfoTest, CommandListReportForNonLifelineGroupIsEmpty)
{
    seed_agi_group(home, GROUP_TWO, TEST_GROUP_CAPACITY);
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_COMMAND_LIST_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_command_list_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, report_command, GROUP_TWO, NO_COMMAND_PAIRS}), bytes);
}

// Verifies that Lifeline command registry ignores removing unknown pair.
TEST_F(AssociationGroupInfoTest, LifelineCommandRegistryIgnoresRemovingUnknownPair)
{
    const component_connector_agi_lifeline_command_payload_t command {TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET};
    EXPECT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_COMMAND), command).get());
    const component_connector_agi_empty_payload_t request;
    auto [status, list] = connector.fire_event_async<component_connector_agi_empty_payload_t, component_connector_agi_lifeline_command_list_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_COMMAND_LIST), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_TRUE(list.commands.empty());
}

// Verifies that Lifeline add fails when home is missing.
TEST_F(AssociationGroupInfoTest, LifelineAddFailsWhenHomeIsMissing)
{
    home.delete_node();
    const component_connector_agi_lifeline_update_payload_t request {{AGI_DESTINATION_A}, {}};
    EXPECT_NE(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE), request).get());
}

// Verifies that Lifeline remove fails when home is missing.
TEST_F(AssociationGroupInfoTest, LifelineRemoveFailsWhenHomeIsMissing)
{
    home.delete_node();
    const component_connector_agi_lifeline_update_payload_t request {{AGI_DESTINATION_A}, {}};
    EXPECT_NE(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE), request).get());
}

// Verifies that Lifeline remove succeeds when group is missing.
TEST_F(AssociationGroupInfoTest, LifelineRemoveSucceedsWhenGroupIsMissing)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    ASSERT_TRUE(lifeline.is_valid());
    lifeline.delete_node();
    const component_connector_agi_lifeline_update_payload_t request {{AGI_DESTINATION_A}, {{AGI_DESTINATION_A, ENDPOINT_PROPERTIES_ENDPOINT_ONE}}};
    EXPECT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE), request).get());
}

// Verifies that Name report keeps requested group when lifeline is absent.
// Requirement: CC:0059.01.02.11.001.
TEST_F(AssociationGroupInfoTest, NameReportKeepsRequestedGroupWhenLifelineIsAbsent)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    ASSERT_TRUE(lifeline.is_valid());
    lifeline.delete_node();
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_NAME_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_name_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ(GROUP_TWO, bytes[AGI_GROUPING_IDENTIFIER_OFFSET]);
    EXPECT_EQ(NO_FLAGS, bytes[GROUP_RECORD_ONE_OFFSET]);
}

// Verifies that Name report uses empty name for other group.
// Requirement: CC:0059.01.02.11.001.
TEST_F(AssociationGroupInfoTest, NameReportUsesEmptyNameForOtherGroup)
{
    seed_agi_group(home, GROUP_TWO, TEST_GROUP_CAPACITY);
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_grp_info_commands_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ASSOCIATION_GROUP_NAME_REPORT);
    zwave_frame_generator_standalone report;
    report.add_header(AGI_CC, report_command);
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_grp_info_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};
    ASSERT_EQ(SL_STATUS_OK, cc().command_class_association_grp_info_core::on_association_group_name_get_support_requested(nullptr, request, report, bytes));
    EXPECT_EQ((std::vector<uint8_t> {AGI_CC, report_command, GROUP_TWO, NO_COMMAND_PAIRS}), bytes);
}

// Verifies that Group destinations query returns empty for unknown group.
TEST_F(AssociationGroupInfoTest, GroupDestinationsQueryReturnsEmptyForUnknownGroup)
{
    const component_connector_agi_group_destinations_query_t request {GROUP_THREE};
    auto [status, values] = connector.fire_event_async<component_connector_agi_group_destinations_query_t, component_connector_agi_group_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_TRUE(values.node_ids.empty());
    EXPECT_TRUE(values.endpoint_associations.empty());
}

// Verifies that Lifeline destination query returns empty when destination attributes are missing.
TEST_F(AssociationGroupInfoTest, LifelineDestinationQueryReturnsEmptyWhenDestinationAttributesAreMissing)
{
    const component_connector_agi_empty_payload_t request;
    auto [status, result] = connector.fire_event_async<component_connector_agi_empty_payload_t, component_connector_agi_lifeline_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_TRUE(result.node_ids.empty());
    EXPECT_TRUE(result.endpoint_associations.empty());
}

// Verifies that Group destinations query returns empty when destination attributes are missing.
TEST_F(AssociationGroupInfoTest, GroupDestinationsQueryReturnsEmptyWhenDestinationAttributesAreMissing)
{
    seed_agi_group(home, GROUP_TWO, TEST_GROUP_CAPACITY);
    const component_connector_agi_group_destinations_query_t request {GROUP_TWO};
    auto [status, result] = connector.fire_event_async<component_connector_agi_group_destinations_query_t, component_connector_agi_group_destinations_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_TRUE(result.node_ids.empty());
    EXPECT_TRUE(result.endpoint_associations.empty());
}

// Verifies that Network address update preserves reported lifeline capacity.
TEST_F(AssociationGroupInfoTest, NetworkAddressUpdatePreservesReportedLifelineCapacity)
{
    auto lifeline = home.child_by_type_and_value(AGI_ZPC_GROUP, LIFELINE_GROUP);
    auto maximum  = lifeline.child_by_type(max_nodes_supported);
    maximum.set_reported<uint8_t>(AGI_DESTINATION_A);
    const component_connector_network_address_updated_payload_t update {};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NETWORK_ADDRESS_UPDATED), update).get());
    EXPECT_EQ(AGI_DESTINATION_A, maximum.reported<uint8_t>());
}

// Verifies that Group command list membership rejects non matching pair.
TEST_F(AssociationGroupInfoTest, GroupCommandListMembershipRejectsNonMatchingPair)
{
    auto group = endpoint.emplace_node(AGI_NODE_GROUP, GROUP_TWO);
    group.emplace_node(group_command_list).set_reported<std::vector<uint8_t>>({TEST_SWITCH_MULTILEVEL_CC, COMMAND_SWITCH_MULTILEVEL_SET});
    EXPECT_FALSE(check_membership(TEST_SWITCH_MULTILEVEL_CC, ENDPOINT_PROPERTIES_ENDPOINT_TWO));
}

// Verifies that Group Name Get rejects an MQTT request missing its group.
TEST_F(AssociationGroupInfoTest, MqttNameGetRejectsMissingGroupIdentifier)
{
    EXPECT_NE(SL_STATUS_OK, command_class_association_grp_info_mqtt::mqtt_on_association_group_name_get_command(endpoint, "{}"));
}

// Verifies that Group Info Get rejects an MQTT request missing its group.
TEST_F(AssociationGroupInfoTest, MqttInfoGetRejectsMissingGroupIdentifier)
{
    EXPECT_NE(SL_STATUS_OK, command_class_association_grp_info_mqtt::mqtt_on_association_group_info_get_command(endpoint, "{}"));
}

// Verifies that Group Command List Get rejects an MQTT request missing its group.
TEST_F(AssociationGroupInfoTest, MqttCommandListGetRejectsMissingGroupIdentifier)
{
    EXPECT_NE(SL_STATUS_OK, command_class_association_grp_info_mqtt::mqtt_on_association_group_command_list_get_command(endpoint, "{}"));
}
