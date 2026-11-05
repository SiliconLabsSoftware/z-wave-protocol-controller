
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

#ifndef COMMAND_CLASS_USER_CODE_H
#define COMMAND_CLASS_USER_CODE_H

#include "command_class_user_code_mqtt.hpp"
#include "command_class_user_code_attribute_store.hpp"
#include "command_class_user_credential_events.hpp"

namespace zwave_command_class
{

    class command_class_user_code final : public command_class_user_code_attribute_store, public command_class_user_code_mqtt
    {

        public:
            command_class_user_code();
            ~command_class_user_code() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            sl_status_t on_user_code_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload) override;
            sl_status_t on_users_number_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload) override;
            sl_status_t on_extended_user_code_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload) override;
            sl_status_t on_user_code_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload) override;
            sl_status_t on_user_code_keypad_mode_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload) override;
            sl_status_t on_admin_code_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload) override;

            sl_status_t on_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_extended_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_extended_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_code_keypad_mode_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_admin_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_code_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

        private:
            static attribute_store::attribute desired_child(attribute_store::attribute group, attribute_store_type_t type);
            void request_users_number(attribute_store::attribute endpoint);
            void request_admin_or_continue(attribute_store::attribute endpoint);
            void request_keypad_or_users(attribute_store::attribute endpoint);
            void request_extended_user(attribute_store::attribute endpoint, uint16_t identifier);
            void request_v1_user(attribute_store::attribute endpoint, uint8_t identifier);
            void begin_user_code_interview(attribute_store::attribute endpoint, uint8_t supported_version);
            sl_status_t on_supported_user_count_available(attribute_store::attribute endpoint, uint16_t supported_user_count);
            sl_status_t on_user_credential_presence_response(const user_credential_presence_response_payload_t &payload);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CODE_H
