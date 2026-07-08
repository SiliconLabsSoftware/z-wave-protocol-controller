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
#include "command_class_switch_color.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_color_mqtt";

    command_class_switch_color_mqtt::command_class_switch_color_mqtt()
    {
        mqtt_callback_map.insert({"SwitchColorSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_color_mqtt::mqtt_on_switch_color_supported_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchColorGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_color_mqtt::mqtt_on_switch_color_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchColorSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_color_mqtt::mqtt_on_switch_color_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchColorStartLevelChange", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_color_mqtt::mqtt_on_switch_color_start_level_change_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchColorStopLevelChange", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_color_mqtt::mqtt_on_switch_color_stop_level_change_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_switch_color_mqtt::mqtt_on_switch_color_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_supported_get_group_attributes_t::SWITCH_COLOR_SUPPORTED_GET_GROUP));
        command_class_switch_color_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_color_mqtt::mqtt_on_switch_color_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t color_component_id = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("color_component_id", color_component_id);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node              = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_get_group_attributes_t::SWITCH_COLOR_GET_GROUP));
        auto color_component_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_get_group_attributes_t::color_component_id));
        color_component_id_node.set_desired(color_component_id);

        command_class_switch_color_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_color_mqtt::mqtt_on_switch_color_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t duration = 0;
        std::vector<switch_color_set_vg1_t_item_t> vg1_items;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("duration", duration);
        for (auto &&[elem, vg1_item]: parser.parse_array("vg1", vg1_items)) {
            elem.parse("color_component_id", vg1_item.color_component_id).parse("value", vg1_item.value);
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto color_component_count = static_cast<uint8_t>(vg1_items.size());

        auto group_node                 = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_set_group_attributes_t::SWITCH_COLOR_SET_GROUP));
        auto color_component_count_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_set_group_attributes_t::color_component_count));
        color_component_count_node.set_desired(color_component_count);

        auto vg1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_set_group_attributes_t::vg1));
        vg1_node.set_desired(vg1_items);

        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_set_group_attributes_t::duration));
        duration_node.set_desired(duration);

        command_class_switch_color_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_color_mqtt::mqtt_on_switch_color_start_level_change_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t ignore_start_state = 0;
        uint8_t up_down            = 0;
        uint8_t color_component_id = 0;
        uint8_t start_level        = 0;
        uint8_t duration           = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("ignore_start_state", ignore_start_state).parse("up_down", up_down);
        parser.parse("color_component_id", color_component_id).parse("start_level", start_level).parse("duration", duration);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node              = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::SWITCH_COLOR_START_LEVEL_CHANGE_GROUP));
        auto ignore_start_state_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::ignore_start_state));
        ignore_start_state_node.set_desired(ignore_start_state);
        auto up_down_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::up_down));
        up_down_node.set_desired(up_down);
        auto color_component_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::color_component_id));
        color_component_id_node.set_desired(color_component_id);
        auto start_level_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::start_level));
        start_level_node.set_desired(start_level);
        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_start_level_change_group_attributes_t::duration));
        duration_node.set_desired(duration);

        command_class_switch_color_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_color_mqtt::mqtt_on_switch_color_stop_level_change_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t color_component_id = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("color_component_id", color_component_id);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node              = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_stop_level_change_group_attributes_t::SWITCH_COLOR_STOP_LEVEL_CHANGE_GROUP));
        auto color_component_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_color_stop_level_change_group_attributes_t::color_component_id));
        color_component_id_node.set_desired(color_component_id);

        command_class_switch_color_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class