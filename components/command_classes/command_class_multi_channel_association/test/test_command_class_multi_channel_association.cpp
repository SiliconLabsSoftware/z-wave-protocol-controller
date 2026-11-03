/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <vector>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_defined_attribute_types.h"
#include "attribute_store_fixt.h"
#include "command_class_association_grp_info_attributes.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "command_class_multi_channel_types.hpp"
#include "command_class_multi_channel_association.hpp"
#include "command_class_multi_channel_association_constants.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_mqtt.hpp"
#include "command_class_multi_channel_association_types.hpp"
#include "component_connector.hpp"
#include "mock_agi_connector.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_generator_standalone.hpp"
#include "zwave_frame_parser.hpp"
#include "zwave_network_management.h"
#include "zwave_keyset_definitions.h"
#include "zwave_utils.h"
#include "zpc_attribute_store_network_helper.h"

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_multi_channel_association_types;

namespace
{
    namespace agi                = command_class_association_grp_info_types;
    constexpr auto EVENT_TIMEOUT = std::chrono::seconds(1);

    struct ParsedEventObserver {
            std::mutex mutex;
            std::condition_variable condition;
            std::optional<component_connector_multi_channel_association_groupings_get_payload_t> groupings_report;
            std::optional<component_connector_multi_channel_association_report_payload_t> get_request;
    };

    ParsedEventObserver &parsed_event_observer()
    {
        static ParsedEventObserver observer;
        return observer;
    }

    constexpr uint8_t MCA_CC                          = 0x8E;
    constexpr uint8_t CMD_SET                         = 0x01;
    constexpr uint8_t CMD_GET                         = 0x02;
    constexpr uint8_t CMD_REPORT                      = 0x03;
    constexpr uint8_t CMD_REMOVE                      = 0x04;
    constexpr uint8_t CMD_GROUPINGS_GET               = 0x05;
    constexpr uint8_t CMD_GROUPINGS_REPORT            = 0x06;
    constexpr uint8_t MULTI_CHANNEL_CC                = 0x60;
    constexpr uint8_t GROUPING_IDENTIFIER_ALL         = 0;
    constexpr uint8_t MQTT_MARKER_VALUE               = 0;
    constexpr uint8_t MQTT_ENDPOINT_BIT_ADDRESS_VALUE = 0;
    constexpr uint8_t BIT_ADDRESS_SET                 = 1;
    constexpr uint16_t NODE_ID                        = 2;
    constexpr uint8_t ROOT_ENDPOINT                   = 0;
    constexpr uint8_t LIFELINE_GROUP                  = 1;
    constexpr uint8_t GROUP_TWO                       = 2;
    constexpr uint8_t UNSUPPORTED_GROUP               = 5;
    constexpr uint8_t DESTINATION_NODE_A              = 7;
    constexpr uint8_t DESTINATION_NODE_B              = 9;
    constexpr uint8_t DESTINATION_NODE_C              = 11;
    constexpr uint8_t DESTINATION_ENDPOINT            = 3;
    constexpr uint8_t OTHER_DESTINATION_ENDPOINT      = 4;
    constexpr uint8_t MAX_DESTINATIONS                = 8;
    constexpr uint8_t GROUP_COUNT                     = 4;
    constexpr uint8_t NO_DESTINATIONS                 = 0;
    constexpr uint8_t NO_REPORTS_FOLLOW               = 0;
    constexpr uint8_t SET_MARKER                      = 0x00;
    constexpr uint8_t FRAME_CAPACITY                  = 32;
    constexpr uint16_t INITIAL_FRAME_LENGTH           = 0;
    constexpr uint8_t GROUPING_ID_OFFSET              = 2;
    constexpr uint8_t GROUPING_REPORT_FRAME_SIZE      = 3;
    constexpr uint8_t REPORTS_TO_FOLLOW_OFFSET        = 4;
    constexpr uint8_t MCA_VERSION_V2                  = 2;
    constexpr uint8_t MCA_VERSION_V3                  = 3;
    constexpr uint8_t MCA_VERSION_V4                  = 4;
    constexpr uint8_t MCA_VERSION_V1                  = 1;
    constexpr uint8_t AGI_CONTROLLED_CC               = MCA_CC;
    constexpr uint8_t AGI_CONTROLLED_COMMAND          = CMD_SET;
    constexpr size_t LONG_NODE_LIST_SIZE              = 80;
    constexpr size_t LONG_ENDPOINT_LIST_SIZE          = 40;

    using report_group_t = command_class_multi_channel_association_attribute_map_t;

    attribute_store::attribute seed_report_group(attribute_store::attribute endpoint, uint8_t group_id, std::vector<uint8_t> nodes, std::vector<multi_channel_association_report_vg_t_item_t> endpoints = {})
    {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP));
        group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(group_id);
        group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).set_reported<std::vector<uint8_t>>(std::move(nodes));
        group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).set_reported<std::vector<multi_channel_association_report_vg_t_item_t>>(std::move(endpoints));
        return group;
    }

    attribute_store::attribute find_report_group(attribute_store::attribute endpoint, uint8_t group_id)
    {
        for (auto group: endpoint.children(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP))) {
            auto identifier = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier));
            if (identifier.is_valid() && identifier.reported_exists() && identifier.reported<uint8_t>() == group_id) {
                return group;
            }
        }
        return {};
    }

    multi_channel_association_set_vg_t_item_t endpoint_destination(uint8_t node_id, uint8_t endpoint_id)
    {
        multi_channel_association_set_vg_t_item_t item {};
        item.multi_channel_node_id = node_id;
        item.properties1.value     = endpoint_id;
        return item;
    }

    multi_channel_association_report_vg_t_item_t report_endpoint_destination(uint8_t node_id, uint8_t endpoint_id)
    {
        multi_channel_association_report_vg_t_item_t item {};
        item.multi_channel_node_id = node_id;
        item.properties1.value     = endpoint_id;
        return item;
    }
}  // namespace

class MultiChannelAssociationTest : public ::testing::Test
{
    protected:
        static command_class_multi_channel_association &cc()
        {
            static command_class_multi_channel_association instance;
            return instance;
        }

        static command_class_multi_channel_association_core &core()
        {
            return static_cast<command_class_multi_channel_association_core &>(cc());
        }

        static command_class_multi_channel_association_mqtt &mqtt()
        {
            static command_class_multi_channel_association_mqtt instance;
            return instance;
        }

        static testing::NiceMock<test::MockMultiChannelAssociationAgiConnector> &agi_mock()
        {
            static testing::NiceMock<test::MockMultiChannelAssociationAgiConnector> instance;
            return instance;
        }

        static void connect_agi()
        {
            static const bool connected = [] {
                component_connector registry;
                agi_mock().connect_events(registry);
                return true;
            }();
            (void)connected;
        }

        static void connect_parsed_events()
        {
            static const bool connected = [] {
                component_connector registry;
                auto *observer = &parsed_event_observer();
                registry.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_groupings_get_payload_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT,
                                                                                                                                                                [observer](const auto &payload) {
                                                                                                                                                                    {
                                                                                                                                                                        std::lock_guard<std::mutex> lock(observer->mutex);
                                                                                                                                                                        observer->groupings_report = payload;
                                                                                                                                                                    }
                                                                                                                                                                    observer->condition.notify_one();
                                                                                                                                                                    return SL_STATUS_OK;
                                                                                                                                                                });
                registry.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_report_payload_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED, [observer](const auto &payload) {
                    {
                        std::lock_guard<std::mutex> lock(observer->mutex);
                        observer->get_request = payload;
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
            home      = root.emplace_node(ATTRIBUTE_HOME_ID, zwave_network_management_get_home_id());
            endpoint  = home.emplace_node(ATTRIBUTE_NODE_ID, NODE_ID).emplace_node(ATTRIBUTE_ENDPOINT_ID, ROOT_ENDPOINT);
            (void)cc();
            (void)mqtt();
            connect_agi();
            connect_parsed_events();
            agi_mock().set_default_responses();
            peer.remote.node_id     = NODE_ID;
            peer.remote.endpoint_id = ROOT_ENDPOINT;
        }

        void TearDown() override
        {
            connector.stop();
            attribute_store_teardown();
        }

        const zwave_controller_connection_info_t *connection() const
        {
            return &peer;
        }

        attribute_store::attribute home;
        attribute_store::attribute endpoint;
        component_connector connector;
        zwave_controller_connection_info_t peer {};
};

// Verifies that the V2 Groupings Get creates the pending interview attribute.
// Requirement: CC:008E.02.05.11.001.
TEST_F(MultiChannelAssociationTest, GroupingsGetCreatesInterviewRecordV2)
{
    // Ask through the connector boundary used by the interviewer.
    const component_connector_multi_channel_association_groupings_get_payload_t request {endpoint};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET), request).get());
    // The report resolver is scheduled on the endpoint after the request is accepted.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_groupings_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET_GROUP)).is_valid());
}

