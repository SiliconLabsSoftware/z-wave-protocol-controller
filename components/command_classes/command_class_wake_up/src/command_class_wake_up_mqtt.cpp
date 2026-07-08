
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
#include <iomanip>
#include <stdexcept>
#include <string_view>

// Base class
#include "command_class_wake_up.hpp"

// MQTT
#include "sl_status.h"
#include "zpc_mqtt.hpp"  // zpc_mqtt::publish_report
#include "attribute_resolver.h"

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_wake_up_mqtt";

    command_class_wake_up_mqtt::command_class_wake_up_mqtt()
    {

        mqtt_callback_map.insert({"WakeUpIntervalCapabilitiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_wake_up_mqtt::mqtt_on_wake_up_interval_capabilities_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"WakeUpIntervalGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_wake_up_mqtt::mqtt_on_wake_up_interval_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"WakeUpIntervalSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_wake_up_mqtt::mqtt_on_wake_up_interval_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"WakeUpNoMoreInformation", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_wake_up_mqtt::mqtt_on_wake_up_no_more_information_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_wake_up_mqtt::mqtt_on_wake_up_interval_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_get_group_attributes_t::WAKE_UP_INTERVAL_CAPABILITIES_GET_GROUP));

        command_class_wake_up_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up_mqtt::mqtt_on_wake_up_interval_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_get_group_attributes_t::WAKE_UP_INTERVAL_GET_GROUP));

        command_class_wake_up_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up_mqtt::mqtt_on_wake_up_interval_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        wake_up_interval_set_seconds_t seconds = 0;
        wake_up_interval_set_nodeid_t nodeid   = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("seconds", seconds).parse("nodeid", nodeid);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::WAKE_UP_INTERVAL_SET_GROUP));

        auto seconds_node = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::seconds));
        seconds_node.set_desired<wake_up_interval_set_seconds_t>(seconds);

        auto nodeid_node = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_set_group_attributes_t::nodeid));
        nodeid_node.set_desired<wake_up_interval_set_nodeid_t>(nodeid);

        // After set, Interval Get is queued (network monitor). Do not use the interview resolution callback:
        // that fires COMMAND_CLASS_WAKE_UP_INTERVAL_SET_INTERVIEW_RESOLUTION_COMPLETED and would advance WakeUpStep spuriously.
        attribute_resolver_set_resolution_listener(group_node, command_class_wake_up::on_wake_up_interval_set_user_resolution);
        command_class_wake_up_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up_mqtt::mqtt_on_wake_up_no_more_information_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_no_more_information_group_attributes_t::WAKE_UP_NO_MORE_INFORMATION_GROUP));

        command_class_wake_up_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class