
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

#ifndef COMMAND_CLASS_MULTI_CHANNEL_H
#define COMMAND_CLASS_MULTI_CHANNEL_H

#include <any>
#include "component_connector_types.hpp"
#include "command_class_multi_channel_mqtt.hpp"
#include "command_class_multi_channel_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_multi_channel final : public command_class_multi_channel_attribute_store, public command_class_multi_channel_mqtt
    {

        public:
            command_class_multi_channel();
            ~command_class_multi_channel() = default;

        private:
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            sl_status_t on_multi_channel_end_point_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_attribute_map_t payload) override;
            sl_status_t on_multi_channel_capability_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_attribute_map_t payload) override;
            sl_status_t on_multi_channel_end_point_find_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_multi_channel_attribute_map_t payload) override;

            sl_status_t on_multi_channel_capability_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_multi_channel_end_point_find_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_multi_channel_aggregated_members_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

            static sl_status_t on_multi_channel_end_point_find_requested(command_class_multi_channel_types::command_class_multi_channel_end_point_find_payload_t payload);
            static sl_status_t on_multi_channel_commands_capability_get_requested(command_class_multi_channel_types::command_class_multi_channel_commands_capability_get_payload_t payload);
            static sl_status_t on_multi_channel_get_list_of_endpoints_requested(const command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t &payload, command_class_multi_channel_types::command_class_multi_channel_get_list_of_endpoints_payload_t &result);
            static sl_status_t on_multi_channel_end_point_get_interview_requested(command_class_multi_channel_types::command_class_multi_channel_end_point_get_payload_t payload);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_MULTI_CHANNEL_H
