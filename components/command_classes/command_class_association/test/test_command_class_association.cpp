/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************/
#include <gtest/gtest.h>
#include <fmt/format.h>

#include <algorithm>
#include <vector>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
#include "command_class_association.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_generated_types.hpp"
#include "command_class_association_mqtt.hpp"
#include "command_class_association_types.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "component_connector.hpp"
#include "mock_agi_connector.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_association_types;

namespace
{
    constexpr uint8_t ASSOCIATION_CC                       = 0x85;
    constexpr uint16_t NODE_ID                             = 2;
    constexpr uint8_t ENDPOINT_ID                          = 0;
    constexpr uint8_t LIFELINE_GROUP                       = 1;
    constexpr uint8_t GROUP_TWO                            = 2;
    constexpr uint8_t MAX_NODES                            = 10;
    constexpr uint8_t FRAME_CAPACITY                       = 32;
    constexpr uint8_t ASSOCIATION_DESTINATION_A            = 7;
    constexpr uint8_t ASSOCIATION_DESTINATION_B            = 9;
    constexpr uint8_t ASSOCIATION_DESTINATION_C            = 11;
    constexpr uint8_t ASSOCIATION_DESTINATION_D            = 8;
    constexpr uint8_t GROUP_THREE                          = 3;
    constexpr uint8_t GROUP_FOUR                           = 4;
    constexpr uint8_t UNSUPPORTED_GROUP_ID                 = 8;
    constexpr uint8_t HOME_ID_BYTE_COUNT                   = 4;
    constexpr uint8_t HOME_ID_BYTE_VALUE                   = 0;
    constexpr uint8_t V1_REPORT_MAX_NODES                  = 5;
    constexpr uint8_t LONG_NODE_LIST_SIZE                  = 60;
    constexpr uint8_t FIRST_ASSOCIATION_NODE_ID            = 1;
    constexpr uint8_t ASSOCIATION_GET_FRAME_LENGTH         = 3;
    constexpr uint8_t NO_REPORTS_TO_FOLLOW                 = 0;
    constexpr uint8_t NO_SUPPORTED_GROUPS                  = 0;
    constexpr uint8_t NO_GROUPING_IDENTIFIER               = 0;
    constexpr uint8_t ASSOCIATION_GROUPING_ID_OFFSET       = 2;
    constexpr uint8_t ASSOCIATION_REPORTS_TO_FOLLOW_OFFSET = 4;
    constexpr uint8_t ASSOCIATION_SET_NODE_IDS_OFFSET      = 3;
    constexpr uint8_t INITIAL_FRAME_LENGTH                 = 0;
    constexpr uint8_t UNKNOWN_MAX_NODES                    = 0;
    constexpr uint8_t TEST_CAPACITY_TWO                    = 2;
    constexpr size_t MINIMUM_ASSOCIATION_REPORT_SIZE       = 5;

    attribute_store::attribute seed_report_group(attribute_store::attribute endpoint, uint8_t group_id, std::vector<uint8_t> node_ids)
    {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
        group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(group_id);
        group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).set_reported<std::vector<uint8_t>>(std::move(node_ids));
        return group;
    }
}  // namespace

class TestableAssociationMqtt : public command_class_association_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class AssociationTest : public ::testing::Test
{
    protected:
        static command_class_association &cc()
        {
            static command_class_association instance;
            return instance;
        }

        static TestableAssociationMqtt &mqtt()
        {
            static TestableAssociationMqtt instance;
            return instance;
        }

        static testing::NiceMock<test::MockAgiConnector> &agi()
        {
            static testing::NiceMock<test::MockAgiConnector> instance;
            return instance;
        }

        static void connect_agi()
        {
            static const bool registered = [] {
                component_connector registry;
                agi().connect_events(registry);
                return true;
            }();
            (void)registered;
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            connector.start();
            auto root = attribute_store::attribute::root();
            auto home = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(HOME_ID_BYTE_COUNT, HOME_ID_BYTE_VALUE));
            endpoint  = home.emplace_node(ATTRIBUTE_NODE_ID, NODE_ID).emplace_node(ATTRIBUTE_ENDPOINT_ID, ENDPOINT_ID);
            (void)cc();
            (void)mqtt();
            connect_agi();
            agi().reset_defaults();
        }

        void TearDown() override
        {
            connector.stop();
            attribute_store_teardown();
        }

        const zwave_controller_connection_info_t *connection() const
        {
            peer.remote.node_id     = NODE_ID;
            peer.remote.endpoint_id = ENDPOINT_ID;
            return &peer;
        }

        mutable zwave_controller_connection_info_t peer {};
        attribute_store::attribute endpoint;
        component_connector connector;
};