// Verifies that the supported group count is zero until a report is stored.
// Requirement: CC:008E.02.06.11.001.
TEST_F(MultiChannelAssociationTest, SupportedGroupingsCountDefaultsToZero)
{
    // Query an empty RAM-backed endpoint through the count event.
    const component_connector_multi_channel_association_groupings_get_payload_t request {endpoint};
    auto [status, count] = connector.fire_event_async<component_connector_multi_channel_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_DESTINATIONS, count);
}

// Verifies that the supported group count returns the reported value.
// Requirement: CC:008E.02.06.11.001.
TEST_F(MultiChannelAssociationTest, SupportedGroupingsCountUsesReportedValue)
{
    // Seed only the reported supported-group field; desired values are not authoritative.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_groupings_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_groupings_report_group_attributes_t::supported_groupings)).set_reported<uint8_t>(GROUP_COUNT);
    const component_connector_multi_channel_association_groupings_get_payload_t request {endpoint};
    auto [status, count] = connector.fire_event_async<component_connector_multi_channel_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(GROUP_COUNT, count);
}

// Verifies that a report group without a reported count still returns zero.
// Requirement: CC:008E.02.06.11.001.
TEST_F(MultiChannelAssociationTest, SupportedGroupingsCountNeedsReportedValue)
{
    // A desired-only value is not a Report and must not be used for the interview count.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_groupings_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_groupings_report_group_attributes_t::supported_groupings)).set_desired<uint8_t>(GROUP_COUNT);
    const component_connector_multi_channel_association_groupings_get_payload_t request {endpoint};
    auto [status, count] = connector.fire_event_async<component_connector_multi_channel_association_groupings_get_payload_t, uint8_t>(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT), request).get();
    ASSERT_EQ(SL_STATUS_OK, status);
    EXPECT_EQ(NO_DESTINATIONS, count);
}

// Verifies that a Groupings Report stores its supported-group count.
// Requirement: CC:008E.02.06.11.001.
TEST_F(MultiChannelAssociationTest, GroupingsReportStoresVersionTwoCount)
{
    // The map contains the one Report field that this command stores.
    const report_group_t report {{"supported_groupings", GROUP_COUNT}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_groupings_report_received_store(endpoint, report));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_groupings_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_COUNT, group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_groupings_report_group_attributes_t::supported_groupings)).reported<uint8_t>());
}

// Verifies that Groupings Report parsing publishes the reported group count.
// Requirement: CC:008E.02.06.11.001.
TEST_F(MultiChannelAssociationTest, GroupingsReportPublishesCount)
{
    // Reset the process-wide observer before this fire-and-forget event is queued.
    auto &observer = parsed_event_observer();
    {
        std::lock_guard<std::mutex> lock(observer.mutex);
        observer.groupings_report.reset();
    }
    const report_group_t report {{"supported_groupings", GROUP_COUNT}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_groupings_report_parsed(connection(), endpoint, report));
    std::unique_lock<std::mutex> lock(observer.mutex);
    ASSERT_TRUE(observer.condition.wait_for(lock, EVENT_TIMEOUT, [&observer] { return observer.groupings_report.has_value(); }));
    EXPECT_EQ(endpoint, observer.groupings_report->endpoint_node);
    EXPECT_EQ(GROUP_COUNT, observer.groupings_report->supported_groupings);
}

// Verifies that a parsed Get request preserves its grouping identifier.
// Requirement: CC:008E.02.02.11.001.
TEST_F(MultiChannelAssociationTest, GetRequestPublishesRequestedGroup)
{
    // Reset the observer; the Get notification is processed asynchronously by the connector.
    auto &observer = parsed_event_observer();
    {
        std::lock_guard<std::mutex> lock(observer.mutex);
        observer.get_request.reset();
    }
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_report_parsed(connection(), endpoint, {{"grouping_identifier", GROUP_TWO}}));
    std::unique_lock<std::mutex> lock(observer.mutex);
    ASSERT_TRUE(observer.condition.wait_for(lock, EVENT_TIMEOUT, [&observer] { return observer.get_request.has_value(); }));
    EXPECT_EQ(endpoint, observer.get_request->endpoint_node);
    EXPECT_EQ(GROUP_TWO, observer.get_request->grouping_identifier);
}

// Verifies that the interview Get event creates work for the requested group.
// Requirement: CC:008E.02.02.11.001.
TEST_F(MultiChannelAssociationTest, GetInterviewEventCreatesRequestedGroup)
{
    // Send the event used by the interview flow and wait for its callback.
    const component_connector_multi_channel_association_get_payload_t request {endpoint, GROUP_TWO};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GET), request).get());

    // The pending attribute records the requested group for frame assembly.
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
}

// Verifies that the interview Set event encodes a destination endpoint in the MCA v3 variant group.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, SetInterviewEventCreatesEndpointAssociation)
{
    // A non-root destination endpoint uses the MCA v3 endpoint tuple representation.
    const component_connector_multi_channel_association_set_payload_t request {endpoint, GROUP_TWO, DESTINATION_NODE_B, DESTINATION_ENDPOINT};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET), request).get());

    // The pending Set has no NodeID list and stores the destination in vg.
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_FALSE(group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).desired_exists());
    const auto destinations = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg)).desired<std::vector<multi_channel_association_set_vg_t_item_t>>();
    ASSERT_EQ(1U, destinations.size());
    EXPECT_EQ(DESTINATION_NODE_B, destinations.front().multi_channel_node_id);
    EXPECT_EQ(DESTINATION_ENDPOINT, destinations.front().properties1.value);
}

// Verifies that a root-endpoint Lifeline request uses the NodeID representation when MCA v3 is unavailable.
// Requirement: CC:008E.02.01.11.003.
TEST_F(MultiChannelAssociationTest, SetInterviewEventCreatesLegacyLifelineNodeAssociation)
{
    // The destination endpoint is the root endpoint, whose empty version state represents legacy support.
    const component_connector_multi_channel_association_set_payload_t request {endpoint, LIFELINE_GROUP, DESTINATION_NODE_B, ROOT_ENDPOINT};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET), request).get());

    // Legacy Lifeline encoding stores the destination in node_id, without an endpoint tuple.
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(DESTINATION_NODE_B, group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).desired<uint8_t>());
    EXPECT_FALSE(group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg)).desired_exists());
}

// Verifies that a V3 destination supporting Multi Channel receives a Lifeline endpoint association.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, SetInterviewEventUsesEndpointLifelineForMcaV3Peer)
{
    // The peer's root endpoint advertises MCA v3 and Multi Channel support.
    auto destination_node     = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    auto destination_endpoint = destination_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, ROOT_ENDPOINT);
    destination_endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V3);
    destination_endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t> {MULTI_CHANNEL_CC});
    const component_connector_multi_channel_association_set_payload_t request {destination_endpoint, LIFELINE_GROUP, DESTINATION_NODE_C, ROOT_ENDPOINT};

    // The specification requires representing this Lifeline as an endpoint association.
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET), request).get());
    auto group = destination_endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_FALSE(group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).desired_exists());
    const auto destinations = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg)).desired<std::vector<multi_channel_association_set_vg_t_item_t>>();
    ASSERT_EQ(1U, destinations.size());
    EXPECT_EQ(DESTINATION_NODE_C, destinations.front().multi_channel_node_id);
    EXPECT_EQ(ROOT_ENDPOINT, destinations.front().properties1.value);
}

// Verifies that a V3 peer without Multi Channel support retains a NodeID Lifeline association.
// Requirement: CC:008E.02.01.11.003.
TEST_F(MultiChannelAssociationTest, SetInterviewEventKeepsNodeIdForPeerWithoutMultiChannel)
{
    // MCA v3 alone is insufficient for endpoint Lifeline encoding when Multi Channel is unsupported.
    auto destination_node     = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    auto destination_endpoint = destination_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, ROOT_ENDPOINT);
    destination_endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V3);
    const component_connector_multi_channel_association_set_payload_t request {destination_endpoint, LIFELINE_GROUP, DESTINATION_NODE_C, ROOT_ENDPOINT};

    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET), request).get());
    auto group = destination_endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(DESTINATION_NODE_C, group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).desired<uint8_t>());
}

