
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

#include "command_class_association_grp_info.hpp"

#include "zpc_mqtt.hpp"
#include "zpc_mqtt_utils.hpp"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_association_grp_info_mqtt";

    command_class_association_grp_info_mqtt::command_class_association_grp_info_mqtt()
    {
        mqtt_callback_map.insert({"AssociationGroupNameGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_grp_info_mqtt::mqtt_on_association_group_name_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AssociationGroupInfoGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_grp_info_mqtt::mqtt_on_association_group_info_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"AssociationGroupCommandListGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      zwave_command_class::command_class_association_grp_info_mqtt::mqtt_on_association_group_command_list_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_association_grp_info_mqtt::mqtt_on_association_group_name_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGroupNameGet MQTT command received");

        uint8_t desired_grouping_identifier = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::ASSOCIATION_GROUP_NAME_GET_GROUP));

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_name_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        command_class_association_grp_info_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info_mqtt::mqtt_on_association_group_info_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGroupInfoGet MQTT command received");

        uint8_t desired_grouping_identifier = 0;
        uint8_t list_mode                   = 0;
        uint8_t refresh_cache               = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier).parse_optional("list_mode", list_mode).parse_optional("refresh_cache", refresh_cache);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::ASSOCIATION_GROUP_INFO_GET_GROUP));

        auto list_mode_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::list_mode));
        list_mode_node.set_desired(list_mode);

        auto refresh_cache_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::refresh_cache));
        refresh_cache_node.set_desired(refresh_cache);

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_info_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        command_class_association_grp_info_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_grp_info_mqtt::mqtt_on_association_group_command_list_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        sl_log_debug(LOG_TAG.data(), "AssociationGroupCommandListGet MQTT command received");

        uint8_t desired_grouping_identifier = 0;
        uint8_t allow_cache                 = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("grouping_identifier", desired_grouping_identifier).parse_optional("allow_cache", allow_cache);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::ASSOCIATION_GROUP_COMMAND_LIST_GET_GROUP));

        auto allow_cache_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::allow_cache));
        allow_cache_node.set_desired(allow_cache);

        auto grouping_identifier_node = group_node.emplace_node(static_cast<attribute_store_type_t>(association_group_command_list_get_group_attributes_t::grouping_identifier));
        grouping_identifier_node.set_desired(desired_grouping_identifier);

        command_class_association_grp_info_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