// Verifies that Supported groupings get creates interview record.
// Requirement: CC:0085.01.05.11.001.
TEST_F(AssociationTest, SupportedGroupingsGetCreatesInterviewRecord)
{
    const component_connector_association_groupings_get_payload_t request {endpoint};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GROUPINGS_GET), request).get());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(association_groupings_get_group_attributes_t::ASSOCIATION_GROUPINGS_GET_GROUP)).is_valid());
}

// Verifies that Association get stores requested group.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, AssociationGetStoresRequestedGroup)
{
    const component_connector_association_get_payload_t request {endpoint, GROUP_TWO};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GET), request).get());
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_get_group_attributes_t::ASSOCIATION_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
}

// Verifies that Association set stores group and destination.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, AssociationSetStoresGroupAndDestination)
{
    constexpr uint8_t destination = ASSOCIATION_DESTINATION_B;
    const component_connector_association_set_payload_t request {static_cast<attribute_store_node_t>(endpoint), GROUP_TWO, destination, ENDPOINT_ID};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SET), request).get());
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_set_group_attributes_t::ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_set_group_attributes_t::grouping_identifier)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {destination}), group.child_by_type(static_cast<attribute_store_type_t>(association_set_group_attributes_t::node_id)).desired<std::vector<uint8_t>>());
}

// Verifies that Supported groupings count is zero until a report is stored.
// Requirement: CC:0085.01.06.11.001.
TEST_F(AssociationTest, SupportedGroupingsCountIsZeroUntilAReportIsStored)
{
    const component_connector_association_groupings_get_payload_t request {endpoint};
    auto [status, count] = connector.fire_event_async<component_connector_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_SUPPORTED_GROUPS, count);
}

// Verifies that Supported groupings count reads only reported value.
// Requirement: CC:0085.01.06.11.001.
TEST_F(AssociationTest, SupportedGroupingsCountReadsOnlyReportedValue)
{
    constexpr uint8_t expected_count = GROUP_THREE;
    auto report                      = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_groupings_report_group_attributes_t::ASSOCIATION_GROUPINGS_REPORT_GROUP));
    report.emplace_node(static_cast<attribute_store_type_t>(association_groupings_report_group_attributes_t::supported_groupings)).set_reported<uint8_t>(expected_count);
    const component_connector_association_groupings_get_payload_t request {endpoint};
    auto [status, count] = connector.fire_event_async<component_connector_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(expected_count, count);
}

// Verifies that Supported groupings report stores version one count.
// Requirement: CC:0085.01.06.11.001.
TEST_F(AssociationTest, SupportedGroupingsReportStoresVersionOneCount)
{
    constexpr uint8_t supported_groupings = GROUP_FOUR;
    // Fixture fields: supported_groupings is the reported group count.
    const command_class_association_attribute_map_t report {{"supported_groupings", supported_groupings}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_groupings_report_received_store(endpoint, report));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_groupings_report_group_attributes_t::ASSOCIATION_GROUPINGS_REPORT_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(supported_groupings, group.child_by_type(static_cast<attribute_store_type_t>(association_groupings_report_group_attributes_t::supported_groupings)).reported<uint8_t>());
}

// Verifies that Association report version one parses node list.
// Requirement: CC:0085.01.03.11.001.
TEST_F(AssociationTest, AssociationReportVersionOneParsesNodeList)
{
    constexpr uint8_t report_command    = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT);
    constexpr uint8_t max_nodes         = V1_REPORT_MAX_NODES;
    constexpr uint8_t reports_to_follow = NO_REPORTS_TO_FOLLOW;
    // V1 Report bytes are CC, command, group, capacity, reports-to-follow, then NodeIDs.
    const std::vector<uint8_t> frame {ASSOCIATION_CC, report_command, LIFELINE_GROUP, max_nodes, reports_to_follow, ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_association_core::report_received_args args {endpoint, parser, nullptr};
    EXPECT_EQ(SL_STATUS_OK, cc().on_association_report_received(args));
}

