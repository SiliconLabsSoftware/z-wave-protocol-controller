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
 ******************************************************************************/

#include <string_view>

#include "command_class_time.hpp"
#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_time_mqtt";

    command_class_time_mqtt::command_class_time_mqtt()
    {
        mqtt_callback_map.insert({"DateGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_time_mqtt::mqtt_on_date_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"TimeGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_time_mqtt::mqtt_on_time_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"TimeOffsetGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_time_mqtt::mqtt_on_time_offset_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"TimeOffsetSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_time_mqtt::mqtt_on_time_offset_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"DateSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_time_mqtt::mqtt_on_date_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"TimeSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_time_mqtt::mqtt_on_time_set_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_time_mqtt::mqtt_on_date_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(date_get_group_attributes_t::DATE_GET_GROUP));
        command_class_time_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time_mqtt::mqtt_on_time_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_get_group_attributes_t::TIME_GET_GROUP));
        command_class_time_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time_mqtt::mqtt_on_time_offset_get_command(attribute_store::attribute &endpoint_node, std::string /*payload*/)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_get_group_attributes_t::TIME_OFFSET_GET_GROUP));
        command_class_time_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time_mqtt::mqtt_on_time_offset_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t hour_tzo          = 0;
        uint8_t sign_tzo          = 0;
        uint8_t minute_tzo        = 0;
        uint8_t minute_offset_dst = 0;
        uint8_t sign_offset_dst   = 0;
        uint8_t month_start_dst   = 0;
        uint8_t day_start_dst     = 0;
        uint8_t hour_start_dst    = 0;
        uint8_t month_end_dst     = 0;
        uint8_t day_end_dst       = 0;
        uint8_t hour_end_dst      = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("level").parse("hour_tzo", hour_tzo).parse("sign_tzo", sign_tzo);
        parser.parse("minute_tzo", minute_tzo);
        parser.parse_nested("level2").parse("minute_offset_dst", minute_offset_dst).parse("sign_offset_dst", sign_offset_dst);
        parser.parse("month_start_dst", month_start_dst).parse("day_start_dst", day_start_dst).parse("hour_start_dst", hour_start_dst);
        parser.parse("month_end_dst", month_end_dst).parse("day_end_dst", day_end_dst).parse("hour_end_dst", hour_end_dst);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::TIME_OFFSET_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::hour_tzo)).set_desired(hour_tzo);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::sign_tzo)).set_desired(sign_tzo);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::minute_tzo)).set_desired(minute_tzo);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::minute_offset_dst)).set_desired(minute_offset_dst);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::sign_offset_dst)).set_desired(sign_offset_dst);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::month_start_dst)).set_desired(month_start_dst);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::day_start_dst)).set_desired(day_start_dst);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::hour_start_dst)).set_desired(hour_start_dst);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::month_end_dst)).set_desired(month_end_dst);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::day_end_dst)).set_desired(day_end_dst);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_offset_set_group_attributes_t::hour_end_dst)).set_desired(hour_end_dst);
        command_class_time_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time_mqtt::mqtt_on_date_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t year = 0;
        uint8_t month = 0;
        uint8_t day   = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("year", year).parse("month", month).parse("day", day);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(date_set_group_attributes_t::DATE_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(date_set_group_attributes_t::year)).set_desired(year);
        group_node.emplace_node(static_cast<attribute_store_type_t>(date_set_group_attributes_t::month)).set_desired(month);
        group_node.emplace_node(static_cast<attribute_store_type_t>(date_set_group_attributes_t::day)).set_desired(day);
        command_class_time_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_time_mqtt::mqtt_on_time_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t hour   = 0;
        uint8_t minute = 0;
        uint8_t second = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse_nested("properties1").parse("hour", hour);
        parser.parse("minute", minute).parse("second", second);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(time_set_group_attributes_t::TIME_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_set_group_attributes_t::hour)).set_desired(hour);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_set_group_attributes_t::minute)).set_desired(minute);
        group_node.emplace_node(static_cast<attribute_store_type_t>(time_set_group_attributes_t::second)).set_desired(second);
        command_class_time_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