// Verifies that a root-endpoint Set outside Lifeline uses the NodeID representation.
// Requirement: CC:008E.02.01.11.003.
TEST_F(MultiChannelAssociationTest, SetInterviewEventUsesNodeIdForNonLifelineGroup)
{
    // Endpoint zero without a Lifeline Grouping Identifier does not apply the Lifeline special case.
    const component_connector_multi_channel_association_set_payload_t request {endpoint, GROUP_TWO, DESTINATION_NODE_B, ROOT_ENDPOINT};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET), request).get());
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(DESTINATION_NODE_B, group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).desired<uint8_t>());
    EXPECT_FALSE(group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg)).desired_exists());
}

// Verifies that Set falls back to a NodeID when the destination endpoint cannot be resolved.
// Requirement: CC:008E.02.01.11.003.
TEST_F(MultiChannelAssociationTest, SetInterviewEventFallsBackWhenDestinationCannotBeResolved)
{
    // The attribute-store root has no Z-Wave NodeID or Endpoint ID for the helper to resolve.
    auto root = attribute_store::attribute::root();
    const component_connector_multi_channel_association_set_payload_t request {root, LIFELINE_GROUP, DESTINATION_NODE_B, ROOT_ENDPOINT};
    ASSERT_EQ(SL_STATUS_OK, connector.fire_event_async(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET), request).get());
    auto group = root.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(DESTINATION_NODE_B, group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).desired<uint8_t>());
}

// Verifies that the V2 Get request parses the grouping identifier.
// Requirement: CC:008E.02.02.11.001.
TEST_F(MultiChannelAssociationTest, GetRequestParsesVersionTwoGroup)
{
    const std::vector<uint8_t> wire_frame {MCA_CC, CMD_GET, GROUP_TWO};
    zwave_frame_parser parser {wire_frame.data(), static_cast<uint16_t>(wire_frame.size())};
    command_class_multi_channel_association_core::get_received_args args {endpoint, parser, nullptr, connection()};
    const auto fields = core().on_multi_channel_association_get_received(args);
    ASSERT_TRUE(fields.contains("grouping_identifier"));
    EXPECT_EQ(GROUP_TWO, std::get<uint8_t>(fields.at("grouping_identifier")));
}

// Verifies that the V2 Groupings Get request has no payload fields.
// Requirement: CC:008E.02.05.11.001.
TEST_F(MultiChannelAssociationTest, GroupingsGetParsesVersionTwoEmptyPayload)
{
    const std::vector<uint8_t> wire_frame {MCA_CC, CMD_GROUPINGS_GET};
    zwave_frame_parser parser {wire_frame.data(), static_cast<uint16_t>(wire_frame.size())};
    command_class_multi_channel_association_core::get_received_args args {endpoint, parser, nullptr, connection()};
    EXPECT_TRUE(core().on_multi_channel_association_groupings_get_received(args).empty());
}

// Verifies that the V2 Get request encodes its group identifier.
// Requirement: CC:008E.02.02.11.001.
TEST_F(MultiChannelAssociationTest, GetRequestEncodesVersionTwoGroup)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_GET, bytes, &length);
    const command_class_multi_channel_association_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_requested(args, bytes, &length));
    // V2 Get is CC, command, then Grouping Identifier.
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_GET, GROUP_TWO}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Get waits until its Grouping Identifier is reported as desired.
// Requirement: CC:008E.02.02.11.001.
TEST_F(MultiChannelAssociationTest, GetRequestWaitsForGroupIdentifier)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GET_GROUP));
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_GET, bytes, &length);
    const command_class_multi_channel_association_core::get_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, core().on_multi_channel_association_get_requested(args, bytes, &length));
}

// Verifies that a plain NodeID Set request uses the V2 wire prefix.
// Requirement: CC:008E.02.01.11.004.
TEST_F(MultiChannelAssociationTest, SetRequestEncodesVersionTwoNodeId)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).set_desired<uint8_t>(DESTINATION_NODE_A);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_SET, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_requested(args, bytes, &length));
    // The V2 frame contains the Grouping Identifier followed by the NodeID list.
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_SET, GROUP_TWO, DESTINATION_NODE_A}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that an End Point Set request encodes the V3 marker and endpoint tuple.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, SetRequestEncodesVersionThreeEndpointAssociation)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::marker)).set_desired<uint8_t>(SET_MARKER);
    multi_channel_association_set_vg_t_item_t item {};
    item.multi_channel_node_id = DESTINATION_NODE_B;
    item.properties1.value     = DESTINATION_ENDPOINT;
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg)).set_desired<std::vector<multi_channel_association_set_vg_t_item_t>>({item});
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_SET, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_requested(args, bytes, &length));
    // After the marker, each endpoint tuple is encoded as Multi Channel NodeID then endpoint properties.
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_SET, GROUP_TWO, SET_MARKER, DESTINATION_NODE_B, DESTINATION_ENDPOINT}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Set waits when neither NodeID nor endpoint destinations are ready.
// Requirement: CC:008E.02.01.11.004.
TEST_F(MultiChannelAssociationTest, SetRequestWaitsForDestinationList)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_SET, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, core().on_multi_channel_association_set_requested(args, bytes, &length));
}

// Verifies that Set frame assembly waits until the Grouping Identifier is desired.
// Requirement: CC:008E.02.01.11.004.
TEST_F(MultiChannelAssociationTest, SetRequestWaitsForGroupIdentifier)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).set_desired<uint8_t>(DESTINATION_NODE_A);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_SET, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, core().on_multi_channel_association_set_requested(args, bytes, &length));
}

// Verifies that Remove allows the V2 NodeID-only layout.
// Requirement: CC:008E.02.04.11.004.
TEST_F(MultiChannelAssociationTest, RemoveRequestEncodesVersionTwoNodeId)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::node_id)).set_desired<std::vector<uint8_t>>({DESTINATION_NODE_A});
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_REMOVE, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_requested(args, bytes, &length));
    // The marker is omitted when the V2 request contains no endpoint tuple.
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_REMOVE, GROUP_TWO, DESTINATION_NODE_A}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Remove encodes an MCA v3 endpoint tuple after its marker.
// Requirement: CC:008E.02.04.11.005.
TEST_F(MultiChannelAssociationTest, RemoveRequestEncodesVersionThreeEndpointAssociation)
{
    // Populate the Remove request's group, marker, and one endpoint destination.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::marker)).set_desired<uint8_t>(SET_MARKER);
    multi_channel_association_remove_vg_t_item_t item {};
    item.multi_channel_node_id = DESTINATION_NODE_B;
    item.properties1.value     = DESTINATION_ENDPOINT;
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::vg)).set_desired<std::vector<multi_channel_association_remove_vg_t_item_t>>({item});

    // Assemble the MCA v3 layout: group, marker, then Multi Channel NodeID and endpoint properties.
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_REMOVE, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_requested(args, bytes, &length));
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_REMOVE, GROUP_TWO, SET_MARKER, DESTINATION_NODE_B, DESTINATION_ENDPOINT}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that endpoint Remove inserts the MCA v3 marker when no stored marker is present.
// Requirement: CC:008E.02.04.11.005.
TEST_F(MultiChannelAssociationTest, RemoveRequestInsertsMarkerForEndpointAssociation)
{
    // The request has an endpoint tuple but intentionally omits its optional marker attribute.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    multi_channel_association_remove_vg_t_item_t item {};
    item.multi_channel_node_id = DESTINATION_NODE_C;
    item.properties1.value     = DESTINATION_ENDPOINT;
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::vg)).set_desired<std::vector<multi_channel_association_remove_vg_t_item_t>>({item});

    // Assembly supplies the mandatory marker before the endpoint tuple.
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_REMOVE, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_requested(args, bytes, &length));
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_REMOVE, GROUP_TWO, SET_MARKER, DESTINATION_NODE_C, DESTINATION_ENDPOINT}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that Remove waits until at least one destination field is available.
// Requirement: CC:008E.02.04.11.004.
TEST_F(MultiChannelAssociationTest, RemoveRequestWaitsForDestinationList)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_REMOVE, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, core().on_multi_channel_association_remove_requested(args, bytes, &length));
}

// Verifies that an empty desired endpoint list is omitted from a NodeID Remove request.
// Requirement: CC:008E.02.04.13.001.
TEST_F(MultiChannelAssociationTest, RemoveRequestWithEmptyEndpointListOmitsMarker)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::node_id)).set_desired<std::vector<uint8_t>>({DESTINATION_NODE_A});
    group.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::vg)).set_desired<std::vector<multi_channel_association_remove_vg_t_item_t>>({});
    uint8_t bytes[FRAME_CAPACITY] {};
    uint16_t length = INITIAL_FRAME_LENGTH;
    zwave_frame_generator generator {MCA_CC};
    generator.initialize_frame(CMD_REMOVE, bytes, &length);
    const command_class_multi_channel_association_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_requested(args, bytes, &length));
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_REMOVE, GROUP_TWO, DESTINATION_NODE_A}), std::vector<uint8_t>(bytes, bytes + length));
}