// Verifies that Association get version one encodes only group identifier.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, AssociationGetVersionOneEncodesOnlyGroupIdentifier)
{
    constexpr uint8_t command      = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_GET);
    constexpr uint8_t frame_length = ASSOCIATION_GET_FRAME_LENGTH;
    auto group                     = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::ASSOCIATION_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {ASSOCIATION_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_get_requested(args, bytes, &length));
    EXPECT_EQ(frame_length, length);
    // V1 Get frame is CC, command, and the selected group identifier.
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_CC, command, GROUP_TWO}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Association get waits for group identifier.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, AssociationGetWaitsForGroupIdentifier)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_GET);
    auto group                = endpoint.add_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::ASSOCIATION_GET_GROUP));
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {ASSOCIATION_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_core::get_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, cc().on_association_get_requested(args, bytes, &length));
}

// Verifies that Association set version one encodes group and node ids.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, AssociationSetVersionOneEncodesGroupAndNodeIds)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_SET);
    const std::vector<uint8_t> nodes {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::ASSOCIATION_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::node_id)).set_desired<std::vector<uint8_t>>(nodes);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {ASSOCIATION_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_set_requested(args, bytes, &length));
    // The command header identifies Association Set and its group; NodeID order is not significant.
    const std::vector<uint8_t> frame(bytes, bytes + length);
    ASSERT_EQ(ASSOCIATION_SET_NODE_IDS_OFFSET + nodes.size(), frame.size());
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_CC, command, GROUP_TWO}), std::vector<uint8_t>(frame.begin(), frame.begin() + ASSOCIATION_SET_NODE_IDS_OFFSET));
    auto encoded_node_ids  = std::vector<uint8_t>(frame.begin() + ASSOCIATION_SET_NODE_IDS_OFFSET, frame.end());
    auto expected_node_ids = nodes;
    std::sort(encoded_node_ids.begin(), encoded_node_ids.end());
    std::sort(expected_node_ids.begin(), expected_node_ids.end());
    EXPECT_EQ(expected_node_ids, encoded_node_ids);
}

// Verifies that Association set waits for required node list.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, AssociationSetWaitsForRequiredNodeList)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_SET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::ASSOCIATION_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {ASSOCIATION_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, static_cast<command_class_association_core &>(cc()).on_association_set_requested(args, bytes, &length));
}

// Verifies that Mqtt association set stores requested values.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, MqttAssociationSetStoresRequestedValues)
{
    mqtt().mqtt_command_handler(endpoint, "AssociationSet", fmt::format(R"({{"grouping_identifier":{},"node_id":[{},{}]}})", GROUP_TWO, ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_set_group_attributes_t::ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_set_group_attributes_t::grouping_identifier)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B}), group.child_by_type(static_cast<attribute_store_type_t>(association_set_group_attributes_t::node_id)).desired<std::vector<uint8_t>>());
}

// Verifies that Support set adds unique destinations and updates lifeline.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, SupportSetAddsUniqueDestinationsAndUpdatesLifeline)
{
    constexpr uint8_t group_id = LIFELINE_GROUP;
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", group_id}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B, ASSOCIATION_DESTINATION_A}}};
    EXPECT_CALL(agi(), add_lifeline_nodes(testing::_)).WillOnce([](const auto &payload) {
        EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B, ASSOCIATION_DESTINATION_A}), payload.node_ids);
        return SL_STATUS_OK;
    });
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_set_support_received(connection(), request));
    const auto stored_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    ASSERT_TRUE(stored_group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B}), stored_group.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
}

// Verifies that Support set rejects unsupported group.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, SupportSetRejectsUnsupportedGroup)
{
    agi().supported_groupings = LIFELINE_GROUP;
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    EXPECT_EQ(SL_STATUS_FAIL, static_cast<command_class_association_core &>(cc()).on_association_set_support_received(connection(), request));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP)).empty());
}

// Verifies that Support set rejects capacity overflow.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, SupportSetRejectsCapacityOverflow)
{
    agi().max_nodes = TEST_CAPACITY_TWO;
    seed_report_group(endpoint, GROUP_TWO, {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_C}}};
    EXPECT_EQ(SL_STATUS_FAIL, static_cast<command_class_association_core &>(cc()).on_association_set_support_received(connection(), request));
}

