
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

#include <algorithm>
#include <string_view>

#include "command_class_association_grp_info.hpp"

#include "component_connector.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "command_class_association_grp_info_generated_types.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "command_class_association_generated_types.hpp"

#include "command_class_association_grp_info_attributes.hpp"
#include "command_class_association_grp_info_constants.hpp"
#include "attribute_store_type_registration.h"
#include "zpc_attribute_store.h"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "log.h"

#include "ZW_classcmd.h"

namespace zwave_command_class
{
    using namespace command_class_association_grp_info_types;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_association_grp_info";

    namespace
    {
        std::vector<std::pair<uint8_t, uint8_t>> unpack_byte_pairs(const std::vector<uint8_t> &bytes)
        {
            std::vector<std::pair<uint8_t, uint8_t>> pairs;
            pairs.reserve(bytes.size() / 2);
            for (size_t i = 0; i + 1 < bytes.size(); i += 2) {
                pairs.emplace_back(bytes[i], bytes[i + 1]);
            }
            return pairs;
        }

        std::vector<uint8_t> pack_byte_pairs(const std::vector<std::pair<uint8_t, uint8_t>> &pairs)
        {
            std::vector<uint8_t> bytes;
            bytes.reserve(pairs.size() * 2);
            for (const auto &[first, second]: pairs) {
                bytes.push_back(first);
                bytes.push_back(second);
            }
            return bytes;
        }

