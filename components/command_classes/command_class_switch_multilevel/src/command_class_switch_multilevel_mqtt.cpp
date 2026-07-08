
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
#include "command_class_switch_multilevel.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_switch_multilevel_mqtt";

    command_class_switch_multilevel_mqtt::command_class_switch_multilevel_mqtt()
    {

        mqtt_callback_map.insert({"SwitchMultilevelGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchMultilevelSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchMultilevelStartLevelChange", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_start_level_change_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchMultilevelStopLevelChange", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_stop_level_change_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"SwitchMultilevelSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_supported_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_get_group_attributes_t::SWITCH_MULTILEVEL_GET_GROUP));
        command_class_switch_multilevel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t value    = 0;
        uint8_t duration = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("value", value).parse("duration", duration);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::SWITCH_MULTILEVEL_SET_GROUP));

        auto value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::value));
        value_node.set_desired(value);

        auto duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::duration));
        duration_node.set_desired(duration);

        command_class_switch_multilevel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_start_level_change_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t up_down            = 0;
        uint8_t ignore_start_level = 0;
        uint8_t start_level        = 0;
        uint8_t dimming_duration   = 0;
        uint8_t step_size          = 0;
        uint8_t inc_dec            = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("inc_dec", inc_dec).parse("ignore_start_level", ignore_start_level).parse("up_down", up_down);
        parser.parse("start_level", start_level).parse("dimming_duration", dimming_duration).parse("step_size", step_size);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::SWITCH_MULTILEVEL_START_LEVEL_CHANGE_GROUP));

        auto up_down_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::up_down));
        up_down_node.set_desired(up_down);

        auto ignore_start_level_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::ignore_start_level));
        ignore_start_level_node.set_desired(ignore_start_level);

        auto start_level_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::start_level));
        start_level_node.set_desired(start_level);

        auto dimming_duration_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::dimming_duration));
        dimming_duration_node.set_desired(dimming_duration);

        auto step_size_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::step_size));
        step_size_node.set_desired(step_size);

        auto inc_dec_node = group_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_start_level_change_group_attributes_t::inc_dec));
        inc_dec_node.set_desired(inc_dec);

        command_class_switch_multilevel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_stop_level_change_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_stop_level_change_group_attributes_t::SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE_GROUP));
        command_class_switch_multilevel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(switch_multilevel_supported_get_group_attributes_t::SWITCH_MULTILEVEL_SUPPORTED_GET_GROUP));
        command_class_switch_multilevel_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class