// Verifies that Support set allows unknown capacity.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, SupportSetAllowsUnknownCapacity)
{
    agi().max_nodes = UNKNOWN_MAX_NODES;
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_set_support_received(connection(), request));
}

// Verifies that Support remove deletes requested node.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveDeletesRequestedNode)
{
    seed_report_group(endpoint, GROUP_TWO, {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B, ASSOCIATION_DESTINATION_C});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_B}}};
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    const auto group = endpoint.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP)).front();
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_C}), group.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
}

// Verifies that Support remove group zero clears all groups.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveGroupZeroClearsAllGroups)
{
    seed_report_group(endpoint, LIFELINE_GROUP, {ASSOCIATION_DESTINATION_A});
    seed_report_group(endpoint, GROUP_TWO, {ASSOCIATION_DESTINATION_B});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", uint8_t {NO_GROUPING_IDENTIFIER}}, {"node_id", std::vector<uint8_t> {}}};
    EXPECT_CALL(agi(), get_group_destinations(testing::_, testing::_)).WillOnce([](const auto &, auto &result) {
        result.node_ids = {ASSOCIATION_DESTINATION_A};
        return SL_STATUS_OK;
    });
    EXPECT_CALL(agi(), remove_lifeline_nodes(testing::_)).WillOnce([](const auto &payload) {
        EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}), payload.node_ids);
        return SL_STATUS_OK;
    });
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    for (const auto &group: endpoint.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP))) {
        EXPECT_TRUE(group.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>().empty());
    }
}

// Verifies that Support remove ignores unsupported group.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveIgnoresUnsupportedGroup)
{
    seed_report_group(endpoint, GROUP_TWO, {ASSOCIATION_DESTINATION_A});
    agi().supported_groupings = LIFELINE_GROUP;
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    const auto group = endpoint.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP)).front();
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}), group.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
}

// Verifies that Support groupings get reports ag i group count.
// Requirement: CC:0085.01.05.11.001.
TEST_F(AssociationTest, SupportGroupingsGetReportsAgIGroupCount)
{
    agi().supported_groupings = GROUP_THREE;
    zwave_frame_generator_standalone report;
    report.add_header(ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_GROUPINGS_REPORT));
    std::vector<uint8_t> bytes;
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_groupings_get_support_requested(connection(), {}, report, bytes));
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_GROUPINGS_REPORT), GROUP_THREE}), bytes);
}

// Verifies that Support specific group get reports unsupported.
// Requirement: CC:0085.02.0B.11.001.
TEST_F(AssociationTest, SupportSpecificGroupGetReportsUnsupported)
{
    zwave_frame_generator_standalone report;
    report.add_header(ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_SPECIFIC_GROUP_REPORT));
    std::vector<uint8_t> bytes;
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_specific_group_get_support_requested(connection(), {}, report, bytes));
    EXPECT_EQ(NO_SUPPORTED_GROUPS, bytes.back());
}

// Verifies that Support association get uses ag i group destinations.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, SupportAssociationGetUsesAgIGroupDestinations)
{
    agi().supported_groupings   = GROUP_TWO;
    agi().max_nodes             = MAX_NODES;
    agi().destinations.node_ids = {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B};
    zwave_frame_generator_standalone report;
    report.add_header(ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT));
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_get_support_requested(connection(), request, report, bytes));
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT), GROUP_TWO, MAX_NODES, NO_REPORTS_TO_FOLLOW, ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B}), bytes);
}

// Verifies that Support association get falls back to lifeline for unsupported group.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, SupportAssociationGetFallsBackToLifelineForUnsupportedGroup)
{
    agi().supported_groupings   = LIFELINE_GROUP;
    agi().destinations.node_ids = {ASSOCIATION_DESTINATION_A};
    zwave_frame_generator_standalone report;
    report.add_header(ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT));
    std::vector<uint8_t> bytes;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", uint8_t {UNSUPPORTED_GROUP_ID}}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_get_support_requested(connection(), request, report, bytes));
    EXPECT_EQ(LIFELINE_GROUP, bytes[ASSOCIATION_GROUPING_ID_OFFSET]);
    EXPECT_EQ(ASSOCIATION_DESTINATION_A, bytes.back());
}