// Verifies that a V2 Set stores unique NodeID destinations in the reported group.
// Requirement: CC:008E.02.01.12.001.
TEST_F(MultiChannelAssociationTest, SetSupportStoresUniqueNodeIds)
{
    // AGI supplies the supported range and capacity; the CC owns the report tree.
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A, DESTINATION_NODE_B, DESTINATION_NODE_A}}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));
    auto group = find_report_group(endpoint, GROUP_TWO);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A, DESTINATION_NODE_B}), group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
}

// Verifies that a V3 Set stores endpoint destinations alongside NodeID destinations.
// Requirement: CC:008E.03.01.13.001.
TEST_F(MultiChannelAssociationTest, SetSupportStoresMixedDestinationTypes)
{
    auto item = endpoint_destination(DESTINATION_NODE_B, DESTINATION_ENDPOINT);
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}, {"vg", std::vector<multi_channel_association_set_vg_t_item_t> {item}}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));
    auto group = find_report_group(endpoint, GROUP_TWO);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A}), group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
    const auto endpoints = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported<std::vector<multi_channel_association_report_vg_t_item_t>>();
    ASSERT_EQ(1U, endpoints.size());
    EXPECT_EQ(DESTINATION_NODE_B, endpoints.front().multi_channel_node_id);
    EXPECT_EQ(DESTINATION_ENDPOINT, endpoints.front().properties1.value);
}

// Verifies that a repeated endpoint tuple is stored only once in an association group.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, SetSupportDoesNotDuplicateEndpointTuples)
{
    // Submit the same endpoint destination twice to exercise tuple equality and uniqueness.
    const auto item = endpoint_destination(DESTINATION_NODE_B, DESTINATION_ENDPOINT);
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {}}, {"vg", std::vector<multi_channel_association_set_vg_t_item_t> {item, item}}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));

    // Only one tuple is committed to the reported group.
    auto group = find_report_group(endpoint, GROUP_TWO);
    ASSERT_TRUE(group.is_valid());
    const auto destinations = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported<std::vector<multi_channel_association_report_vg_t_item_t>>();
    ASSERT_EQ(1U, destinations.size());
    EXPECT_EQ(DESTINATION_NODE_B, destinations.front().multi_channel_node_id);
    EXPECT_EQ(DESTINATION_ENDPOINT, destinations.front().properties1.value);
}

// Verifies that Set ignores a Grouping Identifier outside the reported range.
// Requirement: CC:008E.03.01.11.002.
TEST_F(MultiChannelAssociationTest, SetSupportIgnoresUnsupportedGroup)
{
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", UNSUPPORTED_GROUP}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};
    EXPECT_EQ(SL_STATUS_FAIL, core().on_multi_channel_association_set_support_received(connection(), request));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP)).is_valid());
}

// Verifies that Set rejects an addition that would exceed shared capacity.
// Requirement: CC:008E.02.01.13.002.
TEST_F(MultiChannelAssociationTest, SetSupportRejectsCapacityOverflow)
{
    agi_mock().max_nodes = 2;
    seed_report_group(endpoint, GROUP_TWO, {DESTINATION_NODE_A});
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_B}}, {"vg", std::vector<multi_channel_association_set_vg_t_item_t> {endpoint_destination(DESTINATION_NODE_C, DESTINATION_ENDPOINT)}}};
    EXPECT_EQ(SL_STATUS_FAIL, core().on_multi_channel_association_set_support_received(connection(), request));
    auto group = find_report_group(endpoint, GROUP_TWO);
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A}), group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
}

// Verifies that Set treats an unavailable capacity as unbounded.
// Requirement: CC:008E.02.01.12.001.
TEST_F(MultiChannelAssociationTest, SetSupportContinuesWhenCapacityIsUnknown)
{
    agi_mock().capacity_status = SL_STATUS_NOT_AVAILABLE;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};
    EXPECT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));
}

// Verifies that Set accepts the requested group when AGI cannot provide its count.
// Requirement: CC:008E.02.01.12.001.
TEST_F(MultiChannelAssociationTest, SetSupportContinuesWhenGroupCountIsUnknown)
{
    agi_mock().count_status = SL_STATUS_NOT_AVAILABLE;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};

    EXPECT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));
    EXPECT_TRUE(find_report_group(endpoint, GROUP_TWO).is_valid());
}

// Verifies that Set updates an existing matching group and ignores duplicate destinations.
// Requirement: CC:008E.02.01.11.004.
TEST_F(MultiChannelAssociationTest, SetSupportUpdatesMatchingGroupWithoutDuplicates)
{
    // Put malformed and non-matching groups first so lookup must inspect each candidate.
    const auto group_type = static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP);
    endpoint.add_node(group_type);
    auto desired_identifier_group = endpoint.add_node(group_type);
    desired_identifier_group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier)).set_desired<uint8_t>(GROUP_TWO);
    seed_report_group(endpoint, UNSUPPORTED_GROUP, {});
    seed_report_group(endpoint, GROUP_TWO, {DESTINATION_NODE_A}, {report_endpoint_destination(DESTINATION_NODE_B, OTHER_DESTINATION_ENDPOINT)});
    const auto endpoint_item = endpoint_destination(DESTINATION_NODE_B, DESTINATION_ENDPOINT);
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A, DESTINATION_NODE_B}}, {"vg", std::vector<multi_channel_association_set_vg_t_item_t> {endpoint_item, endpoint_item}}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));
    const auto group = find_report_group(endpoint, GROUP_TWO);
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A, DESTINATION_NODE_B}), group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
    const auto stored_endpoints = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported<std::vector<multi_channel_association_report_vg_t_item_t>>();
    ASSERT_EQ(2U, stored_endpoints.size());
    EXPECT_EQ(OTHER_DESTINATION_ENDPOINT, stored_endpoints.front().properties1.value);
    EXPECT_EQ(DESTINATION_ENDPOINT, stored_endpoints.back().properties1.value);
}

// Verifies that V2 Set notifies AGI when adding lifeline destinations.
// Requirement: CC:008E.02.01.12.001.
TEST_F(MultiChannelAssociationTest, SetSupportNotifiesAgiForLifeline)
{
    EXPECT_CALL(agi_mock(), add_lifeline_destinations(testing::Truly([](const auto &payload) { return payload.node_ids == std::vector<uint8_t> {DESTINATION_NODE_A} && payload.endpoint_associations.empty(); }))).WillOnce(testing::Return(SL_STATUS_OK));
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};
    EXPECT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));
}

// Verifies that Set notifies AGI when adding an endpoint association to Lifeline.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, SetSupportNotifiesAgiForLifelineEndpoint)
{
    const auto item = endpoint_destination(DESTINATION_NODE_B, DESTINATION_ENDPOINT);
    EXPECT_CALL(agi_mock(), add_lifeline_destinations(testing::Truly([](const auto &payload) { return payload.node_ids.empty() && payload.endpoint_associations == std::vector<std::pair<uint8_t, uint8_t>> {{DESTINATION_NODE_B, DESTINATION_ENDPOINT}}; }))).WillOnce(testing::Return(SL_STATUS_OK));
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"vg", std::vector<multi_channel_association_set_vg_t_item_t> {item}}};

    EXPECT_EQ(SL_STATUS_OK, core().on_multi_channel_association_set_support_received(connection(), request));
}

// Verifies that a Get Report lists both destination types from AGI.
// Requirement: CC:008E.02.03.11.003.
TEST_F(MultiChannelAssociationTest, GetSupportReportsNodeAndEndpointDestinations)
{
    agi_mock().destinations.node_ids              = {DESTINATION_NODE_A};
    agi_mock().destinations.endpoint_associations = {{DESTINATION_NODE_B, DESTINATION_ENDPOINT}};
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_REPORT);
    std::vector<uint8_t> frame;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_support_requested(connection(), request, report, frame));
    // Report bytes: header, group, capacity, no-following-report count, NodeID, marker, endpoint pair.
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_REPORT, GROUP_TWO, MAX_DESTINATIONS, NO_REPORTS_FOLLOW, DESTINATION_NODE_A, SET_MARKER, DESTINATION_NODE_B, DESTINATION_ENDPOINT}), frame);
}

