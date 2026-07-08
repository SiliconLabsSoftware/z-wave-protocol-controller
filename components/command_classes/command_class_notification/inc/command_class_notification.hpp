
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

#ifndef COMMAND_CLASS_NOTIFICATION_H
#define COMMAND_CLASS_NOTIFICATION_H

#include <optional>

#include "command_class_notification_mqtt.hpp"
#include "command_class_notification_attribute_store.hpp"
#include "command_class_association_grp_info_types.hpp"
#include "attribute.hpp"

namespace zwave_command_class
{

    class command_class_notification final : public command_class_notification_attribute_store, public command_class_notification_mqtt
    {

        public:
            command_class_notification();
            ~command_class_notification() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            sl_status_t on_notification_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_notification_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_event_supported_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

            sl_status_t on_notification_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_notification_attribute_map_t payload) override;
            sl_status_t on_event_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_notification_attribute_map_t payload) override;
            sl_status_t on_notification_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_notification_attribute_map_t payload) override;

        private:
            static sl_status_t on_check_command_list_result(const command_class_association_grp_info_types::component_connector_agi_check_command_list_result_t &result);
            static sl_status_t on_agi_group_command_list_report(const command_class_association_grp_info_types::component_connector_agi_groupings_payload_t &payload);
            static void fire_check_command_in_group_list(attribute_store::attribute endpoint_node);
            static uint8_t get_notification_mode(attribute_store::attribute endpoint_node);
            static std::vector<uint8_t> extract_supported_types(const std::vector<uint8_t> &bit_mask);
            static std::vector<uint8_t> get_supported_types_from_store(attribute_store::attribute endpoint);
            static std::optional<uint8_t> find_next_type(const std::vector<uint8_t> &types, uint8_t current_type);
            static void start_event_supported_get(attribute_store::attribute endpoint, uint8_t notification_type);
            static void start_notification_get(attribute_store::attribute endpoint, uint8_t notification_type);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_NOTIFICATION_H