        attribute_store::attribute ensure_lifeline_group_for_current_home_id()
        {
            attribute_store::attribute home_id_node(get_zpc_network_node());
            if (!home_id_node.is_valid()) {
                return {};
            }

            auto lifeline_group = home_id_node.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), command_class_association_grp_info_constants::LIFELINE_GROUP_ID);
            auto max_nodes_attr = lifeline_group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::max_nodes_supported));
            if (!max_nodes_attr.reported_exists()) {
                max_nodes_attr.set_reported<uint8_t>(command_class_association_grp_info_constants::MAX_NODES_PER_GROUP);
            }

            return lifeline_group;
        }
    }  // namespace

    command_class_association_grp_info::command_class_association_grp_info()
    {
        component_connector connector;
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_group_name_get_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_NAME_GET, [](const component_connector_agi_group_name_get_payload_t &p) {
            return zwave_command_class::command_class_association_grp_info::on_association_group_name_get_requested(p);
        });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_group_info_get_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_INFO_GET, [](const component_connector_agi_group_info_get_payload_t &p) {
            return zwave_command_class::command_class_association_grp_info::on_association_group_info_get_requested(p);
        });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_group_command_list_get_payload_t>(
          command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_GET,
          [](const component_connector_agi_group_command_list_get_payload_t &p) { return zwave_command_class::command_class_association_grp_info::on_association_group_command_list_get_requested(p); });

        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_check_command_list_payload_t>(
          command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST,
          [](const component_connector_agi_check_command_list_payload_t &p) { return zwave_command_class::command_class_association_grp_info::on_check_command_list_requested(p); });

        // Lifeline management handlers
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_lifeline_update_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE,
                                                                                                                                [](const component_connector_agi_lifeline_update_payload_t &p) { return zwave_command_class::command_class_association_grp_info::on_lifeline_add_requested(p); });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_lifeline_update_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE,
                                                                                                                                [](const component_connector_agi_lifeline_update_payload_t &p) { return zwave_command_class::command_class_association_grp_info::on_lifeline_remove_requested(p); });

        // Lifeline command list registration handlers. Command publishers (e.g.
        // Device Reset Locally) register the commands they send via the Lifeline
        // group, so AGI can advertise them in Association Group Command List Report.
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_lifeline_command_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND,
                                                                                                                                 [this](const component_connector_agi_lifeline_command_payload_t &p) { return this->on_lifeline_command_add_requested(p); });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_lifeline_command_payload_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_COMMAND,
                                                                                                                                 [this](const component_connector_agi_lifeline_command_payload_t &p) { return this->on_lifeline_command_remove_requested(p); });

        // Query handlers (request/response). These let other CCs read AGI's
        // owned attribute sub-tree without including its private layout.
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_empty_payload_t, component_connector_agi_lifeline_destinations_t>(
          command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS,
          [](const component_connector_agi_empty_payload_t &p, component_connector_agi_lifeline_destinations_t &r) { return zwave_command_class::command_class_association_grp_info::on_get_lifeline_destinations_requested(p, r); });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_empty_payload_t, uint8_t>(
          command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT,
          [](const component_connector_agi_empty_payload_t &p, uint8_t &r) { return zwave_command_class::command_class_association_grp_info::on_get_supported_groupings_count_requested(p, r); });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_group_max_nodes_query_t, uint8_t>(
          command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES,
          [](const component_connector_agi_group_max_nodes_query_t &p, uint8_t &r) { return zwave_command_class::command_class_association_grp_info::on_get_group_max_nodes_requested(p, r); });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_group_destinations_query_t, component_connector_agi_group_destinations_t>(
          command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS,
          [](const component_connector_agi_group_destinations_query_t &p, component_connector_agi_group_destinations_t &r) { return zwave_command_class::command_class_association_grp_info::on_get_group_destinations_requested(p, r); });
        connector.connect_typed<command_class_association_grp_info_events_t, component_connector_agi_empty_payload_t, component_connector_agi_lifeline_command_list_t>(
          command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_COMMAND_LIST,
          [this](const component_connector_agi_empty_payload_t &p, component_connector_agi_lifeline_command_list_t &r) { return this->on_get_lifeline_command_list_requested(p, r); });

        connector.connect_typed<component_connector_common_events_t, component_connector_network_address_updated_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NETWORK_ADDRESS_UPDATED, [](const component_connector_network_address_updated_payload_t &) {
            ensure_lifeline_group_for_current_home_id();
            return SL_STATUS_OK;
        });
        connector.connect_typed<component_connector_common_events_t, component_connector_new_network_entered_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NEW_NETWORK_ENTERED, [](const component_connector_new_network_entered_payload_t &) {
            ensure_lifeline_group_for_current_home_id();
            return SL_STATUS_OK;
        });

        ensure_lifeline_group_for_current_home_id();
    }

    void command_class_association_grp_info::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) {}

    uint8_t command_class_association_grp_info::normalize_grouping_identifier_for_report(uint8_t requested_grouping_identifier)
    {
        attribute_store::attribute home_id_node(get_zpc_network_node());
        if (!home_id_node.is_valid()) {
            return requested_grouping_identifier;
        }

        auto requested_group = home_id_node.child_by_type_and_value(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), requested_grouping_identifier);
        if (requested_group.is_valid()) {
            return requested_grouping_identifier;
        }

        auto lifeline_group = home_id_node.child_by_type_and_value(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), command_class_association_grp_info_constants::LIFELINE_GROUP_ID);
        return lifeline_group.is_valid() ? command_class_association_grp_info_constants::LIFELINE_GROUP_ID : requested_grouping_identifier;
    }

    sl_status_t command_class_association_grp_info::on_association_group_name_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_grp_info_attribute_map_t payload)
    {
        component_connector_agi_groupings_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;
        callback_payload.supported_groupings  = std::get<uint8_t>(payload.at("grouping_identifier"));
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT), callback_payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_info_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_grp_info_attribute_map_t payload)
    {
        component_connector_agi_groupings_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;
        callback_payload.supported_groupings  = std::get<uint8_t>(payload.at("group_count"));
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_INFO_REPORT), callback_payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_command_list_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_grp_info_attribute_map_t payload)
    {
        component_connector_agi_groupings_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;
        callback_payload.supported_groupings  = std::get<uint8_t>(payload.at("grouping_identifier"));
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT), callback_payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_name_get_requested(const component_connector_agi_group_name_get_payload_t &payload)
    {
        attribute_store::attribute device_endpoint_node = payload.device_endpoint_node;
        auto group_node                                 = device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::ASSOCIATION_GROUP_NAME_GET_GROUP));
        auto grouping_identifier_node                   = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::grouping_identifier));

        grouping_identifier_node.set_desired<uint8_t>(static_cast<uint8_t>(payload.grouping_identifier));
        command_class_association_grp_info_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_info_get_requested(const component_connector_agi_group_info_get_payload_t &payload)
    {
        attribute_store::attribute device_endpoint_node = payload.device_endpoint_node;
        auto group_node                                 = device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::ASSOCIATION_GROUP_INFO_GET_GROUP));
        auto list_mode_node                             = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::list_mode));
        list_mode_node.set_desired<uint8_t>(static_cast<uint8_t>(payload.list_mode));

        auto refresh_cache_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::refresh_cache));
        refresh_cache_node.set_desired<uint8_t>(static_cast<uint8_t>(payload.refresh_cache));

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired<uint8_t>(static_cast<uint8_t>(payload.grouping_identifier));

        command_class_association_grp_info_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_command_list_get_requested(const component_connector_agi_group_command_list_get_payload_t &payload)
    {
        attribute_store::attribute device_endpoint_node = payload.device_endpoint_node;
        auto group_node                                 = device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_GET_GROUP));
        auto allow_cache_node                           = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::allow_cache));
        allow_cache_node.set_desired<uint8_t>(static_cast<uint8_t>(payload.allow_cache));

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired<uint8_t>(static_cast<uint8_t>(payload.grouping_identifier));
        command_class_association_grp_info_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_name_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGroupNameGet frame assembly");
        auto *frame_generator = args.get_frame_generator;
        auto group_node       = args.node;

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::grouping_identifier));
        if (!grouping_identifier_node.desired_exists()) {
            return SL_STATUS_IS_WAITING;
        }
        frame_generator->add_value(grouping_identifier_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_association_grp_info::on_association_group_info_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGroupInfoGet frame assembly");
        auto *frame_generator = args.get_frame_generator;
        auto group_node       = args.node;

        auto list_mode_node     = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::list_mode));
        auto refresh_cache_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::refresh_cache));

        uint8_t list_mode     = list_mode_node.desired_exists() ? list_mode_node.desired<uint8_t>() : 0;
        uint8_t refresh_cache = refresh_cache_node.desired_exists() ? refresh_cache_node.desired<uint8_t>() : 0;

        uint8_t properties1 = ((list_mode != 0U) ? 0x40 : 0x00) | ((refresh_cache != 0U) ? 0x80 : 0x00);
        frame_generator->add_raw_byte(properties1);

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::grouping_identifier));
        if (!grouping_identifier_node.desired_exists()) {
            return SL_STATUS_IS_WAITING;
        }
        frame_generator->add_value(grouping_identifier_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_association_grp_info::on_association_group_command_list_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGroupCommandListGet frame assembly");
        auto *frame_generator = args.get_frame_generator;
        auto group_node       = args.node;

        auto allow_cache_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::allow_cache));

        uint8_t allow_cache = allow_cache_node.desired_exists() ? allow_cache_node.desired<uint8_t>() : 0;
        uint8_t properties1 = ((allow_cache != 0U) ? 0x80 : 0x00);
        frame_generator->add_raw_byte(properties1);

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::grouping_identifier));
        if (!grouping_identifier_node.desired_exists()) {
            return SL_STATUS_IS_WAITING;
        }
        frame_generator->add_value(grouping_identifier_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_association_grp_info::on_lifeline_add_requested(const component_connector_agi_lifeline_update_payload_t &payload)
    {
        attribute_store::attribute home_id_node(get_zpc_network_node());
        if (!home_id_node.is_valid()) {
            return SL_STATUS_FAIL;
        }

        auto lifeline_group = home_id_node.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), command_class_association_grp_info_constants::LIFELINE_GROUP_ID);

        // Merge plain NodeIDs
        if (!payload.node_ids.empty()) {
            auto node_ids_attr = lifeline_group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::node_ids));
            std::vector<uint8_t> existing;
            if (node_ids_attr.reported_exists()) {
                existing = node_ids_attr.reported<std::vector<uint8_t>>();
            }
            for (const auto &nid: payload.node_ids) {
                if (std::find(existing.begin(), existing.end(), nid) == existing.end()) {
                    existing.push_back(nid);
                }
            }
            node_ids_attr.set_reported<std::vector<uint8_t>>(existing);
        }

        // Merge endpoint associations (stored as flat [node_id, properties1] byte pairs)
        if (!payload.endpoint_associations.empty()) {
            auto ep_attr = lifeline_group.emplace_node(static_cast<attribute_store_type_t>(agi_group_attributes_t::endpoint_associations));
            std::vector<std::pair<uint8_t, uint8_t>> existing;
            if (ep_attr.reported_exists()) {
                existing = unpack_byte_pairs(ep_attr.reported<std::vector<uint8_t>>());
            }
            for (const auto &incoming: payload.endpoint_associations) {
                if (std::find(existing.begin(), existing.end(), incoming) == existing.end()) {
                    existing.push_back(incoming);
                }
            }
            ep_attr.set_reported<std::vector<uint8_t>>(pack_byte_pairs(existing));
        }

        sl_log_debug(LOG_TAG.data(), "Lifeline add: %zu node_id(s), %zu endpoint association byte(s)", payload.node_ids.size(), payload.endpoint_associations.size());
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_lifeline_remove_requested(const component_connector_agi_lifeline_update_payload_t &payload)
    {
        attribute_store::attribute home_id_node(get_zpc_network_node());
        if (!home_id_node.is_valid()) {
            return SL_STATUS_FAIL;
        }

        auto lifeline_group = home_id_node.child_by_type_and_value(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), command_class_association_grp_info_constants::LIFELINE_GROUP_ID);
        if (!lifeline_group.is_valid()) {
            return SL_STATUS_OK;
        }

        // Remove plain NodeIDs
        if (!payload.node_ids.empty()) {
            auto node_ids_attr = lifeline_group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::node_ids));
            if (node_ids_attr.is_valid() && node_ids_attr.reported_exists()) {
                std::vector<uint8_t> existing = node_ids_attr.reported<std::vector<uint8_t>>();
                for (const auto &nid: payload.node_ids) {
                    existing.erase(std::remove(existing.begin(), existing.end(), nid), existing.end());
                }
                node_ids_attr.set_reported<std::vector<uint8_t>>(existing);
            }
        }

        // Remove endpoint associations
        if (!payload.endpoint_associations.empty()) {
            auto ep_attr = lifeline_group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::endpoint_associations));
            if (ep_attr.is_valid() && ep_attr.reported_exists()) {
                auto existing = unpack_byte_pairs(ep_attr.reported<std::vector<uint8_t>>());
                for (const auto &incoming: payload.endpoint_associations) {
                    existing.erase(std::remove(existing.begin(), existing.end(), incoming), existing.end());
                }
                ep_attr.set_reported<std::vector<uint8_t>>(pack_byte_pairs(existing));
            }
        }

        sl_log_debug(LOG_TAG.data(), "Lifeline remove: %zu node_id(s), %zu endpoint association byte(s)", payload.node_ids.size(), payload.endpoint_associations.size());
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_lifeline_command_add_requested(const component_connector_agi_lifeline_command_payload_t &payload)
    {
        std::pair<uint8_t, uint8_t> incoming {payload.command_class_id, payload.command_id};
        if (std::find(lifeline_command_registry_.begin(), lifeline_command_registry_.end(), incoming) != lifeline_command_registry_.end()) {
            return SL_STATUS_OK;
        }

        lifeline_command_registry_.push_back(incoming);

        sl_log_debug(LOG_TAG.data(), "Lifeline command list: registered {0x%02X, 0x%02X}", payload.command_class_id, payload.command_id);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_lifeline_command_remove_requested(const component_connector_agi_lifeline_command_payload_t &payload)
    {
        std::pair<uint8_t, uint8_t> incoming {payload.command_class_id, payload.command_id};
        lifeline_command_registry_.erase(std::remove(lifeline_command_registry_.begin(), lifeline_command_registry_.end(), incoming), lifeline_command_registry_.end());

        sl_log_debug(LOG_TAG.data(), "Lifeline command list: unregistered {0x%02X, 0x%02X}", payload.command_class_id, payload.command_id);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_check_command_list_requested(const component_connector_agi_check_command_list_payload_t &payload)
    {
        component_connector_agi_check_command_list_result_t result;
        result.device_endpoint_node = payload.device_endpoint_node;
        result.found                = false;

        auto group_nodes = payload.device_endpoint_node.children(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP));
        for (const auto &grp: group_nodes) {
            auto cmd_list_node = grp.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::group_command_list));
            if (!cmd_list_node.is_valid() || !cmd_list_node.reported_exists()) {
                continue;
            }

            auto command_list = cmd_list_node.reported<std::vector<uint8_t>>();
            for (size_t i = 0; i + 1 < command_list.size(); i += 2) {
                if (command_list[i] == payload.command_class_id && command_list[i + 1] == payload.command_id) {
                    result.found = true;
                    break;
                }
            }
            if (result.found) {
                break;
            }
        }

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST_RESULT), result);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_get_lifeline_destinations_requested(const component_connector_agi_empty_payload_t &payload, component_connector_agi_lifeline_destinations_t &result)
    {
        result = {};

        attribute_store::attribute home_id_node(get_zpc_network_node());
        if (!home_id_node.is_valid()) {
            return SL_STATUS_NOT_AVAILABLE;
        }

        auto lifeline_group = home_id_node.child_by_type_and_value(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), command_class_association_grp_info_constants::LIFELINE_GROUP_ID);
        if (!lifeline_group.is_valid()) {
            return SL_STATUS_NOT_AVAILABLE;
        }

        auto node_ids_attr = lifeline_group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::node_ids));
        if (node_ids_attr.is_valid() && node_ids_attr.reported_exists()) {
            result.node_ids = node_ids_attr.reported<std::vector<uint8_t>>();
        }

        auto ep_attr = lifeline_group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::endpoint_associations));
        if (ep_attr.is_valid() && ep_attr.reported_exists()) {
            result.endpoint_associations = unpack_byte_pairs(ep_attr.reported<std::vector<uint8_t>>());
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_get_supported_groupings_count_requested(const component_connector_agi_empty_payload_t &payload, uint8_t &result)
    {
        result = 0;
        attribute_store::attribute home_id_node(get_zpc_network_node());
        if (!home_id_node.is_valid()) {
            return SL_STATUS_NOT_AVAILABLE;
        }

        auto groups = home_id_node.children(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP));
        result      = static_cast<uint8_t>(groups.size());
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_get_group_max_nodes_requested(const component_connector_agi_group_max_nodes_query_t &payload, uint8_t &result)
    {
        result = 0;
        attribute_store::attribute home_id_node(get_zpc_network_node());
        if (!home_id_node.is_valid()) {
            return SL_STATUS_NOT_AVAILABLE;
        }

        auto zpc_group = home_id_node.child_by_type_and_value(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), payload.grouping_identifier);
        if (!zpc_group.is_valid()) {
            return SL_STATUS_OK;
        }
        auto max_nodes_attr = zpc_group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::max_nodes_supported));
        if (max_nodes_attr.is_valid() && max_nodes_attr.reported_exists()) {
            result = max_nodes_attr.reported<uint8_t>();
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_get_group_destinations_requested(const component_connector_agi_group_destinations_query_t &payload, component_connector_agi_group_destinations_t &result)
    {
        result = {};
        attribute_store::attribute home_id_node(get_zpc_network_node());
        if (!home_id_node.is_valid()) {
            return SL_STATUS_NOT_AVAILABLE;
        }

        auto zpc_group = home_id_node.child_by_type_and_value(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP), payload.grouping_identifier);
        if (!zpc_group.is_valid()) {
            return SL_STATUS_OK;
        }

        auto node_ids_attr = zpc_group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::node_ids));
        if (node_ids_attr.is_valid() && node_ids_attr.reported_exists()) {
            result.node_ids = node_ids_attr.reported<std::vector<uint8_t>>();
        }

        auto ep_attr = zpc_group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::endpoint_associations));
        if (ep_attr.is_valid() && ep_attr.reported_exists()) {
            result.endpoint_associations = unpack_byte_pairs(ep_attr.reported<std::vector<uint8_t>>());
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_get_lifeline_command_list_requested(const component_connector_agi_empty_payload_t &payload, component_connector_agi_lifeline_command_list_t &result)
    {
        result.commands = pack_byte_pairs(lifeline_command_registry_);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_name_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info,
                                                                                                                   command_class_association_grp_info_attribute_map_t attribute_map,
                                                                                                                   zwave_frame_generator_standalone &report_frame,
                                                                                                                   std::vector<uint8_t> &frame)
    {
        const uint8_t requested_grouping_identifier = get_value_or_default<uint8_t>(attribute_map, "grouping_identifier", 0);
        uint8_t grouping_identifier                 = normalize_grouping_identifier_for_report(requested_grouping_identifier);

        std::string_view name = (grouping_identifier == command_class_association_grp_info_constants::LIFELINE_GROUP_ID) ? std::string_view {"Lifeline"} : std::string_view {""};

        report_frame.add_raw_byte(grouping_identifier);
        report_frame.add_raw_byte(static_cast<uint8_t>(name.size()));
        for (char c: name) {
            report_frame.add_raw_byte(static_cast<uint8_t>(c));
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_info_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info,
                                                                                                                   command_class_association_grp_info_attribute_map_t attribute_map,
                                                                                                                   zwave_frame_generator_standalone &report_frame,
                                                                                                                   std::vector<uint8_t> &frame)
    {
        const uint8_t requested_grouping_identifier = get_value_or_default<uint8_t>(attribute_map, "grouping_identifier", 0);
        uint8_t grouping_identifier                 = normalize_grouping_identifier_for_report(requested_grouping_identifier);
        uint8_t list_mode                           = get_value_or_default<uint8_t>(attribute_map, "list_mode", 0);

        // Collect the set of ZPC-owned groups to report on. list_mode=1 means
        // advertise all groups; list_mode=0 means the single requested group.
        std::vector<uint8_t> group_ids;
        if (list_mode != 0U) {
            attribute_store::attribute home_id_node(get_zpc_network_node());
            if (home_id_node.is_valid()) {
                for (const auto &grp: home_id_node.children(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_ZPC_GROUP))) {
                    if (grp.reported_exists()) {
                        group_ids.push_back(grp.reported<uint8_t>());
                    }
                }
            }
            std::sort(group_ids.begin(), group_ids.end());
            group_ids.erase(std::unique(group_ids.begin(), group_ids.end()), group_ids.end());
            if (group_ids.empty()) {
                group_ids.push_back(command_class_association_grp_info_constants::LIFELINE_GROUP_ID);
            }
        } else {
            group_ids.push_back(grouping_identifier);
        }

        uint8_t properties1 = static_cast<uint8_t>(((list_mode != 0U) ? 0x80 : 0x00) | (group_ids.size() & 0x3F));
        report_frame.add_raw_byte(properties1);

        for (uint8_t id: group_ids) {
            uint8_t profile1 = 0x00;
            uint8_t profile2 = 0x00;
            if (id == command_class_association_grp_info_constants::LIFELINE_GROUP_ID) {
                profile1 = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_GENERAL_V3;
                profile2 = ASSOCIATION_GROUP_INFO_REPORT_AGI_GENERAL_LIFELINE_V3;
            }
            report_frame.add_raw_byte(id);        // Grouping Identifier
            report_frame.add_raw_byte(0x00);      // Mode (reserved in V3)
            report_frame.add_raw_byte(profile1);  // Profile1
            report_frame.add_raw_byte(profile2);  // Profile2
            report_frame.add_raw_byte(0x00);      // Reserved
            report_frame.add_raw_byte(0x00);      // Event Code MSB
            report_frame.add_raw_byte(0x00);      // Event Code LSB
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info::on_association_group_command_list_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info,
                                                                                                                           command_class_association_grp_info_attribute_map_t attribute_map,
                                                                                                                           zwave_frame_generator_standalone &report_frame,
                                                                                                                           std::vector<uint8_t> &frame)
    {
        const uint8_t requested_grouping_identifier = get_value_or_default<uint8_t>(attribute_map, "grouping_identifier", 0);
        uint8_t grouping_identifier                 = normalize_grouping_identifier_for_report(requested_grouping_identifier);

        std::vector<uint8_t> commands;
        if (grouping_identifier == command_class_association_grp_info_constants::LIFELINE_GROUP_ID) {
            commands = pack_byte_pairs(lifeline_command_registry_);
        }

        report_frame.add_raw_byte(grouping_identifier);
        report_frame.add_raw_byte(static_cast<uint8_t>(commands.size()));
        for (uint8_t b: commands) {
            report_frame.add_raw_byte(b);
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
