
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
#include "command_class_indicator.hpp"
#include "command_class_indicator_mqtt.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_indicator_mqtt";

    command_class_indicator_mqtt::command_class_indicator_mqtt()
    {

        mqtt_callback_map.insert({"IndicatorGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_indicator_mqtt::mqtt_on_indicator_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"IndicatorSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_indicator_mqtt::mqtt_on_indicator_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"IndicatorSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_indicator_mqtt::mqtt_on_indicator_supported_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"IndicatorDescriptionGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_indicator_mqtt::mqtt_on_indicator_description_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_indicator_mqtt::mqtt_on_indicator_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t indicator_id = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("indicator_id", indicator_id);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node        = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::INDICATOR_GET_GROUP));
        auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::indicator_id));
        indicator_id_node.set_desired(indicator_id);

        command_class_indicator_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator_mqtt::mqtt_on_indicator_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t indicator_0_value      = 0;
        uint8_t indicator_object_count = 0;
        std::vector<indicator_set_vg1_t_item_t> vg1_items;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("indicator_0_value", indicator_0_value);
        parser.parse_nested("properties1").parse("indicator_object_count", indicator_object_count);
        for (auto &&[elem, vg1_item]: parser.parse_array("vg1", vg1_items)) {
            elem.parse("indicator_id", vg1_item.indicator_id).parse("property_id", vg1_item.property_id).parse("value", vg1_item.value);
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_set_group_attributes_t::INDICATOR_SET_GROUP));

        auto indicator_0_value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_set_group_attributes_t::indicator_0_value));
        indicator_0_value_node.set_desired(indicator_0_value);

        auto indicator_object_count_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_set_group_attributes_t::indicator_object_count));
        indicator_object_count_node.set_desired(indicator_object_count);

        if (indicator_object_count > 0) {
            auto vg1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_set_group_attributes_t::vg1));
            vg1_node.set_desired(vg1_items);
        }

        command_class_indicator_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator_mqtt::mqtt_on_indicator_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t indicator_id = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("indicator_id", indicator_id);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node        = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_get_group_attributes_t::INDICATOR_SUPPORTED_GET_GROUP));
        auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_get_group_attributes_t::indicator_id));
        indicator_id_node.set_desired(indicator_id);

        command_class_indicator_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator_mqtt::mqtt_on_indicator_description_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t indicator_id = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("indicator_id", indicator_id);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node        = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_description_get_group_attributes_t::INDICATOR_DESCRIPTION_GET_GROUP));
        auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_description_get_group_attributes_t::indicator_id));
        indicator_id_node.set_desired(indicator_id);

        command_class_indicator_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class