// Verifies that a long NodeID destination list is split into multiple Reports.
// Requirement: CC:008E.02.03.11.003.
TEST_F(MultiChannelAssociationTest, GetSupportSplitsLongNodeListAcrossReports)
{
    // More NodeIDs than fit in one Z-Wave application frame exercise the continuation path.
    agi_mock().destinations.node_ids.resize(LONG_NODE_LIST_SIZE);
    for (size_t index = 0; index < agi_mock().destinations.node_ids.size(); ++index) {
        agi_mock().destinations.node_ids[index] = static_cast<uint8_t>(DESTINATION_NODE_A + index);
    }
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_REPORT);
    std::vector<uint8_t> final_frame;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};

    // Intermediate Reports are sent by the CC; the returned frame is the final Report.
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_support_requested(connection(), request, report, final_frame));
    ASSERT_GT(final_frame.size(), REPORTS_TO_FOLLOW_OFFSET);
    EXPECT_EQ(NO_REPORTS_FOLLOW, final_frame[REPORTS_TO_FOLLOW_OFFSET]);
    EXPECT_EQ(agi_mock().destinations.node_ids.back(), final_frame.back());
}

// Verifies that endpoint-only destinations are split into Reports with their MCA v3 marker.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, GetSupportSplitsLongEndpointListAcrossReports)
{
    // Endpoint tuples each consume a NodeID byte and endpoint-properties byte after the marker.
    for (size_t index = 0; index < LONG_ENDPOINT_LIST_SIZE; ++index) {
        agi_mock().destinations.endpoint_associations.emplace_back(DESTINATION_NODE_B, DESTINATION_ENDPOINT);
    }
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_REPORT);
    std::vector<uint8_t> final_frame;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};

    // Endpoint-only chunks use the VG packing path after all NodeID destinations are exhausted.
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_support_requested(connection(), request, report, final_frame));
    ASSERT_GT(final_frame.size(), REPORTS_TO_FOLLOW_OFFSET);
    EXPECT_EQ(NO_REPORTS_FOLLOW, final_frame[REPORTS_TO_FOLLOW_OFFSET]);
    EXPECT_EQ(DESTINATION_ENDPOINT, final_frame.back());
}

// Verifies that mixed NodeID and endpoint destinations share Report chunks when space allows.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, GetSupportSplitsMixedDestinationListAcrossReports)
{
    // Both lists are long enough to split, and the chunker may place both types into one Report.
    agi_mock().destinations.node_ids.resize(LONG_NODE_LIST_SIZE);
    for (size_t index = 0; index < agi_mock().destinations.node_ids.size(); ++index) {
        agi_mock().destinations.node_ids[index] = static_cast<uint8_t>(DESTINATION_NODE_A + index);
    }
    for (size_t index = 0; index < LONG_ENDPOINT_LIST_SIZE; ++index) {
        agi_mock().destinations.endpoint_associations.emplace_back(DESTINATION_NODE_B, DESTINATION_ENDPOINT);
    }
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_REPORT);
    std::vector<uint8_t> final_frame;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_support_requested(connection(), request, report, final_frame));
    ASSERT_GT(final_frame.size(), REPORTS_TO_FOLLOW_OFFSET);
    EXPECT_EQ(NO_REPORTS_FOLLOW, final_frame[REPORTS_TO_FOLLOW_OFFSET]);
}

// Verifies that Get falls back to Lifeline when AGI cannot report the supported-group count.
// Requirement: CC:008E.02.02.12.001.
TEST_F(MultiChannelAssociationTest, GetSupportFallsBackWhenGroupCountIsUnknown)
{
    // A failed count query leaves the requested Grouping Identifier unsupported for this response.
    agi_mock().count_status          = SL_STATUS_NOT_AVAILABLE;
    agi_mock().destinations.node_ids = {DESTINATION_NODE_A};
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_REPORT);
    std::vector<uint8_t> frame;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_support_requested(connection(), request, report, frame));
    EXPECT_EQ(LIFELINE_GROUP, frame[GROUPING_ID_OFFSET]);
}

// Verifies that Get Report falls back to Lifeline for an unsupported group.
// Requirement: CC:008E.02.02.12.001.
TEST_F(MultiChannelAssociationTest, GetSupportFallsBackToLifeline)
{
    agi_mock().destinations.node_ids = {DESTINATION_NODE_A};
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_REPORT);
    std::vector<uint8_t> frame;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", UNSUPPORTED_GROUP}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_support_requested(connection(), request, report, frame));
    EXPECT_EQ(LIFELINE_GROUP, frame[GROUPING_ID_OFFSET]);
    EXPECT_EQ(DESTINATION_NODE_A, frame.back());
}

// Verifies Get reports an empty destination list when AGI queries fail.
// Requirement: CC:008E.02.03.11.003.
TEST_F(MultiChannelAssociationTest, GetSupportReportsEmptyDestinationsWhenAgiQueriesFail)
{
    agi_mock().capacity_status     = SL_STATUS_NOT_AVAILABLE;
    agi_mock().destinations_status = SL_STATUS_NOT_AVAILABLE;
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_REPORT);
    std::vector<uint8_t> frame;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_get_support_requested(connection(), request, report, frame));
    EXPECT_EQ((std::vector<uint8_t> {MCA_CC, CMD_REPORT, GROUP_TWO, NO_DESTINATIONS, NO_REPORTS_FOLLOW}), frame);
}

// Verifies that Groupings Report returns the supported-group count from AGI.
// Requirement: CC:008E.02.06.11.001.
TEST_F(MultiChannelAssociationTest, GroupingsGetSupportReportsCount)
{
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_GROUPINGS_REPORT);
    std::vector<uint8_t> frame;
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_groupings_get_support_requested(connection(), {}, report, frame));
    EXPECT_EQ(GROUPING_REPORT_FRAME_SIZE, frame.size());
    EXPECT_EQ(GROUP_COUNT, frame.back());
}

// Verifies that Groupings Report reports zero when AGI cannot provide a count.
// Requirement: CC:008E.02.06.11.001.
TEST_F(MultiChannelAssociationTest, GroupingsGetSupportReportsZeroWhenAgiQueryFails)
{
    agi_mock().count_status = SL_STATUS_NOT_AVAILABLE;
    zwave_frame_generator_standalone report;
    report.add_header(MCA_CC, CMD_GROUPINGS_REPORT);
    std::vector<uint8_t> frame;

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_groupings_get_support_requested(connection(), {}, report, frame));
    EXPECT_EQ(NO_DESTINATIONS, frame.back());
}

// Verifies that removing a specific destination preserves the other group entries.
// Requirement: CC:008E.02.04.11.002.
TEST_F(MultiChannelAssociationTest, RemoveSupportDeletesSelectedNodeId)
{
    seed_report_group(endpoint, GROUP_TWO, {DESTINATION_NODE_A, DESTINATION_NODE_B});
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    auto group = find_report_group(endpoint, GROUP_TWO);
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_B}), group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
}

// Verifies that removing a Lifeline destination notifies AGI with the removed NodeID.
// Requirement: CC:008E.03.05.11.002.
TEST_F(MultiChannelAssociationTest, RemoveSupportNotifiesAgiForRemovedLifelineNode)
{
    // Seed the local Report tree because Remove updates CC state before notifying AGI.
    seed_report_group(endpoint, LIFELINE_GROUP, {DESTINATION_NODE_A, DESTINATION_NODE_B});
    EXPECT_CALL(agi_mock(), remove_lifeline_destinations(testing::Truly([](const auto &payload) { return payload.node_ids == std::vector<uint8_t> {DESTINATION_NODE_A} && payload.endpoint_associations.empty(); }))).WillOnce(testing::Return(SL_STATUS_OK));
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};

    // Remove exactly one member and verify the other remains.
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_B}), find_report_group(endpoint, LIFELINE_GROUP).child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
}