// Verifies that Association report parser publishes requested group.
TEST_F(AssociationTest, AssociationReportParserPublishesRequestedGroup)
{
    constexpr uint8_t report_command = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT);
    const std::vector<uint8_t> frame {ASSOCIATION_CC, report_command, GROUP_TWO, V1_REPORT_MAX_NODES, NO_REPORTS_TO_FOLLOW, ASSOCIATION_DESTINATION_A};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_association_core::report_received_args args {endpoint, parser, nullptr};
    EXPECT_EQ(SL_STATUS_OK, cc().on_association_report_received(args));
}

// Verifies that Specific group report store remains no op.
TEST_F(AssociationTest, SpecificGroupReportStoreRemainsNoOp)
{
    EXPECT_EQ(SL_STATUS_OK, cc().on_association_specific_group_report_received_store(endpoint, {}));
}

// Verifies that Association remove version one decodes group and node list.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, AssociationRemoveVersionOneDecodesGroupAndNodeList)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REMOVE);
    const std::vector<uint8_t> frame {ASSOCIATION_CC, command, GROUP_TWO, ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_association_core::set_received_args args {endpoint, parser};
    const auto decoded = cc().command_class_association_core::on_association_remove_received(args);
    EXPECT_EQ(GROUP_TWO, std::get<uint8_t>(decoded.at("grouping_identifier")));
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B}), std::get<std::vector<uint8_t>>(decoded.at("node_id")));
}

// Verifies that Support set continues when ag i queries fail.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, SupportSetContinuesWhenAgIQueriesFail)
{
    agi().count_status    = SL_STATUS_NOT_AVAILABLE;
    agi().capacity_status = SL_STATUS_NOT_AVAILABLE;
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_set_support_received(connection(), request));
    const auto groups = endpoint.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    ASSERT_EQ(FIRST_ASSOCIATION_NODE_ID, groups.size());
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}), groups.front().child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
}

// Verifies that Support set does not duplicate existing nodes.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, SupportSetDoesNotDuplicateExistingNodes)
{
    seed_report_group(endpoint, GROUP_TWO, {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_B, ASSOCIATION_DESTINATION_C, ASSOCIATION_DESTINATION_C}}};
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_set_support_received(connection(), request));
    const auto group = endpoint.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP)).front();
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B, ASSOCIATION_DESTINATION_C}), group.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
}

// Verifies that Support remove leaves unselected group untouched.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveLeavesUnselectedGroupUntouched)
{
    auto lifeline = seed_report_group(endpoint, LIFELINE_GROUP, {ASSOCIATION_DESTINATION_A});
    auto other    = seed_report_group(endpoint, GROUP_TWO, {ASSOCIATION_DESTINATION_B});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_B}}};
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}), lifeline.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
    EXPECT_TRUE(other.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>().empty());
}

// Verifies that Support remove ignores groups without reported identifiers.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveIgnoresGroupsWithoutReportedIdentifiers)
{
    auto invalid = endpoint.add_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    invalid.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", uint8_t {NO_GROUPING_IDENTIFIER}}, {"node_id", std::vector<uint8_t> {}}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    EXPECT_FALSE(invalid.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier)).reported_exists());
}

// Verifies that Mqtt get stores requested group.
TEST_F(AssociationTest, MqttGetStoresRequestedGroup)
{
    mqtt().mqtt_command_handler(endpoint, "AssociationGet", fmt::format(R"({{"grouping_identifier":{}}})", GROUP_TWO));
    const auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_get_group_attributes_t::ASSOCIATION_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
}

// Verifies that Mqtt remove stores group and node ids.
TEST_F(AssociationTest, MqttRemoveStoresGroupAndNodeIds)
{
    mqtt().mqtt_command_handler(endpoint, "AssociationRemove", fmt::format(R"({{"grouping_identifier":{},"node_id":[{},{}]}})", GROUP_TWO, ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B));
    const auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_remove_group_attributes_t::ASSOCIATION_REMOVE_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(association_remove_group_attributes_t::grouping_identifier)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B}), group.child_by_type(static_cast<attribute_store_type_t>(association_remove_group_attributes_t::node_id)).desired<std::vector<uint8_t>>());
}

// Verifies that Mqtt set rejects missing group identifier.
TEST_F(AssociationTest, MqttSetRejectsMissingGroupIdentifier)
{
    EXPECT_NE(SL_STATUS_OK, command_class_association_mqtt::mqtt_on_association_set_command(endpoint, fmt::format(R"({{"node_id":[{}]}})", ASSOCIATION_DESTINATION_A)));
}

