
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
#include "command_class_barrier_operator.hpp"
#include "command_class_barrier_operator_constants.hpp"

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

// MQTT
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_barrier_operator_mqtt";

    using namespace command_class_barrier_operator_constants;

    bool command_class_barrier_operator_mqtt::is_supported_subsystem_type(uint8_t subsystem_type)
    {
        return subsystem_type == BARRIER_OPERATOR_SUBSYSTEM_AUDIBLE || subsystem_type == BARRIER_OPERATOR_SUBSYSTEM_VISUAL;
    }

    command_class_barrier_operator_mqtt::command_class_barrier_operator_mqtt()
    {

        mqtt_callback_map.insert({"BarrierOperatorSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_barrier_operator_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"BarrierOperatorGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_barrier_operator_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"BarrierOperatorSignalSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_barrier_operator_signal_supported_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"BarrierOperatorEventSignalSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_barrier_operator_event_signal_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"BarrierOperatorEventSignalingGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      mqtt_on_barrier_operator_event_signaling_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_barrier_operator_mqtt::mqtt_on_barrier_operator_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t target_value = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("target_value", target_value);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (target_value != BARRIER_OPERATOR_TARGET_CLOSE && target_value != BARRIER_OPERATOR_TARGET_OPEN) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_set_group_attributes_t::BARRIER_OPERATOR_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_set_group_attributes_t::target_value)).set_desired(target_value);
        command_class_barrier_operator_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator_mqtt::mqtt_on_barrier_operator_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        command_class_barrier_operator_core::start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_get_group_attributes_t::BARRIER_OPERATOR_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator_mqtt::mqtt_on_barrier_operator_signal_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        command_class_barrier_operator_core::start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_signal_supported_get_group_attributes_t::BARRIER_OPERATOR_SIGNAL_SUPPORTED_GET_GROUP)));
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator_mqtt::mqtt_on_barrier_operator_event_signal_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t subsystem_type  = 0;
        uint8_t subsystem_state = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("subsystem_type", subsystem_type).parse("subsystem_state", subsystem_state);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!is_supported_subsystem_type(subsystem_type) || (subsystem_state != BARRIER_OPERATOR_SUBSYSTEM_OFF && subsystem_state != BARRIER_OPERATOR_SUBSYSTEM_ON)) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNAL_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_type)).set_desired(subsystem_type);
        group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_state)).set_desired(subsystem_state);
        command_class_barrier_operator_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_barrier_operator_mqtt::mqtt_on_barrier_operator_event_signaling_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t subsystem_type = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("subsystem_type", subsystem_type);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (!is_supported_subsystem_type(subsystem_type)) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNALING_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::subsystem_type)).set_desired(subsystem_type);
        command_class_barrier_operator_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