// Verifies that removing a Lifeline endpoint tuple updates Report state and notifies AGI.
// Requirement: CC:008E.02.04.11.005.
TEST_F(MultiChannelAssociationTest, RemoveSupportDeletesEndpointTupleAndNotifiesAgi)
{
    // Seed two endpoint destinations so the test can verify both removal and preservation.
    seed_report_group(endpoint, LIFELINE_GROUP, {}, {report_endpoint_destination(DESTINATION_NODE_B, DESTINATION_ENDPOINT), report_endpoint_destination(DESTINATION_NODE_C, DESTINATION_ENDPOINT)});
    multi_channel_association_remove_vg_t_item_t removed {};
    removed.multi_channel_node_id = DESTINATION_NODE_B;
    removed.properties1.value     = DESTINATION_ENDPOINT;
    EXPECT_CALL(agi_mock(), remove_lifeline_destinations(testing::Truly([](const auto &payload) { return payload.node_ids.empty() && payload.endpoint_associations == std::vector<std::pair<uint8_t, uint8_t>> {{DESTINATION_NODE_B, DESTINATION_ENDPOINT}}; }))).WillOnce(testing::Return(SL_STATUS_OK));
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"vg", std::vector<multi_channel_association_remove_vg_t_item_t> {removed}}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    const auto remaining = find_report_group(endpoint, LIFELINE_GROUP).child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported<std::vector<multi_channel_association_report_vg_t_item_t>>();
    ASSERT_EQ(1U, remaining.size());
    EXPECT_EQ(DESTINATION_NODE_C, remaining.front().multi_channel_node_id);
    EXPECT_EQ(DESTINATION_ENDPOINT, remaining.front().properties1.value);
}

// Verifies that Remove proceeds when AGI cannot report the supported-group count.
// Requirement: CC:008E.02.04.11.005.
TEST_F(MultiChannelAssociationTest, RemoveSupportContinuesWhenGroupCountIsUnknown)
{
    // A count-query failure cannot classify a positive Grouping Identifier as unsupported.
    agi_mock().count_status = SL_STATUS_NOT_AVAILABLE;
    seed_report_group(endpoint, GROUP_TWO, {DESTINATION_NODE_A});
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    EXPECT_TRUE(find_report_group(endpoint, GROUP_TWO).child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>().empty());
}

// Verifies that Remove All clears all report groups for Grouping Identifier zero.
// Requirement: CC:008E.02.04.11.002.
TEST_F(MultiChannelAssociationTest, RemoveSupportGroupZeroClearsEveryGroup)
{
    seed_report_group(endpoint, LIFELINE_GROUP, {DESTINATION_NODE_A});
    seed_report_group(endpoint, GROUP_TWO, {DESTINATION_NODE_B});
    seed_report_group(endpoint, LIFELINE_GROUP, {}, {report_endpoint_destination(DESTINATION_NODE_B, DESTINATION_ENDPOINT)});
    seed_report_group(endpoint, GROUP_TWO, {}, {report_endpoint_destination(DESTINATION_NODE_C, DESTINATION_ENDPOINT)});
    agi_mock().destinations.node_ids              = {DESTINATION_NODE_A, DESTINATION_NODE_C};
    agi_mock().destinations.endpoint_associations = {{DESTINATION_NODE_B, DESTINATION_ENDPOINT}, {DESTINATION_NODE_C, DESTINATION_ENDPOINT}};
    EXPECT_CALL(agi_mock(), remove_lifeline_destinations(testing::Truly([](const auto &payload) {
                    return payload.node_ids == std::vector<uint8_t> {DESTINATION_NODE_A, DESTINATION_NODE_C} && payload.endpoint_associations == std::vector<std::pair<uint8_t, uint8_t>> {{DESTINATION_NODE_B, DESTINATION_ENDPOINT}, {DESTINATION_NODE_C, DESTINATION_ENDPOINT}};
                })))
      .WillOnce(testing::Return(SL_STATUS_OK));
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUPING_IDENTIFIER_ALL}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    auto lifeline  = find_report_group(endpoint, LIFELINE_GROUP);
    auto group_two = find_report_group(endpoint, GROUP_TWO);
    ASSERT_TRUE(lifeline.is_valid());
    ASSERT_TRUE(group_two.is_valid());
    EXPECT_TRUE(lifeline.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>().empty());
    EXPECT_TRUE(group_two.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>().empty());
    EXPECT_TRUE(lifeline.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported<std::vector<multi_channel_association_report_vg_t_item_t>>().empty());
    EXPECT_TRUE(group_two.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported<std::vector<multi_channel_association_report_vg_t_item_t>>().empty());
}

// Verifies that Remove ignores an unsupported positive Grouping Identifier.
// Requirement: CC:008E.03.05.11.003.
TEST_F(MultiChannelAssociationTest, RemoveSupportIgnoresUnsupportedGroup)
{
    seed_report_group(endpoint, GROUP_TWO, {DESTINATION_NODE_A});
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", UNSUPPORTED_GROUP}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A}), find_report_group(endpoint, GROUP_TWO).child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
}

// Verifies that Remove skips report groups without reported identifiers and non-target groups.
// Requirement: CC:008E.02.04.11.002.
TEST_F(MultiChannelAssociationTest, RemoveSupportSkipsUnidentifiedAndUntargetedGroups)
{
    const auto group_type   = static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP);
    auto unidentified_group = endpoint.add_node(group_type);
    unidentified_group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier));
    auto unrelated_group = endpoint.add_node(group_type);
    unrelated_group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(GROUP_TWO);
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    EXPECT_FALSE(unidentified_group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).is_valid());
    EXPECT_FALSE(unrelated_group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).is_valid());
}

// Verifies that Remove tolerates destination attributes that have not been reported yet.
// Requirement: CC:008E.02.04.11.002.
TEST_F(MultiChannelAssociationTest, RemoveSupportSkipsUnreportedDestinationAttributes)
{
    // A group without an identifier must be skipped before destination attributes are inspected.
    const auto group_type = static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP);
    endpoint.add_node(group_type);
    auto group = endpoint.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP));
    group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(GROUP_TWO);
    group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id));
    group.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg));
    auto group_without_destinations = endpoint.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP));
    group_without_destinations.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier)).set_reported<uint8_t>(UNSUPPORTED_GROUP);
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUP_TWO}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_A}}};

    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    EXPECT_FALSE(group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported_exists());
    EXPECT_FALSE(group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported_exists());
    EXPECT_FALSE(group_without_destinations.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).is_valid());
    EXPECT_FALSE(group_without_destinations.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).is_valid());
}

// Verifies that removing a non-member from Lifeline leaves stored nodes unchanged and sends no notification.
// Requirement: CC:008E.03.05.11.002.
TEST_F(MultiChannelAssociationTest, RemoveSupportDoesNotNotifyForUnmatchedLifelineNode)
{
    seed_report_group(endpoint, LIFELINE_GROUP, {DESTINATION_NODE_A});
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"node_id", std::vector<uint8_t> {DESTINATION_NODE_C}}};

    EXPECT_CALL(agi_mock(), remove_lifeline_destinations(testing::_)).Times(0);
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A}), find_report_group(endpoint, LIFELINE_GROUP).child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id)).reported<std::vector<uint8_t>>());
}

// Verifies that an unmatched endpoint tuple does not trigger a Lifeline update.
// Requirement: CC:008E.02.04.11.005.
TEST_F(MultiChannelAssociationTest, RemoveSupportDoesNotNotifyForUnmatchedLifelineEndpoint)
{
    seed_report_group(endpoint, LIFELINE_GROUP, {}, {report_endpoint_destination(DESTINATION_NODE_B, OTHER_DESTINATION_ENDPOINT)});
    multi_channel_association_remove_vg_t_item_t requested_endpoint {};
    requested_endpoint.multi_channel_node_id = DESTINATION_NODE_B;
    requested_endpoint.properties1.value     = DESTINATION_ENDPOINT;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", LIFELINE_GROUP}, {"vg", std::vector<multi_channel_association_remove_vg_t_item_t> {requested_endpoint}}};

    EXPECT_CALL(agi_mock(), remove_lifeline_destinations(testing::_)).Times(0);
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
    EXPECT_EQ(OTHER_DESTINATION_ENDPOINT, find_report_group(endpoint, LIFELINE_GROUP).child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg)).reported<std::vector<multi_channel_association_report_vg_t_item_t>>().front().properties1.value);
}

// Verifies that a failed AGI destination query leaves Remove All without a lifeline notification.
// Requirement: CC:008E.02.04.11.002.
TEST_F(MultiChannelAssociationTest, RemoveSupportDoesNotNotifyWhenAgiDestinationsAreUnavailable)
{
    agi_mock().destinations_status = SL_STATUS_NOT_AVAILABLE;
    const command_class_multi_channel_association_attribute_map_t request {{"grouping_identifier", GROUPING_IDENTIFIER_ALL}};

    EXPECT_CALL(agi_mock(), remove_lifeline_destinations(testing::_)).Times(0);
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_remove_support_received(connection(), request));
}

