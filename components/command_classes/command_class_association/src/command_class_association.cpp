
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
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_association.hpp"

// Component connector for interview-driven Association Supported Groupings Get
#include "component_connector.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"

#include "zwave_command_class_utils.hpp"
#include "log.h"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "zwave_tx_scheme_selector.h"

// AGI interaction goes exclusively through component_connector queries/events.
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "command_class_association_grp_info_constants.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_association";

    command_class_association::command_class_association()
    {
        component_connector connector;
        connector.connect_typed<command_class_association_events_t, component_connector_association_groupings_get_payload_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GROUPINGS_GET,
                                                                                                                             [](const component_connector_association_groupings_get_payload_t &p) { return zwave_command_class::command_class_association::on_association_groupings_get_requested(p); });
        connector.connect_typed<command_class_association_events_t, component_connector_association_groupings_get_payload_t, uint8_t>(
          command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT,
          [](const component_connector_association_groupings_get_payload_t &p, uint8_t &r) { return zwave_command_class::command_class_association::on_association_supported_groupings_count_requested(p, r); });
        connector.connect_typed<command_class_association_events_t, component_connector_association_get_payload_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GET,
                                                                                                                   [](const component_connector_association_get_payload_t &p) { return zwave_command_class::command_class_association::on_association_get_interview_requested(p); });
        connector.connect_typed<command_class_association_events_t, component_connector_association_set_payload_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_SET,
                                                                                                                   [](const component_connector_association_set_payload_t &p) { return zwave_command_class::command_class_association::on_association_set_requested(p); });
    }

    sl_status_t command_class_association::on_association_set_requested(component_connector_association_set_payload_t payload)
    {
        attribute_store::attribute endpoint_node(payload.endpoint_node);
        auto group_node               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::ASSOCIATION_SET_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired<uint8_t>(payload.grouping_identifier);
        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::node_id));
        node_id_node.set_desired<std::vector<uint8_t>>(std::vector<uint8_t> {static_cast<uint8_t>(payload.node_id)});
        command_class_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_groupings_get_requested(component_connector_association_groupings_get_payload_t payload)
    {
        sl_log_debug(LOG_TAG.data(), "Association Supported Groupings Get requested");
        auto group_node = payload.endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_groupings_get_group_attributes_t::ASSOCIATION_GROUPINGS_GET_GROUP));
        command_class_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_supported_groupings_count_requested(component_connector_association_groupings_get_payload_t payload, uint8_t &result)
    {
        result          = 0;
        auto group_node = payload.endpoint_node.child_by_type(static_cast<attribute_store_type_t>(command_class_association_types::association_groupings_report_group_attributes_t::ASSOCIATION_GROUPINGS_REPORT_GROUP));
        if (!group_node.is_valid()) {
            return SL_STATUS_OK;
        }
        auto supported_node = group_node.child_by_type(static_cast<attribute_store_type_t>(command_class_association_types::association_groupings_report_group_attributes_t::supported_groupings));
        if (!supported_node.is_valid() || !supported_node.reported_exists()) {
            return SL_STATUS_OK;
        }
        result = supported_node.reported<uint8_t>();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGet command received");
        auto *frame_generator = args.get_frame_generator;
        auto group_node       = args.node;

        auto target_value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::grouping_identifier));
        if (!target_value_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(target_value_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_association::on_association_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationSet command received");
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::grouping_identifier));
        if (!grouping_identifier_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(grouping_identifier_node, DESIRED_ATTRIBUTE);

        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::node_id));
        if (!node_id_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(node_id_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_association::on_association_get_interview_requested(component_connector_association_get_payload_t payload)
    {
        auto group_node               = payload.endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::ASSOCIATION_GET_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired<uint8_t>(payload.grouping_identifier);
        command_class_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_endpoint_node(connection_info));

        uint8_t grouping_identifier = 0;
        grouping_identifier         = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        component_connector connector;

        // Validate grouping_identifier against the count of groups AGI advertises.
        auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
        auto [count_status, max_group_id] = count_future.get();
        if (count_status == SL_STATUS_OK) {
            if (grouping_identifier == 0 || grouping_identifier > max_group_id) {
                sl_log_debug(LOG_TAG.data(), "Association Set rejected: grouping_identifier %d is unsupported (supported: 1..%d)", grouping_identifier, max_group_id);
                return SL_STATUS_FAIL;
            }
        }

        association_set_node_id_t received_node_ids;
        received_node_ids = get_value_or_default(attribute_map, "node_id", received_node_ids);

        auto max_nodes_future
          = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_max_nodes_query_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES), {grouping_identifier});
        auto [max_nodes_status, max_nodes] = max_nodes_future.get();
        if (max_nodes_status != SL_STATUS_OK) {
            max_nodes = 0;
        }

        // Find existing group with matching grouping_identifier, or create a new one
        attribute_store::attribute group_node;
        for (auto candidate: endpoint_node.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP))) {
            auto gid_node = candidate.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier));
            if (gid_node.is_valid() && gid_node.reported_exists() && gid_node.reported<uint8_t>() == grouping_identifier) {
                group_node = candidate;
                break;
            }
        }
        if (!group_node.is_valid()) {
            group_node            = endpoint_node.add_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));
            auto grouping_id_node = group_node.add_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier));
            grouping_id_node.set_reported<uint8_t>(grouping_identifier);
        }

        auto nodeid_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid));
        association_report_nodeid_t existing_node_ids;
        if (nodeid_node.reported_exists()) {
            existing_node_ids = nodeid_node.reported<association_report_nodeid_t>();
        }

        std::vector<uint8_t> new_unique_nodes;
        for (const auto &nid: received_node_ids) {
            if (std::find(existing_node_ids.begin(), existing_node_ids.end(), nid) == existing_node_ids.end() && std::find(new_unique_nodes.begin(), new_unique_nodes.end(), nid) == new_unique_nodes.end()) {
                new_unique_nodes.push_back(nid);
            }
        }

        if (max_nodes > 0 && existing_node_ids.size() + new_unique_nodes.size() > max_nodes) {
            sl_log_debug(LOG_TAG.data(), "Association Set rejected: group %d would exceed capacity (%zu existing + %zu new > %u max)", grouping_identifier, existing_node_ids.size(), new_unique_nodes.size(), max_nodes);
            return SL_STATUS_FAIL;
        }

        existing_node_ids.insert(existing_node_ids.end(), new_unique_nodes.begin(), new_unique_nodes.end());
        nodeid_node.set_reported<association_report_nodeid_t>(existing_node_ids);

        // If this is the lifeline group, notify AGI to update the centralized lifeline attribute
        if (grouping_identifier == command_class_association_grp_info_constants::LIFELINE_GROUP_ID) {
            command_class_association_grp_info_types::component_connector_agi_lifeline_update_payload_t lifeline_payload;
            lifeline_payload.node_ids = std::vector<uint8_t>(received_node_ids.begin(), received_node_ids.end());
            connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE), lifeline_payload);
        }

        sl_log_debug(LOG_TAG.data(), "Association Set received for group %d: %zu node_id(s)", grouping_identifier, received_node_ids.size());

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_remove_support_received(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_endpoint_node(connection_info));

        uint8_t grouping_identifier = 0;
        grouping_identifier         = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        // CC:0085.02.04.11.003: Unsupported Grouping Identifier MUST be ignored, except for 0.
        // Validate against AGI's authoritative count of ZPC-owned association groups.
        if (grouping_identifier > 0) {
            component_connector connector;
            auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
            auto [count_status, supported_groupings] = count_future.get();
            if (count_status == SL_STATUS_OK && grouping_identifier > supported_groupings) {
                sl_log_debug(LOG_TAG.data(), "Association Remove ignored: grouping_identifier %d is unsupported (supported: 1..%d)", grouping_identifier, supported_groupings);
                return SL_STATUS_OK;
            }
        }

        association_remove_node_id_t remove_node_ids;
        remove_node_ids = get_value_or_default(attribute_map, "node_id", remove_node_ids);

        auto report_groups = endpoint_node.children(static_cast<attribute_store_type_t>(association_report_group_attributes_t::ASSOCIATION_REPORT_GROUP));

        // Accumulate node IDs actually removed from the lifeline group (group 1) so the
        // centralized AGI lifeline can be updated even when the request is "remove all"
        // (empty remove_node_ids), which per CC:0085.02.04.11.002 means remove every
        // destination from the targeted group(s).
        std::vector<uint8_t> lifeline_node_ids_removed;

        for (auto group_node: report_groups) {
            auto gid_node = group_node.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::grouping_identifier));
            if (!gid_node.is_valid() || !gid_node.reported_exists()) {
                continue;
            }
            uint8_t group_id = gid_node.reported<uint8_t>();
            if (grouping_identifier > 0 && group_id != grouping_identifier) {
                continue;
            }

            auto nodeid_node = group_node.child_by_type(static_cast<attribute_store_type_t>(association_report_group_attributes_t::nodeid));
            if (!nodeid_node.is_valid() || !nodeid_node.reported_exists()) {
                continue;
            }

            association_report_nodeid_t existing_nodeids = nodeid_node.reported<association_report_nodeid_t>();
            const bool is_lifeline_group                 = (group_id == command_class_association_grp_info_constants::LIFELINE_GROUP_ID);

            if (remove_node_ids.empty()) {
                if (is_lifeline_group) {
                    lifeline_node_ids_removed.insert(lifeline_node_ids_removed.end(), existing_nodeids.begin(), existing_nodeids.end());
                }
                existing_nodeids.clear();
            } else {
                for (const auto &nid: remove_node_ids) {
                    auto new_end = std::remove(existing_nodeids.begin(), existing_nodeids.end(), nid);
                    if (is_lifeline_group && new_end != existing_nodeids.end()) {
                        lifeline_node_ids_removed.push_back(nid);
                    }
                    existing_nodeids.erase(new_end, existing_nodeids.end());
                }
            }

            nodeid_node.set_reported<association_report_nodeid_t>(existing_nodeids);
        }

        // When doing "remove all" on the lifeline, AGI may hold destinations
        // that were never tracked in ASSOCIATION_REPORT_GROUP (e.g. the including
        // controller added during inclusion). Query AGI and merge them in.
        bool lifeline_cleared = remove_node_ids.empty() && (grouping_identifier == 0 || grouping_identifier == command_class_association_grp_info_constants::LIFELINE_GROUP_ID);
        if (lifeline_cleared) {
            component_connector agi_connector;
            auto dest_future = agi_connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_destinations_query_t, command_class_association_grp_info_types::component_connector_agi_group_destinations_t>(
              static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS),
              {command_class_association_grp_info_constants::LIFELINE_GROUP_ID});
            auto [dest_status, agi_destinations] = dest_future.get();
            if (dest_status == SL_STATUS_OK) {
                for (const auto &nid: agi_destinations.node_ids) {
                    if (std::find(lifeline_node_ids_removed.begin(), lifeline_node_ids_removed.end(), nid) == lifeline_node_ids_removed.end()) {
                        lifeline_node_ids_removed.push_back(nid);
                    }
                }
            }
        }

        // If the lifeline group was affected, notify AGI with the actually-removed node IDs
        if (!lifeline_node_ids_removed.empty()) {
            command_class_association_grp_info_types::component_connector_agi_lifeline_update_payload_t lifeline_payload;
            lifeline_payload.node_ids = std::move(lifeline_node_ids_removed);
            component_connector connector;
            connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE), lifeline_payload);
        }

        sl_log_debug(LOG_TAG.data(), "Association Remove received: group=%d, node_id count=%zu", grouping_identifier, remove_node_ids.size());

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        uint8_t grouping_identifier = 0;
        grouping_identifier         = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        // ZPC's association group state lives exclusively in AGI. Per CC:0085.02.02.12.001
        // an unsupported Grouping Identifier (including 0) SHOULD report the lifeline group.
        uint8_t supported_groupings = 0;
        uint8_t max_nodes_supported = 0;
        association_report_nodeid_t nodeid;

        try {
            component_connector connector;
            auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
            auto [count_status, count_result] = count_future.get();
            if (count_status == SL_STATUS_OK) {
                supported_groupings = count_result;
            }
        } catch (const std::exception &e) {
            sl_log_warning(LOG_TAG.data(), "Failed to query supported groupings count: %s", e.what());
        }

        if (grouping_identifier == 0 || grouping_identifier > supported_groupings) {
            grouping_identifier = command_class_association_grp_info_constants::LIFELINE_GROUP_ID;
        }

        try {
            component_connector connector;
            auto max_nodes_future
              = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_max_nodes_query_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES), {grouping_identifier});
            auto [max_nodes_status, max_nodes_value] = max_nodes_future.get();
            if (max_nodes_status == SL_STATUS_OK) {
                max_nodes_supported = max_nodes_value;
            }
        } catch (const std::exception &e) {
            sl_log_warning(LOG_TAG.data(), "Failed to query max nodes for group %d: %s", grouping_identifier, e.what());
        }

        try {
            component_connector connector;
            auto destinations_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_destinations_query_t, command_class_association_grp_info_types::component_connector_agi_group_destinations_t>(
              static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS),
              {grouping_identifier});
            auto [destinations_status, destinations] = destinations_future.get();
            if (destinations_status == SL_STATUS_OK) {
                nodeid.assign(destinations.node_ids.begin(), destinations.node_ids.end());
            }
        } catch (const std::exception &e) {
            sl_log_warning(LOG_TAG.data(), "Failed to query destinations for group %d: %s", grouping_identifier, e.what());
        }

        // Split the node ID list across multiple reports when it does not fit
        // in a single frame. Each report carries a reports_to_follow counter so
        // the requesting node knows how many more reports are coming.
        // Overhead per report: 2 (CC + Cmd IDs) + 3 (grouping_identifier,
        // max_nodes_supported, reports_to_follow) = 5 bytes.
        constexpr uint16_t REPORT_HEADER_OVERHEAD = 5;
        const uint16_t max_payload                = zwave_tx_scheme_get_max_application_payload(connection_info->remote.node_id, connection_info->remote.endpoint_id);
        const size_t max_nids_per_report          = (max_payload > REPORT_HEADER_OVERHEAD) ? static_cast<size_t>(max_payload - REPORT_HEADER_OVERHEAD) : 1;

        size_t total_reports = 1;
        if (nodeid.size() > max_nids_per_report) {
            total_reports = (nodeid.size() + max_nids_per_report - 1) / max_nids_per_report;
        }
        // reports_to_follow is a uint8_t in the spec; cap at 256 total reports.
        constexpr size_t MAX_TOTAL_REPORTS = 256;
        size_t effective_nids              = nodeid.size();
        if (total_reports > MAX_TOTAL_REPORTS) {
            sl_log_warning(LOG_TAG.data(), "Association Report for group %u: node ID list too large (%zu), truncating to %zu entries", grouping_identifier, nodeid.size(), MAX_TOTAL_REPORTS * max_nids_per_report);
            total_reports  = MAX_TOTAL_REPORTS;
            effective_nids = MAX_TOTAL_REPORTS * max_nids_per_report;
        }

        // Send the first (total_reports - 1) reports via send_report directly;
        // the last report is returned through the provided report_frame.
        size_t sent = 0;
        for (size_t i = 0; i + 1 < total_reports; ++i) {
            const size_t chunk_size           = std::min(max_nids_per_report, effective_nids - sent);
            const uint8_t reports_to_follow_i = static_cast<uint8_t>(total_reports - 1 - i);

            zwave_frame_generator_standalone intermediate_frame;
            intermediate_frame.add_header(properties.command_class_id, static_cast<uint8_t>(command_class_association_commands_t::COMMAND_CLASS_ASSOCIATION_ASSOCIATION_REPORT));
            intermediate_frame.add_raw_byte(grouping_identifier);
            intermediate_frame.add_raw_byte(max_nodes_supported);
            intermediate_frame.add_raw_byte(reports_to_follow_i);
            for (size_t j = 0; j < chunk_size; ++j) {
                intermediate_frame.add_raw_byte(nodeid[sent + j]);
            }

            auto intermediate_bytes = intermediate_frame.generate_frame();
            command_class_utils::send_report(connection_info, static_cast<uint16_t>(intermediate_bytes.size()), intermediate_bytes.data());
            sent += chunk_size;
        }

        // Final report uses the provided report_frame (header already added by
        // the core dispatcher) and has reports_to_follow = 0.
        const size_t last_chunk_size = effective_nids - sent;
        report_frame.add_raw_byte(grouping_identifier);
        report_frame.add_raw_byte(max_nodes_supported);
        report_frame.add_raw_byte(0);
        for (size_t j = 0; j < last_chunk_size; ++j) {
            report_frame.add_raw_byte(nodeid[sent + j]);
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_attribute_map_t payload)
    {
        component_connector_association_report_payload_t callback_payload;
        callback_payload.endpoint_node       = endpoint;
        callback_payload.grouping_identifier = 0;
        auto it                              = payload.find("grouping_identifier");
        if (it != payload.end()) {
            callback_payload.grouping_identifier = std::get<uint8_t>(it->second);
        }
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_REPORT_RECEIVED), callback_payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association::on_association_groupings_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        uint8_t supported_groupings = 0;
        try {
            component_connector connector;
            auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
            auto [count_status, count_result] = count_future.get();
            if (count_status == SL_STATUS_OK) {
                supported_groupings = count_result;
            }
        } catch (const std::exception &e) {
            sl_log_warning(LOG_TAG.data(), "Failed to query supported groupings count: %s", e.what());
        }

        report_frame.add_raw_byte(supported_groupings);
        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t
      command_class_association::on_association_specific_group_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        constexpr uint8_t SPECIFIC_GROUP_NOT_SUPPORTED = 0;
        report_frame.add_raw_byte(SPECIFIC_GROUP_NOT_SUPPORTED);
        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class