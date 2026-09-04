
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
#include <vector>

// Base class
#include "command_class_multi_channel_association.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "zwave_command_class_mqtt_utils.hpp"
#include "command_class_association_grp_info_attributes.hpp"
#include "command_class_multi_channel_generated_types.hpp"
#include "zpc_attribute_store_network_helper.h"
#include "zwave_controller_keyset.h"
#include "zwave_utils.h"
#include "ZW_classcmd.h"
#include "log.h"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_multi_channel_association_mqtt";

    namespace
    {
        // CL:008E.02.51.01.1 / CL:008E.04.51.01.1 — security class rules for Node A (group owner) → Node B (destination).
        bool destination_security_compatible(const attribute_store::attribute &source_endpoint, zwave_node_id_t destination_node_id)
        {
            zwave_node_id_t source_node_id = 0;
            zwave_endpoint_id_t unused_ep  = 0;
            if (attribute_store_network_helper_get_zwave_ids_from_node(source_endpoint, &source_node_id, &unused_ep) != SL_STATUS_OK) {
                return true;
            }

            zwave_keyset_t keys_src  = 0;
            zwave_keyset_t keys_dest = 0;
            if (zwave_get_node_granted_keys(source_node_id, &keys_src) != SL_STATUS_OK || zwave_get_node_granted_keys(destination_node_id, &keys_dest) != SL_STATUS_OK) {
                return true;
            }

            const auto highest_src    = zwave_controller_get_highest_encapsulation(keys_src);
            const auto highest_dest   = zwave_controller_get_highest_encapsulation(keys_dest);
            const uint8_t mca_version = zwave_node_get_command_class_version(COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3, source_node_id, 0);

            if (mca_version >= MULTI_CHANNEL_ASSOCIATION_VERSION_V4) {
                // NONE is not a Security Class, so the requirement applies only when the destination has one.
                if (highest_dest != ZWAVE_CONTROLLER_ENCAPSULATION_NONE) {
                    const zwave_keyset_t dest_highest_key = zwave_controller_get_key_from_encapsulation(highest_dest);
                    if ((keys_src & dest_highest_key) == 0) {
                        // CL:008E.04.51.01.1 — MUST NOT associate if Node A was not granted Node B's highest Security Class.
                        sl_log_info(LOG_TAG.data(), "Set ignored: NodeID %d was not granted NodeID %d's highest Security Class", source_node_id, destination_node_id);
                        return false;
                    }
                }
            } else if (mca_version >= MULTI_CHANNEL_ASSOCIATION_VERSION_V2) {
                if (highest_src != highest_dest) {
                    // CL:008E.02.51.01.1 — v2/v3: highest Security Classes must be identical.
                    sl_log_info(LOG_TAG.data(), "Set ignored: NodeID %d and NodeID %d highest Security Classes are not identical", source_node_id, destination_node_id);
                    return false;
                }
            }
            return true;
        }

        // CL:008E.01.51.03.3 — reject destinations that do not support a CC the group controls.
        bool destination_supports_group_ccs(const attribute_store::attribute &source_endpoint, uint8_t grouping_identifier, zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id)
        {
            auto group = source_endpoint.child_by_type_and_value(static_cast<attribute_store_type_t>(agi_group_attributes_t::AGI_NODE_GROUP), grouping_identifier);
            if (!group.is_valid()) {
                // No AGI group: we don't know which CCs the group controls, so skip this check.
                return true;
            }
            auto cmd_list = group.child_by_type(static_cast<attribute_store_type_t>(agi_group_attributes_t::group_command_list));
            if (!cmd_list.is_valid() || !cmd_list.reported_exists()) {
                // No AGI command list: we don't know which CCs the group controls, so skip this check.
                return true;
            }

            const auto commands = cmd_list.reported<std::vector<uint8_t>>();

            // Pre-load Capability Report CC list once (endpoints often only expose CCs there).
            using mc_t = command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t;
            std::vector<uint8_t> cap_ccs;
            attribute_store::attribute ep_node(zwave_get_endpoint_node(node_id, endpoint_id));
            auto cap = ep_node.child_by_type(static_cast<attribute_store_type_t>(mc_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP));
            if (cap.is_valid()) {
                auto ccs = cap.child_by_type(static_cast<attribute_store_type_t>(mc_t::command_class));
                if (ccs.is_valid() && ccs.reported_exists()) {
                    cap_ccs = ccs.reported<std::vector<uint8_t>>();
                }
            }

            // Accept if the destination supports at least one CC the group controls.
            // Reject only when none of the listed CCs are supported (CL:008E.01.51.03.3).
            for (size_t i = 0; i + 1 < commands.size(); i += 2) {
                const uint8_t cc = commands[i];
                if (zwave_node_supports_command_class(cc, node_id, endpoint_id)) {
                    return true;
                }
                if (std::find(cap_ccs.begin(), cap_ccs.end(), cc) != cap_ccs.end()) {
                    return true;
                }
            }

            if (commands.size() >= 2) {
                sl_log_info(LOG_TAG.data(), "Set ignored: NodeID %d Endpoint %d supports none of association group %d's Command Classes", node_id, endpoint_id, grouping_identifier);
                return false;
            }
            // Empty command list: the group advertises no commands, so the requirement doesn't apply.
            return true;
        }
    }  // namespace

    command_class_multi_channel_association_mqtt::command_class_multi_channel_association_mqtt()
    {

        mqtt_callback_map.insert({"MultiChannelAssociationGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiChannelAssociationGroupingsGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_groupings_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiChannelAssociationRemove", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_remove_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiChannelAssociationSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "MultiChannelAssociationGet command received");

        uint8_t desired_grouping_identifier = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GET_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        command_class_multi_channel_association_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_groupings_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "MultiChannelAssociationGroupingsGet command received");

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_groupings_get_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET_GROUP));
        command_class_multi_channel_association_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_remove_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "MultiChannelAssociationRemove command received");

        uint8_t desired_grouping_identifier = 0;
        std::vector<uint8_t> desired_node_id;
        uint8_t marker = 0;
        std::vector<multi_channel_association_remove_vg_t_item_t> vg;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier).parse("node_id", desired_node_id).parse("marker", marker);
        for (auto &&[elem, vg_item]: parser.parse_array("vg", vg)) {
            uint8_t end_point   = 0;
            uint8_t bit_address = 0;
            elem.parse("multi_channel_node_id", vg_item.multi_channel_node_id);
            elem.parse_nested("properties1").parse("end_point", end_point).parse("bit_address", bit_address);
            vg_item.properties1.flags.multi_channel_association_remove_end_point   = end_point;
            vg_item.properties1.flags.multi_channel_association_remove_bit_address = bit_address;
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_REMOVE_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::node_id));
        node_id_node.set_desired(desired_node_id);

        auto marker_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::marker));
        marker_node.set_desired(marker);

        auto vg_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_remove_group_attributes_t::vg));
        vg_node.set_desired(vg);

        command_class_multi_channel_association_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_association_mqtt::mqtt_on_multi_channel_association_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "MultiChannelAssociationSet command received");

        uint8_t desired_grouping_identifier = 0;
        std::vector<uint8_t> desired_node_id;
        uint8_t marker = 0;
        std::vector<multi_channel_association_set_vg_t_item_t> vg;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier).parse("node_id", desired_node_id).parse("marker", marker);
        for (auto &&[elem, vg_item]: parser.parse_array("vg", vg)) {
            uint8_t end_point   = 0;
            uint8_t bit_address = 0;
            elem.parse("multi_channel_node_id", vg_item.multi_channel_node_id);
            elem.parse_nested("properties1").parse("end_point", end_point).parse("bit_address", bit_address);
            vg_item.properties1.flags.multi_channel_association_set_end_point   = end_point;
            vg_item.properties1.flags.multi_channel_association_set_bit_address = bit_address;
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        for (uint8_t node_id: desired_node_id) {
            if (!destination_security_compatible(endpoint_node, node_id) || !destination_supports_group_ccs(endpoint_node, desired_grouping_identifier, node_id, 0)) {
                return SL_STATUS_FAIL;
            }
        }
        for (const auto &vg_item: vg) {
            if (vg_item.properties1.flags.multi_channel_association_set_bit_address != 0) {
                continue;  // bit-addressed sets are not validated here
            }
            if (!destination_security_compatible(endpoint_node, vg_item.multi_channel_node_id) || !destination_supports_group_ccs(endpoint_node, desired_grouping_identifier, vg_item.multi_channel_node_id, vg_item.properties1.flags.multi_channel_association_set_end_point)) {
                return SL_STATUS_FAIL;
            }
        }

        auto group_node               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::MULTI_CHANNEL_ASSOCIATION_SET_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::node_id));
        node_id_node.set_desired(desired_node_id);

        auto marker_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::marker));
        marker_node.set_desired(marker);

        auto vg_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_association_set_group_attributes_t::vg));
        vg_node.set_desired(vg);

        command_class_multi_channel_association_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