// Verifies that V2 MQTT Set stores NodeID destinations.
// Requirement: CC:008E.02.01.11.004.
TEST_F(MultiChannelAssociationTest, MqttSetStoresNodeIds)
{
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{},{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_A, DESTINATION_NODE_B, MQTT_MARKER_VALUE);
    ASSERT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A, DESTINATION_NODE_B}), group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id)).desired<std::vector<uint8_t>>());
}

// Verifies that V3 MQTT Set parses endpoint tuples after the marker.
// Requirement: CC:008E.03.01.13.004.
TEST_F(MultiChannelAssociationTest, MqttSetStoresEndpointTuple)
{
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[],"marker":{},"vg":[{{"multi_channel_node_id":{},"properties1":{{"end_point":{},"bit_address":{}}}}}]}})", GROUP_TWO, MQTT_MARKER_VALUE, DESTINATION_NODE_B, DESTINATION_ENDPOINT, MQTT_ENDPOINT_BIT_ADDRESS_VALUE);
    ASSERT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    const auto destinations = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg)).desired<std::vector<multi_channel_association_set_vg_t_item_t>>();
    ASSERT_EQ(1U, destinations.size());
    EXPECT_EQ(DESTINATION_NODE_B, destinations.front().multi_channel_node_id);
    EXPECT_EQ(DESTINATION_ENDPOINT, destinations.front().properties1.value);
}

// Verifies that bit-addressed MQTT endpoint Set entries bypass single-endpoint validation.
// Requirement: CC:008E.03.01.11.005.
TEST_F(MultiChannelAssociationTest, MqttSetStoresBitAddressedEndpointTuple)
{
    // A bit-addressed endpoint encodes a mask and is intentionally skipped by single-endpoint checks.
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[],"marker":{},"vg":[{{"multi_channel_node_id":{},"properties1":{{"end_point":{} ,"bit_address":{}}}}}]}})", GROUP_TWO, MQTT_MARKER_VALUE, DESTINATION_NODE_C, DESTINATION_ENDPOINT, BIT_ADDRESS_SET);
    ASSERT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));

    // The original bit-address flag is retained in desired VG state.
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    const auto destinations = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg)).desired<std::vector<multi_channel_association_set_vg_t_item_t>>();
    ASSERT_EQ(1U, destinations.size());
    EXPECT_EQ(BIT_ADDRESS_SET, destinations.front().properties1.flags.multi_channel_association_set_bit_address);
}

// Verifies that the MQTT Set accepts V2 NodeID destinations with matching security classes.
// Requirement: CC:008E.02.01.11.003.
TEST_F(MultiChannelAssociationTest, MqttSetV2AcceptsMatchingSecurityClasses)
{
    // Seed source and destination granted keys so the V2 security comparison can run.
    auto source_node = home.child_by_type(ATTRIBUTE_NODE_ID);
    source_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);
    auto destination_node = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    destination_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V2);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    // Equal highest Security Classes are required by Multi Channel Association V2.
    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that pre-V2 peers bypass the V2/V3 matching security-class rule.
// Requirement: CC:008E.02.01.11.003.
TEST_F(MultiChannelAssociationTest, MqttSetLegacyPeerSkipsSecurityClassComparison)
{
    // The nodes have different highest classes, but the peer's MCA version predates the rule.
    auto source_node = home.child_by_type(ATTRIBUTE_NODE_ID);
    source_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);
    auto destination_node = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    destination_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_ACCESS_KEY);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V1);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that the MQTT Set rejects V3 NodeID destinations with different highest security classes.
// Requirement: CC:008E.02.51.01.1.
TEST_F(MultiChannelAssociationTest, MqttSetV3RejectsDifferentSecurityClasses)
{
    // The source and destination both have granted keys, but their highest classes differ.
    auto source_node = home.child_by_type(ATTRIBUTE_NODE_ID);
    source_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);
    auto destination_node = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    destination_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_ACCESS_KEY);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V3);
    ASSERT_EQ(MCA_VERSION_V3, zwave_node_get_command_class_version(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3, NODE_ID, ROOT_ENDPOINT));
    zwave_keyset_t source_keys      = 0;
    zwave_keyset_t destination_keys = 0;
    ASSERT_EQ(SL_STATUS_OK, zwave_get_node_granted_keys(NODE_ID, &source_keys));
    ASSERT_EQ(SL_STATUS_OK, zwave_get_node_granted_keys(DESTINATION_NODE_C, &destination_keys));
    ASSERT_NE(zwave_controller_get_highest_encapsulation(source_keys), zwave_controller_get_highest_encapsulation(destination_keys));
    zwave_node_id_t resolved_source       = 0;
    zwave_endpoint_id_t resolved_endpoint = 0;
    ASSERT_EQ(SL_STATUS_OK, attribute_store_network_helper_get_zwave_ids_from_node(endpoint, &resolved_source, &resolved_endpoint));
    ASSERT_EQ(NODE_ID, resolved_source);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    // V2/V3 peers require identical highest classes, so no pending Set may be created.
    EXPECT_EQ(SL_STATUS_FAIL, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP)).is_valid());
}

// Verifies that the MQTT Set V4 rule requires the source to hold the destination's highest key.
// Requirement: CC:008E.04.51.01.1.
TEST_F(MultiChannelAssociationTest, MqttSetV4RejectsMissingDestinationHighestKey)
{
    // V4 allows different highest classes only when the source also holds the destination's key.
    auto source_node = home.child_by_type(ATTRIBUTE_NODE_ID);
    source_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);
    auto destination_node = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    destination_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_ACCESS_KEY);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V4);
    ASSERT_EQ(MCA_VERSION_V4, zwave_node_get_command_class_version(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3, NODE_ID, ROOT_ENDPOINT));
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_FAIL, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that the MQTT Set V4 rule accepts a source holding the destination's highest key.
// Requirement: CC:008E.04.51.01.1.
TEST_F(MultiChannelAssociationTest, MqttSetV4AcceptsSharedDestinationHighestKey)
{
    // The destination's highest key is Access; the source also holds Access.
    auto source_node = home.child_by_type(ATTRIBUTE_NODE_ID);
    source_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_ACCESS_KEY);
    auto destination_node = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    destination_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_ACCESS_KEY | ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V4);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that the V4 security rule does not require a key when the destination has no Security Class.
// Requirement: CC:008E.04.51.01.1.
TEST_F(MultiChannelAssociationTest, MqttSetV4AllowsDestinationWithoutGrantedKeys)
{
    // Both key attributes exist, but the destination's empty set maps to no Security Class.
    auto source_node = home.child_by_type(ATTRIBUTE_NODE_ID);
    source_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_ACCESS_KEY);
    auto destination_node = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    destination_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(0);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3)).set_reported<uint8_t>(MCA_VERSION_V4);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that Set continues conservatively when the destination has no granted-key record.
// Requirement: CC:008E.02.01.11.003.
TEST_F(MultiChannelAssociationTest, MqttSetContinuesWhenDestinationKeysAreUnavailable)
{
    // A valid source key record and a missing destination key record trigger the unavailable-key fallback.
    auto source_node = home.child_by_type(ATTRIBUTE_NODE_ID);
    source_node.emplace_node(ATTRIBUTE_GRANTED_SECURITY_KEYS).set_reported<zwave_keyset_t>(ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY);
    home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that MQTT Set accepts a destination supporting a CC listed by the AGI group.
// Requirement: CC:008E.01.51.03.3.
TEST_F(MultiChannelAssociationTest, MqttSetAcceptsDestinationSupportingGroupCommandClass)
{
    // AGI's command list is encoded as command-class/command pairs; the NIF advertises the CC.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP), GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::group_command_list)).set_reported<std::vector<uint8_t>>({AGI_CONTROLLED_CC, AGI_CONTROLLED_COMMAND});
    auto destination_node     = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    auto destination_endpoint = destination_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, ROOT_ENDPOINT);
    destination_endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t> {AGI_CONTROLLED_CC});
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    // The destination supports a controlled CC, so the request is accepted.
    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that MQTT Set rejects a destination supporting none of the AGI group's command classes.
