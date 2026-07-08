/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMMAND_CLASS_ASSOCIATION_GRP_INFO_TYPES_H
#define COMMAND_CLASS_ASSOCIATION_GRP_INFO_TYPES_H

#include <utility>
#include <vector>
#include "attribute_store.h"
#include "command_class_association_grp_info_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_association_grp_info_types
    {
        struct component_connector_agi_groupings_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t supported_groupings;
        };

        /**
         * @brief Payload for Association Group Name Get request.
         */
        struct component_connector_agi_group_name_get_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t grouping_identifier;
        };

        /**
         * @brief Payload for Association Group Info Get request.
         */
        struct component_connector_agi_group_info_get_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t grouping_identifier;
                uint8_t list_mode;
                uint8_t refresh_cache;
        };

        /**
         * @brief Payload for Association Group Command List Get request.
         */
        struct component_connector_agi_group_command_list_get_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t grouping_identifier;
                uint8_t allow_cache;
        };

        struct component_connector_agi_check_command_list_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t command_class_id;
                uint8_t command_id;
        };

        struct component_connector_agi_check_command_list_result_t {
                attribute_store::attribute device_endpoint_node;
                bool found;
        };

        struct component_connector_agi_lifeline_update_payload_t {
                std::vector<uint8_t> node_ids;
                std::vector<std::pair<uint8_t, uint8_t>> endpoint_associations;
        };

        // Empty payload marker for AGI query events that take no input.
        struct component_connector_agi_empty_payload_t {};

        // Result of COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS.
        // node_ids: plain NodeID destinations of the lifeline group.
        // endpoint_associations: (node_id, endpoint) pairs; endpoint MSB carries the
        // bit-resolution flag as defined by the spec (caller must mask 0x7F to get the endpoint).
        struct component_connector_agi_lifeline_destinations_t {
                std::vector<uint8_t> node_ids;
                std::vector<std::pair<uint8_t, uint8_t>> endpoint_associations;
        };

        struct component_connector_agi_group_max_nodes_query_t {
                uint8_t grouping_identifier;
        };

        // Query input for COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS.
        struct component_connector_agi_group_destinations_query_t {
                uint8_t grouping_identifier;
        };

        // Result of COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS.
        // Same shape as component_connector_agi_lifeline_destinations_t but applies
        // to any ZPC-owned association group identified by grouping_identifier.
        struct component_connector_agi_group_destinations_t {
                std::vector<uint8_t> node_ids;
                std::vector<std::pair<uint8_t, uint8_t>> endpoint_associations;
        };

        // Payload for COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND /
        // _REMOVE_LIFELINE_COMMAND. Command publishers (e.g. Device Reset Locally)
        // use these events to declare which commands they send via the Lifeline
        // group, so AGI can advertise them in Association Group Command List Report.
        struct component_connector_agi_lifeline_command_payload_t {
                uint8_t command_class_id;
                uint8_t command_id;
        };

        // Result of COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_COMMAND_LIST.
        // Flat byte stream of CC,Cmd,CC,Cmd,... pairs ready for the Group Command
        // List Report body.
        struct component_connector_agi_lifeline_command_list_t {
                std::vector<uint8_t> commands;
        };

    }  // namespace command_class_association_grp_info_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_ASSOCIATION_GRP_INFO_TYPES_H
