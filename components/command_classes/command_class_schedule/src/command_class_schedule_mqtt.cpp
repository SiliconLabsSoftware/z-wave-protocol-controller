
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

#include <string_view>
#include <vector>

#include "command_class_schedule.hpp"

#include "zpc_mqtt.hpp"
#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_schedule_mqtt";

    command_class_schedule_mqtt::command_class_schedule_mqtt()
    {
        mqtt_callback_map.insert({"ScheduleSupportedGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_schedule_mqtt::mqtt_on_schedule_supported_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CommandScheduleSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"CommandScheduleGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_schedule_mqtt::mqtt_on_command_schedule_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ScheduleRemove", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_schedule_mqtt::mqtt_on_schedule_remove_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ScheduleStateSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_schedule_mqtt::mqtt_on_schedule_state_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ScheduleStateGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_schedule_mqtt::mqtt_on_schedule_state_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ScheduleSupportedCommandsGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_schedule_mqtt::mqtt_on_schedule_supported_commands_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_schedule_mqtt::mqtt_on_schedule_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t schedule_id_block = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("schedule_id_block", schedule_id_block);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);
        command_class_schedule_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t schedule_id        = 0;
        uint8_t schedule_id_block  = 0;
        uint8_t start_year         = 0;
        uint8_t start_month        = 0;
        uint8_t recurrence_offset  = 0;
        uint8_t start_day_of_month = 0;
        uint8_t recurrence_mode    = 0;
        uint8_t start_weekday      = 0;
        uint8_t start_hour         = 0;
        uint8_t duration_type      = 0;
        uint8_t start_minute       = 0;
        uint8_t relative           = 0;
        uint16_t duration_byte     = 0;
        uint8_t reports_to_follow  = 0;
        std::vector<command_schedule_set_vg1_t_item_t> vg1_items;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("schedule_id", schedule_id).parse("schedule_id_block", schedule_id_block).parse("start_year", start_year).parse("duration_byte", duration_byte).parse("reports_to_follow", reports_to_follow);
        parser.parse_nested("properties1").parse("start_month", start_month).parse("recurrence_offset", recurrence_offset);
        parser.parse_nested("properties2").parse("start_day_of_month", start_day_of_month).parse("recurrence_mode", recurrence_mode);
        parser.parse_nested("properties3").parse("start_weekday", start_weekday);
        parser.parse_nested("properties4").parse("start_hour", start_hour).parse("duration_type", duration_type);
        parser.parse_nested("properties5").parse("start_minute", start_minute).parse("relative", relative);
        for (auto &&[elem, vg1_item]: parser.parse_array("vg1", vg1_items)) {
            elem.parse("cmd_byte", vg1_item.cmd_byte);
            vg1_item.cmd_length = static_cast<uint8_t>(vg1_item.cmd_byte.size());
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto number_of_cmd_to_follow = static_cast<uint8_t>(vg1_items.size());

        // Flatten command list for attribute-store storage (variable-length cmd_byte is not POD).
        std::vector<uint8_t> vg1_bytes;
        for (const auto &item: vg1_items) {
            vg1_bytes.push_back(static_cast<uint8_t>(item.cmd_byte.size()));
            vg1_bytes.insert(vg1_bytes.end(), item.cmd_byte.begin(), item.cmd_byte.end());
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::COMMAND_SCHEDULE_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::schedule_id)).set_desired(schedule_id);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_year)).set_desired(start_year);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_month)).set_desired(start_month);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::recurrence_offset)).set_desired(recurrence_offset);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_day_of_month)).set_desired(start_day_of_month);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::recurrence_mode)).set_desired(recurrence_mode);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_weekday)).set_desired(start_weekday);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_hour)).set_desired(start_hour);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::duration_type)).set_desired(duration_type);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_minute)).set_desired(start_minute);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::relative)).set_desired(relative);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::duration_byte)).set_desired(duration_byte);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::reports_to_follow)).set_desired(reports_to_follow);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::number_of_cmd_to_follow)).set_desired(number_of_cmd_to_follow);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::vg1)).set_desired(vg1_bytes);

        command_class_schedule_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule_mqtt::mqtt_on_command_schedule_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t schedule_id       = 0;
        uint8_t schedule_id_block = 0;
        uint8_t aid_ro_ctl        = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("schedule_id", schedule_id).parse("schedule_id_block", schedule_id_block);
        parser.parse_nested("properties1").parse("aid_ro_ctl", aid_ro_ctl);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::COMMAND_SCHEDULE_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::schedule_id)).set_desired(schedule_id);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);
        group_node.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::aid_ro_ctl)).set_desired(aid_ro_ctl);

        command_class_schedule_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule_mqtt::mqtt_on_schedule_remove_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t schedule_id       = 0;
        uint8_t schedule_id_block = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("schedule_id", schedule_id).parse("schedule_id_block", schedule_id_block);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::SCHEDULE_REMOVE_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::schedule_id)).set_desired(schedule_id);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);

        command_class_schedule_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule_mqtt::mqtt_on_schedule_state_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t schedule_id       = 0;
        uint8_t schedule_state    = 0;
        uint8_t schedule_id_block = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("schedule_id", schedule_id).parse("schedule_state", schedule_state).parse("schedule_id_block", schedule_id_block);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::SCHEDULE_STATE_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_id)).set_desired(schedule_id);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_state)).set_desired(schedule_state);
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);

        command_class_schedule_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule_mqtt::mqtt_on_schedule_state_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t schedule_id_block = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("schedule_id_block", schedule_id_block);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_get_group_attributes_t::SCHEDULE_STATE_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_state_get_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);

        command_class_schedule_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_schedule_mqtt::mqtt_on_schedule_supported_commands_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t schedule_id_block = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("schedule_id_block", schedule_id_block);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::schedule_id_block)).set_desired(schedule_id_block);

        command_class_schedule_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
