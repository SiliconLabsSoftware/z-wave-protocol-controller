
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
#include "command_class_multi_channel.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_multi_channel_mqtt";

    command_class_multi_channel_mqtt::command_class_multi_channel_mqtt()
    {

        mqtt_callback_map.insert({"MultiChannelCapabilityGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_mqtt::mqtt_on_multi_channel_capability_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiChannelCmdEncap", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_mqtt::mqtt_on_multi_channel_cmd_encap_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiChannelEndPointFind", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_mqtt::mqtt_on_multi_channel_end_point_find_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiChannelEndPointGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_mqtt::mqtt_on_multi_channel_end_point_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiInstanceCmdEncap", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_mqtt::mqtt_on_multi_instance_cmd_encap_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiInstanceGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_mqtt::mqtt_on_multi_instance_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MultiChannelAggregatedMembersGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_multi_channel_mqtt::mqtt_on_multi_channel_aggregated_members_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_multi_channel_mqtt::mqtt_on_multi_channel_capability_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_info(LOG_TAG.data(), "MQTT command received: MultiChannelCapabilityGet");

        uint8_t parsed_end_point = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("end_point", parsed_end_point);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        attribute_store::attribute desired_endpoint_node;
        if (parsed_end_point > 0) {
            desired_endpoint_node = endpoint_node.parent().emplace_node(ATTRIBUTE_ENDPOINT_ID, parsed_end_point);

            auto group_node = desired_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_get_group_attributes_t::MULTI_CHANNEL_CAPABILITY_GET_GROUP));

            auto end_point_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_capability_get_group_attributes_t::end_point));
            end_point_node.set_desired(parsed_end_point);

            command_class_multi_channel_core::start_group_resolution(group_node);
        } else {
            desired_endpoint_node = endpoint_node;
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_mqtt::mqtt_on_multi_channel_cmd_encap_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_warning(LOG_TAG.data(), "MultiChannelCmdEncap command not implemented");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_mqtt::mqtt_on_multi_channel_end_point_find_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t generic_device_class  = 0;
        uint8_t specific_device_class = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("generic_device_class", generic_device_class).parse("specific_device_class", specific_device_class);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::MULTI_CHANNEL_END_POINT_FIND_GROUP));

        auto generic_device_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::generic_device_class));
        generic_device_class_node.set_desired(generic_device_class);

        auto specific_device_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_find_group_attributes_t::specific_device_class));
        specific_device_class_node.set_desired(specific_device_class);

        command_class_multi_channel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_mqtt::mqtt_on_multi_channel_end_point_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_info(LOG_TAG.data(), "MultiChannelEndPointGet command received");

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_end_point_get_group_attributes_t::MULTI_CHANNEL_END_POINT_GET_GROUP));
        command_class_multi_channel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_mqtt::mqtt_on_multi_instance_cmd_encap_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_warning(LOG_TAG.data(), "MultiInstanceCmdEncap command not implemented");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_mqtt::mqtt_on_multi_instance_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_warning(LOG_TAG.data(), "MultiInstanceGet command not implemented");
        return SL_STATUS_OK;
    }

    sl_status_t command_class_multi_channel_mqtt::mqtt_on_multi_channel_aggregated_members_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_info(LOG_TAG.data(), "MultiChannelAggregatedMembersGet command received");

        uint8_t aggregated_end_point = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("aggregated_end_point", aggregated_end_point);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node                = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_aggregated_members_get_group_attributes_t::MULTI_CHANNEL_AGGREGATED_MEMBERS_GET_GROUP));
        auto aggregated_end_point_node = group_node.emplace_node(static_cast<attribute_store_type_t>(multi_channel_aggregated_members_get_group_attributes_t::aggregated_end_point));
        aggregated_end_point_node.set_desired(aggregated_end_point);

        command_class_multi_channel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class