// Verifies that Mqtt get rejects missing group identifier.
TEST_F(AssociationTest, MqttGetRejectsMissingGroupIdentifier)
{
    EXPECT_NE(SL_STATUS_OK, command_class_association_mqtt::mqtt_on_association_get_command(endpoint, "{}"));
}

// Verifies that Mqtt remove rejects missing node list.
TEST_F(AssociationTest, MqttRemoveRejectsMissingNodeList)
{
    EXPECT_NE(SL_STATUS_OK, command_class_association_mqtt::mqtt_on_association_remove_command(endpoint, fmt::format(R"({{"grouping_identifier":{}}})", LIFELINE_GROUP)));
}

// Verifies that Mqtt groupings get accepts empty payload.
// Requirement: CC:0085.01.05.11.001.
TEST_F(AssociationTest, MqttGroupingsGetAcceptsEmptyPayload)
{
    mqtt().mqtt_command_handler(endpoint, "AssociationGroupingsGet", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(association_groupings_get_group_attributes_t::ASSOCIATION_GROUPINGS_GET_GROUP)).is_valid());
}

// Verifies that Support association get returns empty lifeline when ag i queries fail.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, SupportAssociationGetReturnsEmptyLifelineWhenAgIQueriesFail)
{
    agi().count_status        = SL_STATUS_NOT_AVAILABLE;
    agi().capacity_status     = SL_STATUS_NOT_AVAILABLE;
    agi().destinations_status = SL_STATUS_NOT_AVAILABLE;
    zwave_frame_generator_standalone report;
    report.add_header(ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT));
    std::vector<uint8_t> frame;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_get_support_requested(connection(), request, report, frame));
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT), LIFELINE_GROUP, UNKNOWN_MAX_NODES, NO_REPORTS_TO_FOLLOW}), frame);
}

// Verifies that Support association get splits long node list across reports.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, SupportAssociationGetSplitsLongNodeListAcrossReports)
{
    const std::vector<uint8_t> destinations = [] {
        std::vector<uint8_t> ids(LONG_NODE_LIST_SIZE);
        for (size_t index = 0; index < ids.size(); ++index) {
            ids[index] = static_cast<uint8_t>(index + FIRST_ASSOCIATION_NODE_ID);
        }
        return ids;
    }();
    agi().destinations.node_ids = destinations;
    zwave_frame_generator_standalone report;
    report.add_header(ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT));
    std::vector<uint8_t> final_frame;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_get_support_requested(connection(), request, report, final_frame));
    ASSERT_GE(final_frame.size(), MINIMUM_ASSOCIATION_REPORT_SIZE);
    EXPECT_EQ(NO_REPORTS_TO_FOLLOW, final_frame[ASSOCIATION_REPORTS_TO_FOLLOW_OFFSET]);
    EXPECT_EQ(destinations.back(), final_frame.back());
}

// Verifies that Supported groupings count is zero when report group is missing.
// Requirement: CC:0085.01.06.11.001.
TEST_F(AssociationTest, SupportedGroupingsCountIsZeroWhenReportGroupIsMissing)
{
    const component_connector_association_groupings_get_payload_t request {endpoint};
    auto [status, result] = connector.fire_event_async<component_connector_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    EXPECT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_SUPPORTED_GROUPS, result);
}

// Verifies that Supported groupings count is zero when count is desired only.
// Requirement: CC:0085.01.06.11.001.
TEST_F(AssociationTest, SupportedGroupingsCountIsZeroWhenCountIsDesiredOnly)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_groupings_report_group_attributes_t::ASSOCIATION_GROUPINGS_REPORT_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_groupings_report_group_attributes_t::supported_groupings)).set_desired<uint8_t>(GROUP_TWO);
    const component_connector_association_groupings_get_payload_t request {endpoint};
    auto [status, result] = connector.fire_event_async<component_connector_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    EXPECT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_SUPPORTED_GROUPS, result);
}

// Verifies that Supported groupings count is zero when report has no count attribute.
// Requirement: CC:0085.01.06.11.001.
TEST_F(AssociationTest, SupportedGroupingsCountIsZeroWhenReportHasNoCountAttribute)
{
    endpoint.emplace_node(static_cast<attribute_store_type_t>(association_groupings_report_group_attributes_t::ASSOCIATION_GROUPINGS_REPORT_GROUP));
    const component_connector_association_groupings_get_payload_t request {endpoint};
    auto [status, result] = connector.fire_event_async<component_connector_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    EXPECT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_SUPPORTED_GROUPS, result);
}

