
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

// Base class
#include "command_class_multi_channel_association.hpp"
#include "component_connector.hpp"
#include "command_class_multi_channel_association_events.hpp"
#include "command_class_multi_channel_association_constants.hpp"
#include "command_class_multi_channel_association_types.hpp"

#include "zpc_attribute_store_network_helper.h"
#include "zwave_network_management.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_tx_scheme_selector.h"
#include "zwave_utils.h"
#include "log.h"

// Z-Wave defintions
#include "ZW_classcmd.h"

// AGI lifeline management via component_connector
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_multi_channel_association";

    command_class_multi_channel_association::command_class_multi_channel_association()
    {
        // At end of interview, send Multi Channel Association Supported Groupings Get when requested by device_interviewer
        component_connector connector;
        connector.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_groupings_get_payload_t>(
          command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET,
          [](const component_connector_multi_channel_association_groupings_get_payload_t &p) { return zwave_command_class::command_class_multi_channel_association::on_multi_channel_association_groupings_get_requested(p); });
        connector.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_groupings_get_payload_t, uint8_t>(
          command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_COUNT,
          [](const component_connector_multi_channel_association_groupings_get_payload_t &p, uint8_t &r) { return zwave_command_class::command_class_multi_channel_association::on_multi_channel_association_supported_groupings_count_requested(p, r); });
        connector.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_get_payload_t>(
          command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GET,
          [](const component_connector_multi_channel_association_get_payload_t &p) { return zwave_command_class::command_class_multi_channel_association::on_multi_channel_association_get_interview_requested(p); });
        connector.connect_typed<command_class_multi_channel_association_events_t, component_connector_multi_channel_association_set_payload_t>(
          command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_SET,
          [](const component_connector_multi_channel_association_set_payload_t &p) { return zwave_command_class::command_class_multi_channel_association::on_multi_channel_association_set_requested(p); });
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_set_requested(component_connector_multi_channel_association_set_payload_t payload)
    {
        auto group_node               = payload.endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired<uint8_t>(payload.grouping_identifier);

        // A destination is either a plain (root-device) Node ID association or an
        // End Point association carried in the variant group ("vg") section
        // after the 0x00 marker. Adding it to both sections of the same Set
        // frame would register the destination twice in the target's
        // association group, so pick exactly one representation.
        //
        // Per CL:008E.01.21.02.2 the lifeline association MUST be an End Point
        // association when both the controlling node (ZPC, always MCA v3+) and
        // the supporting node implement MCA v3+ AND the supporting node also
        // supports the Multi Channel Command Class. ZPC is always the
        // controlling node here, so the rule reduces to checking the
        // destination (supporting) node's capabilities.
        //
        // CC versions are uniform across a node's endpoints (the per-endpoint
        // Version interview deliberately skips CCs already versioned on root,
        // see PrepareEndpointVersionsStep), so always query the root endpoint
        // for both MCA version and Multi Channel CC support, regardless of
        // which endpoint of the supporting node we are configuring.
        bool use_endpoint_association = (payload.endpoint_id != 0);
        if (!use_endpoint_association && payload.grouping_identifier == command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER) {
            zwave_node_id_t destination_node_id      = 0;
            zwave_endpoint_id_t destination_endpoint = 0;
            // payload.endpoint_node IS the destination's ATTRIBUTE_ENDPOINT_ID
            // attribute, so the per-helper *_from_node calls return NOT_FOUND
            // because get_first_parent_with_type skips the input node itself.
            // get_zwave_ids_from_node handles that case correctly.
            if (attribute_store_network_helper_get_zwave_ids_from_node(payload.endpoint_node, &destination_node_id, &destination_endpoint) == SL_STATUS_OK) {
                const uint8_t destination_mca_version            = zwave_node_get_command_class_version(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3, destination_node_id, 0);
                const bool destination_supports_multi_channel_cc = zwave_node_supports_command_class(COMMAND_CLASS_MULTI_CHANNEL_V4, destination_node_id, 0);
                const bool spec_requires_endpoint_assoc          = (destination_mca_version >= MULTI_CHANNEL_ASSOCIATION_VERSION_V3) && destination_supports_multi_channel_cc;
                sl_log_info(LOG_TAG.data(),
                            "Lifeline encoding decision for NodeID %d Endpoint %d: MCA v%u on root EP, Multi Channel CC supported=%d, use End Point Association=%d (CL:008E.01.21.02.2)",
                            destination_node_id,
                            destination_endpoint,
                            destination_mca_version,
                            destination_supports_multi_channel_cc,
                            spec_requires_endpoint_assoc);
                if (spec_requires_endpoint_assoc) {
                    use_endpoint_association = true;
                }
            } else {
                sl_log_info(LOG_TAG.data(), "Lifeline encoding: could not resolve destination NodeID/Endpoint from endpoint_node, falling back to NodeID association");
            }
        }

        if (!use_endpoint_association) {
            auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id));
            node_id_node.set_desired<uint8_t>(payload.node_id);
        } else {
            multi_channel_association_set_vg_t_item_t vg_item {};
            vg_item.multi_channel_node_id                                       = static_cast<uint8_t>(payload.node_id);
            vg_item.properties1.flags.multi_channel_association_set_end_point   = payload.endpoint_id & static_cast<uint8_t>(multi_channel_association_set_vg_properties1_attribute_masks_t::end_point_mask);
            vg_item.properties1.flags.multi_channel_association_set_bit_address = 0;

            auto vg_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg));
            vg_node.set_desired(multi_channel_association_set_vg_t {vg_item});

            auto marker_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::marker));
            marker_node.set_desired(static_cast<uint8_t>(0));
        }

        command_class_multi_channel_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_groupings_get_requested(component_connector_multi_channel_association_groupings_get_payload_t payload)
    {
        auto group_node = payload.endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_groupings_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET_GROUP));
        command_class_multi_channel_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_supported_groupings_count_requested(component_connector_multi_channel_association_groupings_get_payload_t payload, uint8_t &result)
    {
        result          = 0;
        auto group_node = payload.endpoint_node.child_by_type(static_cast<attribute_store_type_t>(command_class_multi_channel_association_types::multi_channel_association_groupings_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT_GROUP));
        if (!group_node.is_valid()) {
            return SL_STATUS_OK;
        }
        auto supported_node = group_node.child_by_type(static_cast<attribute_store_type_t>(command_class_multi_channel_association_types::multi_channel_association_groupings_report_group_attributes_t::supported_groupings));
        if (!supported_node.is_valid() || !supported_node.reported_exists()) {
            return SL_STATUS_OK;
        }
        result = supported_node.reported<uint8_t>();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_get_interview_requested(component_connector_multi_channel_association_get_payload_t payload)
    {
        auto group_node               = payload.endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GET_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired<uint8_t>(payload.grouping_identifier);
        command_class_multi_channel_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_groupings_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_association_attribute_map_t payload)
    {
        (void)connection_info;
        multi_channel_association_groupings_report_supported_groupings_t supported_groupings = 0;
        supported_groupings                                                                  = get_value_or_default(payload, "supported_groupings", supported_groupings);

        component_connector_multi_channel_association_groupings_get_payload_t callback_payload;
        callback_payload.endpoint_node       = endpoint;
        callback_payload.supported_groupings = static_cast<uint8_t>(supported_groupings);

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT), callback_payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_association_attribute_map_t payload)
    {
        component_connector_multi_channel_association_report_payload_t callback_payload;
        callback_payload.endpoint_node       = endpoint;
        callback_payload.grouping_identifier = 0;

        uint8_t grouping_identifier          = 0;
        grouping_identifier                  = get_value_or_default(payload, "grouping_identifier", grouping_identifier);
        callback_payload.grouping_identifier = grouping_identifier;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_multi_channel_association_events_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED), callback_payload);
        return SL_STATUS_OK;
    }

    void command_class_multi_channel_association::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) {}

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto *frame_generator = args.get_frame_generator;
        auto group_node       = args.node;

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::grouping_identifier));
        if (!grouping_identifier_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(grouping_identifier_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto *frame_generator = args.set_frame_generator;
        auto group_node       = args.node;

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::grouping_identifier));
        if (!grouping_identifier_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(grouping_identifier_node, DESIRED_ATTRIBUTE);

        // Destinations may be carried as plain Node IDs, as Endpoint associations
        // in the vg section, or both. Require at least one to be populated but
        // treat each one individually as optional so the requester can pick the
        // right representation for the destination it wants to add.
        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id));
        auto marker_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::marker));
        auto vg_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg));

        if (!node_id_node.desired_exists() && !vg_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        if (node_id_node.desired_exists()) {
            frame_generator->add_value(node_id_node, DESIRED_ATTRIBUTE);
        }
        if (marker_node.desired_exists()) {
            frame_generator->add_value(marker_node, DESIRED_ATTRIBUTE);
        }
        if (vg_node.desired_exists()) {
            frame_generator->add_value(vg_node, DESIRED_ATTRIBUTE);
        }

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_multi_channel_association_attribute_map_t attribute_map)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_endpoint_node(connection_info));

        uint8_t grouping_identifier = 0;
        grouping_identifier         = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        component_connector connector;

        // CC:008E.03.01.11.002 / CC:008E.02.01.11.003: Unsupported Grouping Identifier MUST be ignored.
        // Validate against AGI's authoritative count of ZPC-owned association groups.
        auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
        auto [count_status, supported_groupings] = count_future.get();
        if (count_status == SL_STATUS_OK) {
            if (grouping_identifier == 0 || grouping_identifier > supported_groupings) {
                sl_log_debug(LOG_TAG.data(), "Multi Channel Association Set ignored: grouping_identifier %d is unsupported (supported: 1..%d)", grouping_identifier, supported_groupings);
                return SL_STATUS_FAIL;
            }
        }

        multi_channel_association_set_node_id_t received_node_ids;
        received_node_ids = get_value_or_default(attribute_map, "node_id", received_node_ids);

        multi_channel_association_set_vg_t received_vg;
        received_vg = get_value_or_default(attribute_map, "vg", received_vg);

        // Per CC:008E.02.00.21.004 the destination capacity is shared between NodeID
        // and End Point destinations. Ask AGI for the configured maximum.
        auto max_nodes_future
          = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_max_nodes_query_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES), {grouping_identifier});
        auto [max_nodes_status, max_nodes] = max_nodes_future.get();
        if (max_nodes_status != SL_STATUS_OK) {
            max_nodes = 0;
        }

        // Find existing group with matching grouping_identifier, or create a new one
        attribute_store::attribute group_node;
        for (auto candidate: endpoint_node.children(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP))) {
            auto gid_node = candidate.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier));
            if (gid_node.is_valid() && gid_node.reported_exists() && gid_node.reported<uint8_t>() == grouping_identifier) {
                group_node = candidate;
                break;
            }
        }
        if (!group_node.is_valid()) {
            group_node            = endpoint_node.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP));
            auto grouping_id_node = group_node.add_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier));
            grouping_id_node.set_reported<uint8_t>(grouping_identifier);
        }

        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id));
        multi_channel_association_report_node_id_t existing_node_ids;
        if (node_id_node.reported_exists()) {
            existing_node_ids = node_id_node.reported<multi_channel_association_report_node_id_t>();
        }

        auto vg_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg));
        multi_channel_association_report_vg_t existing_vg;
        if (vg_node.reported_exists()) {
            existing_vg = vg_node.reported<multi_channel_association_report_vg_t>();
        }

        // Compute new unique destinations before committing to enforce capacity atomically.
        std::vector<uint8_t> new_unique_node_ids;
        for (const auto &nid: received_node_ids) {
            if (std::find(existing_node_ids.begin(), existing_node_ids.end(), nid) == existing_node_ids.end() && std::find(new_unique_node_ids.begin(), new_unique_node_ids.end(), nid) == new_unique_node_ids.end()) {
                new_unique_node_ids.push_back(nid);
            }
        }
        std::vector<multi_channel_association_report_vg_t_item_t> new_unique_vg;
        for (const auto &item: received_vg) {
            auto matches = [&item](const multi_channel_association_report_vg_t_item_t &existing) {
                return existing.multi_channel_node_id == item.multi_channel_node_id && existing.properties1.value == item.properties1.value;
            };
            if (std::find_if(existing_vg.begin(), existing_vg.end(), matches) == existing_vg.end() && std::find_if(new_unique_vg.begin(), new_unique_vg.end(), matches) == new_unique_vg.end()) {
                multi_channel_association_report_vg_t_item_t report_item;
                report_item.multi_channel_node_id = item.multi_channel_node_id;
                report_item.properties1.value     = item.properties1.value;
                new_unique_vg.push_back(report_item);
            }
        }

        if (max_nodes > 0 && existing_node_ids.size() + existing_vg.size() + new_unique_node_ids.size() + new_unique_vg.size() > max_nodes) {
            sl_log_debug(LOG_TAG.data(), "Multi Channel Association Set rejected: group %d would exceed capacity (%zu existing + %zu new > %u max)", grouping_identifier, existing_node_ids.size() + existing_vg.size(), new_unique_node_ids.size() + new_unique_vg.size(), max_nodes);
            return SL_STATUS_FAIL;
        }

        existing_node_ids.insert(existing_node_ids.end(), new_unique_node_ids.begin(), new_unique_node_ids.end());
        node_id_node.set_reported<multi_channel_association_report_node_id_t>(existing_node_ids);

        existing_vg.insert(existing_vg.end(), new_unique_vg.begin(), new_unique_vg.end());
        vg_node.set_reported<multi_channel_association_report_vg_t>(existing_vg);

        if (grouping_identifier == command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER) {
            command_class_association_grp_info_types::component_connector_agi_lifeline_update_payload_t lifeline_payload;
            lifeline_payload.node_ids = std::vector<uint8_t>(received_node_ids.begin(), received_node_ids.end());
            for (const auto &item: received_vg) {
                lifeline_payload.endpoint_associations.emplace_back(item.multi_channel_node_id, item.properties1.value);
            }
            connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE), lifeline_payload);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_remove_support_received(const zwave_controller_connection_info_t *connection_info, command_class_multi_channel_association_attribute_map_t attribute_map)
    {
        attribute_store::attribute endpoint_node(command_class_utils::get_endpoint_node(connection_info));

        uint8_t grouping_identifier = 0;
        grouping_identifier         = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        // CC:008E.03.05.11.003: Unsupported Grouping Identifier MUST be ignored, except for 0.
        // Validate against AGI's authoritative count of ZPC-owned association groups.
        if (grouping_identifier > 0) {
            component_connector connector;
            auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
            auto [count_status, supported_groupings] = count_future.get();
            if (count_status == SL_STATUS_OK && grouping_identifier > supported_groupings) {
                sl_log_debug(LOG_TAG.data(), "Multi Channel Association Remove ignored: grouping_identifier %d is unsupported (supported: 1..%d)", grouping_identifier, supported_groupings);
                return SL_STATUS_OK;
            }
        }

        multi_channel_association_remove_node_id_t remove_node_ids;
        remove_node_ids = get_value_or_default(attribute_map, "node_id", remove_node_ids);

        multi_channel_association_remove_vg_t remove_vg;
        remove_vg = get_value_or_default(attribute_map, "vg", remove_vg);

        auto report_groups = endpoint_node.children(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP));

        // Accumulate destinations actually removed from the lifeline group (group 1) so the
        // centralized AGI lifeline can be updated even when the request is "remove all"
        // (empty remove_node_ids and remove_vg), which per CC:008E.03.05.11.002 means
        // remove every destination from the targeted group(s).
        std::vector<uint8_t> lifeline_node_ids_removed;
        std::vector<std::pair<uint8_t, uint8_t>> lifeline_endpoint_associations_removed;
        const bool remove_all = remove_node_ids.empty() && remove_vg.empty();

        for (auto group_node: report_groups) {
            auto gid_node = group_node.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::grouping_identifier));
            if (!gid_node.is_valid() || !gid_node.reported_exists()) {
                continue;
            }
            uint8_t group_id = gid_node.reported<uint8_t>();
            if (grouping_identifier > 0 && group_id != grouping_identifier) {
                continue;
            }
            const bool is_lifeline_group = (group_id == 1);

            // Remove node_ids
            auto node_id_node = group_node.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::node_id));
            if (node_id_node.is_valid() && node_id_node.reported_exists()) {
                multi_channel_association_report_node_id_t existing_node_ids = node_id_node.reported<multi_channel_association_report_node_id_t>();
                if (remove_all) {
                    if (is_lifeline_group) {
                        lifeline_node_ids_removed.insert(lifeline_node_ids_removed.end(), existing_node_ids.begin(), existing_node_ids.end());
                    }
                    existing_node_ids.clear();
                } else {
                    for (const auto &nid: remove_node_ids) {
                        auto new_end = std::remove(existing_node_ids.begin(), existing_node_ids.end(), nid);
                        if (is_lifeline_group && new_end != existing_node_ids.end()) {
                            lifeline_node_ids_removed.push_back(nid);
                        }
                        existing_node_ids.erase(new_end, existing_node_ids.end());
                    }
                }
                node_id_node.set_reported<multi_channel_association_report_node_id_t>(existing_node_ids);
            }

            // Remove vg items
            auto vg_node = group_node.child_by_type(static_cast<attribute_store_type_t>(multi_channel_association_report_group_attributes_t::vg));
            if (vg_node.is_valid() && vg_node.reported_exists()) {
                multi_channel_association_report_vg_t existing_vg = vg_node.reported<multi_channel_association_report_vg_t>();
                if (remove_all) {
                    if (is_lifeline_group) {
                        for (const auto &existing: existing_vg) {
                            lifeline_endpoint_associations_removed.emplace_back(existing.multi_channel_node_id, existing.properties1.value);
                        }
                    }
                    existing_vg.clear();
                } else {
                    for (const auto &item: remove_vg) {
                        auto new_end = std::remove_if(existing_vg.begin(), existing_vg.end(), [&item](const multi_channel_association_report_vg_t_item_t &existing) { return existing.multi_channel_node_id == item.multi_channel_node_id && existing.properties1.value == item.properties1.value; });
                        if (is_lifeline_group && new_end != existing_vg.end()) {
                            lifeline_endpoint_associations_removed.emplace_back(item.multi_channel_node_id, item.properties1.value);
                        }
                        existing_vg.erase(new_end, existing_vg.end());
                    }
                }
                vg_node.set_reported<multi_channel_association_report_vg_t>(existing_vg);
            }
        }

        // When doing "remove all" on the lifeline, AGI may hold destinations
        // that were never tracked in MULTI_CHANNEL_ASSOCIATION_REPORT_GROUP (e.g. the including
        // controller added during inclusion). Query AGI and merge them in.
        bool lifeline_cleared = remove_all && (grouping_identifier == 0 || grouping_identifier == command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER);
        if (lifeline_cleared) {
            component_connector agi_connector;
            auto dest_future = agi_connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_destinations_query_t, command_class_association_grp_info_types::component_connector_agi_group_destinations_t>(
              static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS),
              {command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER});
            auto [dest_status, agi_destinations] = dest_future.get();
            if (dest_status == SL_STATUS_OK) {
                for (const auto &nid: agi_destinations.node_ids) {
                    if (std::find(lifeline_node_ids_removed.begin(), lifeline_node_ids_removed.end(), nid) == lifeline_node_ids_removed.end()) {
                        lifeline_node_ids_removed.push_back(nid);
                    }
                }
                for (const auto &ep: agi_destinations.endpoint_associations) {
                    if (std::find(lifeline_endpoint_associations_removed.begin(), lifeline_endpoint_associations_removed.end(), ep) == lifeline_endpoint_associations_removed.end()) {
                        lifeline_endpoint_associations_removed.push_back(ep);
                    }
                }
            }
        }

        // If the lifeline group was affected, notify AGI with the actually-removed destinations
        if (!lifeline_node_ids_removed.empty() || !lifeline_endpoint_associations_removed.empty()) {
            command_class_association_grp_info_types::component_connector_agi_lifeline_update_payload_t lifeline_payload;
            lifeline_payload.node_ids              = std::move(lifeline_node_ids_removed);
            lifeline_payload.endpoint_associations = std::move(lifeline_endpoint_associations_removed);
            component_connector connector;
            connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE), lifeline_payload);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info,
                                                                                                                           command_class_multi_channel_association_attribute_map_t attribute_map,
                                                                                                                           zwave_frame_generator_standalone &report_frame,
                                                                                                                           std::vector<uint8_t> &frame)
    {
        uint8_t grouping_identifier = 0;
        grouping_identifier         = get_value_or_default(attribute_map, "grouping_identifier", grouping_identifier);

        // ZPC's association group state lives exclusively in AGI. Per CC:008E.02.02.12.001
        // an unsupported Grouping Identifier (including 0) SHOULD report the lifeline group.
        component_connector connector;
        auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
        auto [count_status, supported_groupings] = count_future.get();
        if (count_status != SL_STATUS_OK) {
            supported_groupings = 0;
        }
        if (grouping_identifier == 0 || grouping_identifier > supported_groupings) {
            grouping_identifier = command_class_multi_channel_association_constants::LIFELINE_GROUPING_IDENTIFIER;
        }

        auto max_nodes_future
          = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_max_nodes_query_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES), {grouping_identifier});
        auto [max_nodes_status, max_nodes_value] = max_nodes_future.get();
        uint8_t max_nodes_supported              = (max_nodes_status == SL_STATUS_OK) ? max_nodes_value : 0;

        auto destinations_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_group_destinations_query_t, command_class_association_grp_info_types::component_connector_agi_group_destinations_t>(
          static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS),
          {grouping_identifier});
        auto [destinations_status, destinations] = destinations_future.get();

        multi_channel_association_report_node_id_t node_ids;
        multi_channel_association_report_vg_t vg_items;
        if (destinations_status == SL_STATUS_OK) {
            node_ids.assign(destinations.node_ids.begin(), destinations.node_ids.end());
            vg_items.reserve(destinations.endpoint_associations.size());
            for (const auto &ep: destinations.endpoint_associations) {
                multi_channel_association_report_vg_t_item_t item;
                item.multi_channel_node_id = ep.first;
                item.properties1.value     = ep.second;
                vg_items.push_back(item);
            }
        }

        // Split the destination list across multiple reports when it does not
        // fit in a single frame. Each report carries a reports_to_follow
        // counter so the requesting node knows how many more reports follow.
        // Overhead per report: 2 (CC + Cmd IDs) + 3 (grouping_identifier,
        // max_nodes_supported, reports_to_follow) = 5 bytes. When a report
        // contains any vg items it also needs a 1-byte 0x00 marker.
        constexpr uint16_t REPORT_HEADER_OVERHEAD = 5;
        constexpr uint16_t MARKER_SIZE            = 1;
        constexpr uint16_t VG_ITEM_SIZE           = 2;
        const uint16_t max_payload                = zwave_tx_scheme_get_max_application_payload(connection_info->remote.node_id, connection_info->remote.endpoint_id);
        const size_t payload_capacity             = (max_payload > REPORT_HEADER_OVERHEAD) ? static_cast<size_t>(max_payload - REPORT_HEADER_OVERHEAD) : 1;

        auto build_report_payload = [&](zwave_frame_generator_standalone &gen, uint8_t rtf, const uint8_t *nid_start, size_t nid_count, const multi_channel_association_report_vg_t_item_t *vg_start, size_t vg_count) {
            gen.add_raw_byte(grouping_identifier);
            gen.add_raw_byte(max_nodes_supported);
            gen.add_raw_byte(rtf);
            for (size_t k = 0; k < nid_count; ++k) {
                gen.add_raw_byte(nid_start[k]);
            }
            if (vg_count > 0) {
                gen.add_raw_byte(0x00);
                for (size_t k = 0; k < vg_count; ++k) {
                    gen.add_raw_byte(vg_start[k].multi_channel_node_id);
                    gen.add_raw_byte(vg_start[k].properties1.value);
                }
            }
        };

        // Per the spec, node IDs and endpoint associations (VG) belong in the
        // same report when they fit. Only split across multiple reports when the
        // combined payload exceeds a single frame.
        const size_t vg_wire_bytes        = vg_items.empty() ? 0 : MARKER_SIZE + (vg_items.size() * VG_ITEM_SIZE);
        const size_t total_combined_bytes = node_ids.size() + vg_wire_bytes;

        if (total_combined_bytes <= payload_capacity) {
            build_report_payload(report_frame, 0, node_ids.data(), node_ids.size(), vg_items.data(), vg_items.size());
            frame = report_frame.generate_frame();
            return SL_STATUS_OK;
        }

        // Multi-report path: pack as many node IDs as possible into each report,
        // then fill remaining capacity with VG items after a 0x00 marker. Once
        // all node IDs are sent, remaining reports carry VG items only.
        struct report_chunk {
                size_t nid_offset;
                size_t nid_count;
                size_t vg_offset;
                size_t vg_count;
        };
        std::vector<report_chunk> chunks;
        size_t nids_remaining = node_ids.size();
        size_t vgs_remaining  = vg_items.size();
        size_t nid_pos        = 0;
        size_t vg_pos         = 0;

        while (nids_remaining > 0 || vgs_remaining > 0) {
            size_t nid_count = 0;
            size_t vg_count  = 0;

            if (nids_remaining > 0) {
                nid_count                   = std::min(nids_remaining, payload_capacity);
                size_t remaining_after_nids = payload_capacity - nid_count;
                if (vgs_remaining > 0 && remaining_after_nids > MARKER_SIZE) {
                    vg_count = std::min(vgs_remaining, (remaining_after_nids - MARKER_SIZE) / VG_ITEM_SIZE);
                }
            } else {
                vg_count = std::min(vgs_remaining, (payload_capacity > MARKER_SIZE) ? (payload_capacity - MARKER_SIZE) / VG_ITEM_SIZE : static_cast<size_t>(1));
            }

            chunks.push_back({nid_pos, nid_count, vg_pos, vg_count});
            nid_pos += nid_count;
            vg_pos += vg_count;
            nids_remaining -= nid_count;
            vgs_remaining -= vg_count;
        }

        if (chunks.empty()) {
            chunks.push_back({0, 0, 0, 0});
        }

        constexpr size_t MAX_TOTAL_REPORTS = 256;
        if (chunks.size() > MAX_TOTAL_REPORTS) {
            sl_log_warning(LOG_TAG.data(), "Multi Channel Association Report for group %u: destination list too large (%zu node IDs, %zu vg items), truncating", grouping_identifier, node_ids.size(), vg_items.size());
            chunks.resize(MAX_TOTAL_REPORTS);
        }

        // Send all but the last report through send_report. The last report is
        // placed in the provided report_frame for the core dispatcher.
        for (size_t i = 0; i + 1 < chunks.size(); ++i) {
            const auto &c                     = chunks[i];
            const uint8_t reports_to_follow_i = static_cast<uint8_t>(chunks.size() - 1 - i);

            zwave_frame_generator_standalone intermediate_frame;
            intermediate_frame.add_header(properties.command_class_id, static_cast<uint8_t>(command_class_multi_channel_association_commands_t::COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_MULTI_CHANNEL_ASSOCIATION_REPORT));
            build_report_payload(intermediate_frame, reports_to_follow_i, node_ids.data() + c.nid_offset, c.nid_count, vg_items.data() + c.vg_offset, c.vg_count);

            auto intermediate_bytes = intermediate_frame.generate_frame();
            command_class_utils::send_report(connection_info, static_cast<uint16_t>(intermediate_bytes.size()), intermediate_bytes.data());
        }

        const auto &last = chunks.back();
        build_report_payload(report_frame, 0, node_ids.data() + last.nid_offset, last.nid_count, vg_items.data() + last.vg_offset, last.vg_count);

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_groupings_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info,
                                                                                                                                     command_class_multi_channel_association_attribute_map_t attribute_map,
                                                                                                                                     zwave_frame_generator_standalone &report_frame,
                                                                                                                                     std::vector<uint8_t> &frame)
    {
        uint8_t supported_groupings = 0;
        sl_status_t count_status    = SL_STATUS_FAIL;
        try {
            component_connector connector;
            auto count_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, uint8_t>(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT), {});
            auto [future_status, future_result] = count_future.get();
            count_status                        = future_status;
            if (count_status == SL_STATUS_OK) {
                supported_groupings = future_result;
            }
        } catch (const std::exception &e) {
            sl_log_warning(LOG_TAG.data(), "Failed to query supported groupings count: %s", e.what());
        }

        report_frame.add_raw_byte(supported_groupings);
        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association::on_multi_channel_association_remove_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto *frame_generator = args.set_frame_generator;
        auto group_node       = args.node;

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::grouping_identifier));
        if (!grouping_identifier_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(grouping_identifier_node, DESIRED_ATTRIBUTE);

        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::node_id));
        if (!node_id_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(node_id_node, DESIRED_ATTRIBUTE);

        auto marker_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::marker));
        if (!marker_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(marker_node, DESIRED_ATTRIBUTE);

        auto vg_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::vg));
        if (!vg_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(vg_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class