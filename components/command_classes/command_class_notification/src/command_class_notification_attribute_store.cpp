
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

#include "command_class_notification_attribute_store.hpp"
#include "command_class_notification_attributes.hpp"

namespace zwave_command_class
{

    command_class_notification_attribute_store::command_class_notification_attribute_store()
    {
        using namespace zwave_command_class;
        register_attribute_types({
          {static_cast<attribute_store_type_t>(notification_cc_attributes_t::NOTIFICATION_CC_GROUP), "Notification CC Group", ATTRIBUTE_ENDPOINT_ID, EMPTY_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(notification_cc_attributes_t::notification_mode), "Notification Mode", static_cast<attribute_store_type_t>(notification_cc_attributes_t::NOTIFICATION_CC_GROUP), U8_STORAGE_TYPE},
        });
    }

    sl_status_t command_class_notification_attribute_store::on_notification_report_received_store(attribute_store::attribute endpoint_node, command_class_notification_attribute_map_t attribute_map)
    {
        notification_report_v1_alarm_type_t v1_alarm_type     = 0;
        notification_report_v1_alarm_level_t v1_alarm_level   = 0;
        notification_report_notification_status_t status      = 0;
        notification_report_notification_type_t notif_type    = 0;
        notification_report_event_t event                     = 0;
        uint8_t event_parameters_length                       = 0;
        uint8_t sequence                                      = 0;
        notification_report_event_parameter_t event_parameter = {};
        notification_report_sequence_number_t sequence_number = 0;

        v1_alarm_type           = get_value_or_default(attribute_map, "v1_alarm_type", v1_alarm_type);
        v1_alarm_level          = get_value_or_default(attribute_map, "v1_alarm_level", v1_alarm_level);
        status                  = get_value_or_default(attribute_map, "notification_status", status);
        notif_type              = get_value_or_default(attribute_map, "notification_type", notif_type);
        event                   = get_value_or_default(attribute_map, "event", event);
        event_parameters_length = get_value_or_default(attribute_map, "event_parameters_length", event_parameters_length);
        sequence                = get_value_or_default(attribute_map, "sequence", sequence);
        event_parameter         = get_value_or_default(attribute_map, "event_parameter", event_parameter);
        sequence_number         = get_value_or_default(attribute_map, "sequence_number", sequence_number);

        auto parent_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::NOTIFICATION_REPORT_GROUP));

        auto v1_alarm_type_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::v1_alarm_type));
        v1_alarm_type_node.set_reported<notification_report_v1_alarm_type_t>(v1_alarm_type);

        auto v1_alarm_level_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::v1_alarm_level));
        v1_alarm_level_node.set_reported<notification_report_v1_alarm_level_t>(v1_alarm_level);

        auto status_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::notification_status));
        status_node.set_reported<notification_report_notification_status_t>(status);

        auto notif_type_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::notification_type));
        notif_type_node.set_reported<notification_report_notification_type_t>(notif_type);

        auto event_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::event));
        event_node.set_reported<notification_report_event_t>(event);

        auto event_params_len_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::event_parameters_length));
        event_params_len_node.set_reported<uint8_t>(event_parameters_length);

        auto sequence_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::sequence));
        sequence_node.set_reported<uint8_t>(sequence);

        auto event_param_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::event_parameter));
        event_param_node.set_reported<notification_report_event_parameter_t>(event_parameter);

        auto seq_num_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_report_group_attributes_t::sequence_number));
        seq_num_node.set_reported<notification_report_sequence_number_t>(sequence_number);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification_attribute_store::on_notification_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_notification_attribute_map_t attribute_map)
    {
        uint8_t number_of_bit_masks                       = 0;
        uint8_t v1_alarm                                  = 0;
        notification_supported_report_bit_mask_t bit_mask = {};

        number_of_bit_masks = get_value_or_default(attribute_map, "number_of_bit_masks", number_of_bit_masks);
        v1_alarm            = get_value_or_default(attribute_map, "v1_alarm", v1_alarm);
        bit_mask            = get_value_or_default(attribute_map, "bit_mask", bit_mask);

        auto parent_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(notification_supported_report_group_attributes_t::NOTIFICATION_SUPPORTED_REPORT_GROUP));

        auto num_masks_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_supported_report_group_attributes_t::number_of_bit_masks));
        num_masks_node.set_reported<uint8_t>(number_of_bit_masks);

        auto v1_alarm_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_supported_report_group_attributes_t::v1_alarm));
        v1_alarm_node.set_reported<uint8_t>(v1_alarm);

        auto bit_mask_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(notification_supported_report_group_attributes_t::bit_mask));
        bit_mask_node.set_reported<notification_supported_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_notification_attribute_store::on_event_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_notification_attribute_map_t attribute_map)
    {
        event_supported_report_notification_type_t notif_type = 0;
        uint8_t number_of_bit_masks                           = 0;
        event_supported_report_bit_mask_t bit_mask            = {};

        notif_type          = get_value_or_default(attribute_map, "notification_type", notif_type);
        number_of_bit_masks = get_value_or_default(attribute_map, "number_of_bit_masks", number_of_bit_masks);
        bit_mask            = get_value_or_default(attribute_map, "bit_mask", bit_mask);

        auto parent_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(event_supported_report_group_attributes_t::EVENT_SUPPORTED_REPORT_GROUP));

        auto notif_type_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(event_supported_report_group_attributes_t::notification_type));
        notif_type_node.set_reported<event_supported_report_notification_type_t>(notif_type);

        auto num_masks_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(event_supported_report_group_attributes_t::number_of_bit_masks));
        num_masks_node.set_reported<uint8_t>(number_of_bit_masks);

        auto bit_mask_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(event_supported_report_group_attributes_t::bit_mask));
        bit_mask_node.set_reported<event_supported_report_bit_mask_t>(bit_mask);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
