/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#ifndef MOCK_AGI_CONNECTOR_HPP
#define MOCK_AGI_CONNECTOR_HPP

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gmock/gmock.h>

#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "component_connector.hpp"

namespace zwave_command_class::test
{
    namespace agi = command_class_association_grp_info_types;

    constexpr uint8_t MOCK_AGI_SUPPORTED_GROUPINGS = 4;
    constexpr uint8_t MOCK_AGI_MAX_NODES_SUPPORTED = 10;
    constexpr auto MOCK_AGI_NOTIFICATION_TIMEOUT   = std::chrono::seconds(1);

    class MockAgiConnector
    {
        public:
            MOCK_METHOD(sl_status_t, get_supported_groupings, (const agi::component_connector_agi_empty_payload_t &, uint8_t &));
            MOCK_METHOD(sl_status_t, get_max_nodes, (const agi::component_connector_agi_group_max_nodes_query_t &, uint8_t &));
            MOCK_METHOD(sl_status_t, get_group_destinations, (const agi::component_connector_agi_group_destinations_query_t &, agi::component_connector_agi_group_destinations_t &));
            MOCK_METHOD(sl_status_t, add_lifeline_nodes, (const agi::component_connector_agi_lifeline_update_payload_t &));
            MOCK_METHOD(sl_status_t, remove_lifeline_nodes, (const agi::component_connector_agi_lifeline_update_payload_t &));

            sl_status_t count_status        = SL_STATUS_OK;
            uint8_t supported_groupings     = MOCK_AGI_SUPPORTED_GROUPINGS;
            sl_status_t capacity_status     = SL_STATUS_OK;
            uint8_t max_nodes               = MOCK_AGI_MAX_NODES_SUPPORTED;
            sl_status_t destinations_status = SL_STATUS_OK;
            agi::component_connector_agi_group_destinations_t destinations;
            // fire_event() is fire-and-forget, so tests wait until a notification callback has arrived.
            std::mutex notification_mutex;
            std::condition_variable notification_condition;
            size_t notification_count = 0;

            void connect_events(component_connector &event_registry)
            {
                event_registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_empty_payload_t, uint8_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT,
                                                                                                                                                 [this](const auto &payload, uint8_t &result) { return get_supported_groupings(payload, result); });
                event_registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_group_max_nodes_query_t, uint8_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES,
                                                                                                                                                         [this](const auto &payload, uint8_t &result) { return get_max_nodes(payload, result); });
                event_registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_group_destinations_query_t, agi::component_connector_agi_group_destinations_t>(
                  command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS,
                  [this](const auto &payload, auto &result) { return get_group_destinations(payload, result); });
                event_registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_lifeline_update_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE,
                                                                                                                                                  [this](const auto &payload) { return add_lifeline_nodes(payload); });
                event_registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_lifeline_update_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE,
                                                                                                                                                  [this](const auto &payload) { return remove_lifeline_nodes(payload); });
            }

            void reset_defaults()
            {
                // Clear per-test interaction expectations before restoring ordinary AGI responses.
                testing::Mock::VerifyAndClear(this);
                count_status        = SL_STATUS_OK;
                supported_groupings = MOCK_AGI_SUPPORTED_GROUPINGS;
                capacity_status     = SL_STATUS_OK;
                max_nodes           = MOCK_AGI_MAX_NODES_SUPPORTED;
                destinations_status = SL_STATUS_OK;
                destinations        = {};
                {
                    std::lock_guard<std::mutex> lock(notification_mutex);
                    notification_count = 0;
                }

                // Default responses model the peer boundary; tests add EXPECT_CALL only when a specific interaction matters.
                ON_CALL(*this, get_supported_groupings(testing::_, testing::_)).WillByDefault([this](const auto &, uint8_t &result) {
                    result = supported_groupings;
                    return count_status;
                });
                ON_CALL(*this, get_max_nodes(testing::_, testing::_)).WillByDefault([this](const auto &, uint8_t &result) {
                    result = max_nodes;
                    return capacity_status;
                });
                ON_CALL(*this, get_group_destinations(testing::_, testing::_)).WillByDefault([this](const auto &, auto &result) {
                    result = destinations;
                    return destinations_status;
                });
                ON_CALL(*this, add_lifeline_nodes(testing::_)).WillByDefault([this](const auto &) { return record_notification(); });
                ON_CALL(*this, remove_lifeline_nodes(testing::_)).WillByDefault([this](const auto &) { return record_notification(); });
            }

            bool wait_for_notification(size_t expected_count = 1)
            {
                std::unique_lock<std::mutex> lock(notification_mutex);
                return notification_condition.wait_for(lock, MOCK_AGI_NOTIFICATION_TIMEOUT, [this, expected_count] { return notification_count >= expected_count; });
            }

        private:
            sl_status_t record_notification()
            {
                {
                    std::lock_guard<std::mutex> lock(notification_mutex);
                    ++notification_count;
                }
                notification_condition.notify_one();
                return SL_STATUS_OK;
            }
    };
}  // namespace zwave_command_class::test

#endif  // MOCK_AGI_CONNECTOR_HPP