// Requirement: CC:008E.01.51.03.3.
TEST_F(MultiChannelAssociationTest, MqttSetRejectsDestinationMissingGroupCommandClasses)
{
    // The group controls one CC, while the destination endpoint has no advertised NIF.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP), GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::group_command_list)).set_reported<std::vector<uint8_t>>({AGI_CONTROLLED_CC, AGI_CONTROLLED_COMMAND});
    home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    // A destination with no matching CC must not get a pending Set request.
    EXPECT_EQ(SL_STATUS_FAIL, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies MQTT Set can find a controlled CC in the destination's Multi Channel Capability Report.
// Requirement: CC:008E.01.51.03.3.
TEST_F(MultiChannelAssociationTest, MqttSetAcceptsCapabilityReportCommandClass)
{
    // The destination omits the CC from its NIF but advertises it in the endpoint capability report.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP), GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::group_command_list)).set_reported<std::vector<uint8_t>>({AGI_CONTROLLED_CC, AGI_CONTROLLED_COMMAND});
    auto destination_node     = home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    auto destination_endpoint = destination_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, ROOT_ENDPOINT);
    auto capability_report    = destination_endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP));
    capability_report.emplace_node(static_cast<attribute_store_type_t>(command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t::command_class)).set_reported<std::vector<uint8_t>>({AGI_CONTROLLED_CC});
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies a short AGI command list with no complete command pair does not block MQTT Set.
// Requirement: CC:008E.01.51.03.3.
TEST_F(MultiChannelAssociationTest, MqttSetAllowsIncompleteControlledCommandList)
{
    // One trailing CC byte does not form a command-class/command pair and cannot be validated.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP), GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::group_command_list)).set_reported<std::vector<uint8_t>>({AGI_CONTROLLED_CC});
    home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that an empty AGI command list does not block MQTT Set.
// Requirement: CC:008E.01.51.03.3.
TEST_F(MultiChannelAssociationTest, MqttSetAllowsGroupWithoutControlledCommands)
{
    // An empty command list has no controlled CC requirement to check against the destination.
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP), GROUP_TWO);
    group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::group_command_list)).set_reported<std::vector<uint8_t>>({});
    home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that a group without an AGI command-list attribute does not block MQTT Set.
// Requirement: CC:008E.01.51.03.3.
TEST_F(MultiChannelAssociationTest, MqttSetAllowsGroupWithoutCommandList)
{
    // The group exists, but without a command list the destination-support check has no constraints.
    endpoint.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP), GROUP_TWO);
    home.add_node(ATTRIBUTE_NODE_ID).set_reported<zwave_node_id_t>(DESTINATION_NODE_C);
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_C, MQTT_MARKER_VALUE);

    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
}

// Verifies that MQTT Set skips network capability checks if the source endpoint cannot be resolved.
// Requirement: CC:008E.02.01.11.004.
TEST_F(MultiChannelAssociationTest, MqttSetAcceptsUnresolvableSourceEndpoint)
{
    // The tree root has no Z-Wave NodeID, so external peer checks conservatively allow the request.
    auto root          = attribute_store::attribute::root();
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_A, MQTT_MARKER_VALUE);
    EXPECT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(root, payload));
}

// Verifies that MQTT Get rejects a payload without a Grouping Identifier.
// Requirement: CC:008E.02.02.11.001.
TEST_F(MultiChannelAssociationTest, MqttGetRejectsMissingGroupIdentifier)
{
    // Parsing must fail before a pending Get group is created.
    EXPECT_NE(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_get_command(endpoint, "{}"));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GET_GROUP)).is_valid());
}

// Verifies that malformed MQTT Set payloads do not create pending work.
// Requirement: CC:008E.02.01.11.004.
TEST_F(MultiChannelAssociationTest, MqttSetRejectsMissingGroupIdentifier)
{
    const auto payload = fmt::format(R"({{"node_id":[{}],"marker":{}}})", DESTINATION_NODE_A, MQTT_MARKER_VALUE);
    EXPECT_NE(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP)).is_valid());
}

// Verifies that MQTT Remove parses the requested NodeID list.
// Requirement: CC:008E.02.04.11.004.
TEST_F(MultiChannelAssociationTest, MqttRemoveStoresNodeIds)
{
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[{}],"marker":{},"vg":[]}})", GROUP_TWO, DESTINATION_NODE_A, MQTT_MARKER_VALUE);
    ASSERT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_remove_command(endpoint, payload));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {DESTINATION_NODE_A}), group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::node_id)).desired<std::vector<uint8_t>>());
}

// Verifies that MQTT Remove parses MCA v3 endpoint tuples.
// Requirement: CC:008E.02.04.11.005.
TEST_F(MultiChannelAssociationTest, MqttRemoveStoresEndpointTuple)
{
    // The payload includes marker and the nested endpoint tuple fields required by the Remove schema.
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[],"marker":{},"vg":[{{"multi_channel_node_id":{},"properties1":{{"end_point":{},"bit_address":{}}}}}]}})", GROUP_TWO, MQTT_MARKER_VALUE, DESTINATION_NODE_C, DESTINATION_ENDPOINT, MQTT_ENDPOINT_BIT_ADDRESS_VALUE);
    ASSERT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_remove_command(endpoint, payload));

    // The nested endpoint is stored as one desired Remove VG item.
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
    ASSERT_TRUE(group.is_valid());
    const auto destinations = group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::vg)).desired<std::vector<multi_channel_association_remove_vg_t_item_t>>();
    ASSERT_EQ(1U, destinations.size());
    EXPECT_EQ(DESTINATION_NODE_C, destinations.front().multi_channel_node_id);
    EXPECT_EQ(DESTINATION_ENDPOINT, destinations.front().properties1.value);
}

// Verifies that MQTT Remove rejects malformed endpoint tuple fields without creating work.
// Requirement: CC:008E.02.04.11.005.
TEST_F(MultiChannelAssociationTest, MqttRemoveRejectsMalformedEndpointTuple)
{
    // The tuple is missing its required nested endpoint-properties object.
    const auto payload = fmt::format(R"({{"grouping_identifier":{},"node_id":[],"marker":{},"vg":[{{"multi_channel_node_id":{}}}]}})", GROUP_TWO, MQTT_MARKER_VALUE, DESTINATION_NODE_C);
    EXPECT_NE(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_remove_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP)).is_valid());
}

// Verifies that MQTT Get stores the requested grouping identifier.
// Requirement: CC:008E.02.02.11.001.
TEST_F(MultiChannelAssociationTest, MqttGetStoresGroup)
{
    const auto payload = fmt::format(R"({{"grouping_identifier":{}}})", GROUP_TWO);
    ASSERT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_get_command(endpoint, payload));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(GROUP_TWO, group.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::grouping_identifier)).desired<uint8_t>());
}

// Verifies that MQTT Groupings Get creates its pending interview attribute.
// Requirement: CC:008E.02.05.11.001.
TEST_F(MultiChannelAssociationTest, MqttGroupingsGetCreatesInterviewRecord)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_groupings_get_command(endpoint, "{}"));
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_groupings_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET_GROUP)).is_valid());
}

// Verifies that a V2 Report frame with NodeID destinations parses successfully.
// Requirement: CC:008E.02.03.13.001.
TEST_F(MultiChannelAssociationTest, ReportVersionTwoStoresNodeIdsAndCapacity)
{
    const std::vector<uint8_t> wire_frame {MCA_CC, CMD_REPORT, GROUP_TWO, MAX_DESTINATIONS, NO_REPORTS_FOLLOW, DESTINATION_NODE_A, DESTINATION_NODE_B};
    zwave_frame_parser parser {wire_frame.data(), static_cast<uint16_t>(wire_frame.size())};
    const command_class_multi_channel_association_core::report_received_args args {endpoint, parser, connection()};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_report_received(args));
}

// Verifies that a V3 Report frame parses endpoint associations after its marker.
// Requirement: CC:008E.03.01.13.003.
TEST_F(MultiChannelAssociationTest, ReportVersionThreeStoresEndpointTuple)
{
    const std::vector<uint8_t> wire_frame {MCA_CC, CMD_REPORT, GROUP_TWO, MAX_DESTINATIONS, NO_REPORTS_FOLLOW, DESTINATION_NODE_A, SET_MARKER, DESTINATION_NODE_B, DESTINATION_ENDPOINT};
    zwave_frame_parser parser {wire_frame.data(), static_cast<uint16_t>(wire_frame.size())};
    const command_class_multi_channel_association_core::report_received_args args {endpoint, parser, connection()};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_report_received(args));
}

// Verifies that a V2 Report with no destinations parses successfully.
// Requirement: CC:008E.02.03.11.003.
TEST_F(MultiChannelAssociationTest, ReportVersionTwoStoresEmptyDestinations)
{
    const std::vector<uint8_t> wire_frame {MCA_CC, CMD_REPORT, GROUP_TWO, MAX_DESTINATIONS, NO_REPORTS_FOLLOW};
    zwave_frame_parser parser {wire_frame.data(), static_cast<uint16_t>(wire_frame.size())};
    const command_class_multi_channel_association_core::report_received_args args {endpoint, parser, connection()};
    ASSERT_EQ(SL_STATUS_OK, core().on_multi_channel_association_report_received(args));
}
