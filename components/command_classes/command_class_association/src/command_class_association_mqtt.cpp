
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

#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_association.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_association_mqtt";

    command_class_association_mqtt::command_class_association_mqtt()
    {

        mqtt_callback_map.insert({"AssociationGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_mqtt::mqtt_on_association_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AssociationGroupingsGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_mqtt::mqtt_on_association_groupings_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AssociationRemove", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_mqtt::mqtt_on_association_remove_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AssociationSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_mqtt::mqtt_on_association_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AssociationSpecificGroupGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_mqtt::mqtt_on_association_specific_group_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_association_mqtt::mqtt_on_association_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGet command received");

        uint8_t desired_grouping_identifier = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::ASSOCIATION_GET_GROUP));

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        command_class_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_mqtt::mqtt_on_association_groupings_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGroupingsGet command received");

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_groupings_get_group_attributes_t::ASSOCIATION_GROUPINGS_GET_GROUP));
        command_class_association_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_mqtt::mqtt_on_association_remove_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationRemove command received");

        uint8_t desired_grouping_identifier = 0;
        std::vector<uint8_t> node_id_to_remove;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier).parse("node_id", node_id_to_remove);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_remove_group_attributes_t::ASSOCIATION_REMOVE_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_remove_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_remove_group_attributes_t::node_id));
        node_id_node.set_desired(node_id_to_remove);

        command_class_association_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_mqtt::mqtt_on_association_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationSet command received");

        uint8_t desired_grouping_identifier = 0;
        std::vector<uint8_t> desired_node_id;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier).parse("node_id", desired_node_id);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::ASSOCIATION_SET_GROUP));
        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);
        auto node_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_set_group_attributes_t::node_id));
        node_id_node.set_desired(desired_node_id);

        command_class_association_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_mqtt::mqtt_on_association_specific_group_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationSpecificGroupGet command received");
        sl_log_debug(LOG_TAG.data(), "NOT IMPLEMENTED");
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class