// Verifies that Support remove does not notify for unmatched lifeline destination.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveDoesNotNotifyForUnmatchedLifelineDestination)
{
    seed_report_group(endpoint, LIFELINE_GROUP, {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_D});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_B}}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_D}),
              endpoint.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP)).child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
}

// Verifies that Support remove all for non lifeline group does not clear lifeline.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveAllForNonLifelineGroupDoesNotClearLifeline)
{
    seed_report_group(endpoint, LIFELINE_GROUP, {ASSOCIATION_DESTINATION_A});
    seed_report_group(endpoint, GROUP_TWO, {ASSOCIATION_DESTINATION_D});
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    auto lifeline = endpoint.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    EXPECT_EQ((std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}), lifeline.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).reported<std::vector<uint8_t>>());
}

// Verifies that Support remove skips groups without reported destinations.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveSkipsGroupsWithoutReportedDestinations)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(GROUP_TWO);
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
}

// Verifies that Association set waits for grouping identifier.
// Requirement: CC:0085.01.01.11.001.
TEST_F(AssociationTest, AssociationSetWaitsForGroupingIdentifier)
{
    constexpr uint8_t command = static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_SET);
    auto group                = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::ASSOCIATION_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::node_id)).set_desired<std::vector<uint8_t>>({ASSOCIATION_DESTINATION_A});
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {ASSOCIATION_CC};
    generator.initialize_frame(command, bytes, &length);
    command_class_association_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, static_cast<command_class_association_core &>(cc()).on_association_set_requested(args, bytes, &length));
}

// Verifies that Support association get uses lifeline for group zero.
// Requirement: CC:0085.01.02.11.001.
TEST_F(AssociationTest, SupportAssociationGetUsesLifelineForGroupZero)
{
    agi().destinations.node_ids = {ASSOCIATION_DESTINATION_A};
    zwave_frame_generator_standalone report;
    report.add_header(ASSOCIATION_CC, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT));
    std::vector<uint8_t> frame;
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", NO_GROUPING_IDENTIFIER}};
    ASSERT_EQ(SL_STATUS_OK, cc().on_association_get_support_requested(connection(), request, report, frame));
    EXPECT_EQ(LIFELINE_GROUP, frame[ASSOCIATION_GROUPING_ID_OFFSET]);
}

// Verifies that Support remove notifies ag i only for destinations found in lifeline.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveNotifiesAgIOnlyForDestinationsFoundInLifeline)
{
    seed_report_group(endpoint, LIFELINE_GROUP, {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_D});
    agi().destinations.node_ids = {ASSOCIATION_DESTINATION_A, ASSOCIATION_DESTINATION_B};
    // Fixture fields: grouping_identifier selects the association group.
    const command_class_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}};
    ASSERT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
    EXPECT_TRUE(agi().wait_for_notification());
}

// Verifies that Support set rejects group zero when supported groupings are known.
// Requirement: CC:0085.01.05.11.001.
TEST_F(AssociationTest, SupportSetRejectsGroupZeroWhenSupportedGroupingsAreKnown)
{
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", NO_GROUPING_IDENTIFIER}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    EXPECT_NE(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_set_support_received(connection(), request));
}

// Verifies that Support remove skips group with missing destination attribute.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveSkipsGroupWithMissingDestinationAttribute)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(GROUP_TWO);
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
}

// Verifies that Support remove skips group with desired only identifier.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveSkipsGroupWithDesiredOnlyIdentifier)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).set_reported<std::vector<uint8_t>>({ASSOCIATION_DESTINATION_A});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
}

// Verifies that Support remove skips desired only destination list.
// Requirement: CC:0085.01.04.11.001.
TEST_F(AssociationTest, SupportRemoveSkipsDesiredOnlyDestinationList)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid)).set_desired<std::vector<uint8_t>>({ASSOCIATION_DESTINATION_A});
    // Fixture fields: grouping_identifier selects the association group; node_id lists destination NodeIDs.
    const command_class_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {ASSOCIATION_DESTINATION_A}}};
    EXPECT_EQ(SL_STATUS_OK, static_cast<command_class_association_core &>(cc()).on_association_remove_support_received(connection(), request));
}
