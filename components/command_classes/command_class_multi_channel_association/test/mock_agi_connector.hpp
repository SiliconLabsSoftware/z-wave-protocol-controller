/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************/
#ifndef MOCK_MULTI_CHANNEL_ASSOCIATION_AGI_CONNECTOR_HPP
#define MOCK_MULTI_CHANNEL_ASSOCIATION_AGI_CONNECTOR_HPP

#include <gmock/gmock.h>

#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "component_connector.hpp"

namespace zwave_command_class::test
{
    namespace agi = command_class_association_grp_info_types;

    class MockMultiChannelAssociationAgiConnector
    {
        public:
            MOCK_METHOD(sl_status_t, get_supported_groupings, (const agi::component_connector_agi_empty_payload_t &, uint8_t &));
            MOCK_METHOD(sl_status_t, get_max_nodes, (const agi::component_connector_agi_group_max_nodes_query_t &, uint8_t &));
            MOCK_METHOD(sl_status_t, get_group_destinations, (const agi::component_connector_agi_group_destinations_query_t &, agi::component_connector_agi_group_destinations_t &));
            MOCK_METHOD(sl_status_t, add_lifeline_destinations, (const agi::component_connector_agi_lifeline_update_payload_t &));
            MOCK_METHOD(sl_status_t, remove_lifeline_destinations, (const agi::component_connector_agi_lifeline_update_payload_t &));

            sl_status_t count_status        = SL_STATUS_OK;
            uint8_t supported_groupings     = 4;
            sl_status_t capacity_status     = SL_STATUS_OK;
            uint8_t max_nodes               = 8;
            sl_status_t destinations_status = SL_STATUS_OK;
            agi::component_connector_agi_group_destinations_t destinations;

            void connect_events(component_connector &registry)
            {
                registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_empty_payload_t, uint8_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT, [this](const auto &request, uint8_t &result) {
                    result = supported_groupings;
                    return get_supported_groupings(request, result);
                });
                registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_group_max_nodes_query_t, uint8_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES, [this](const auto &request, uint8_t &result) {
                    result = max_nodes;
                    return get_max_nodes(request, result);
                });
                registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_group_destinations_query_t, agi::component_connector_agi_group_destinations_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS,
                                                                                                                                                                                                [this](const auto &request, auto &result) {
                                                                                                                                                                                                    result = destinations;
                                                                                                                                                                                                    return get_group_destinations(request, result);
                                                                                                                                                                                                });
                registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_lifeline_update_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE,
                                                                                                                                            [this](const auto &request) { return add_lifeline_destinations(request); });
                registry.connect_typed<command_class_association_grp_info_events_t, agi::component_connector_agi_lifeline_update_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE,
                                                                                                                                            [this](const auto &request) { return remove_lifeline_destinations(request); });
            }

            void set_default_responses()
            {
                testing::Mock::VerifyAndClear(this);
                count_status        = SL_STATUS_OK;
                supported_groupings = SUPPORTED_GROUPINGS;
                capacity_status     = SL_STATUS_OK;
                max_nodes           = MAX_DESTINATIONS;
                destinations_status = SL_STATUS_OK;
                destinations        = {};
                ON_CALL(*this, get_supported_groupings(testing::_, testing::_)).WillByDefault([this](const auto &, uint8_t &) { return count_status; });
                ON_CALL(*this, get_max_nodes(testing::_, testing::_)).WillByDefault([this](const auto &, uint8_t &) { return capacity_status; });
                ON_CALL(*this, get_group_destinations(testing::_, testing::_)).WillByDefault([this](const auto &, auto &) { return destinations_status; });
                ON_CALL(*this, add_lifeline_destinations(testing::_)).WillByDefault([](const auto &) { return SL_STATUS_OK; });
                ON_CALL(*this, remove_lifeline_destinations(testing::_)).WillByDefault([](const auto &) { return SL_STATUS_OK; });
            }

        private:
            static constexpr uint8_t SUPPORTED_GROUPINGS = 4;
            static constexpr uint8_t MAX_DESTINATIONS    = 8;
    };
}  // namespace zwave_command_class::test

#endif  // MOCK_MULTI_CHANNEL_ASSOCIATION_AGI_CONNECTOR_HPP
