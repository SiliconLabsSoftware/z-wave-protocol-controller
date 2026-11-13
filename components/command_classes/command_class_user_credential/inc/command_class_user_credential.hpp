/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMMAND_CLASS_USER_CREDENTIAL_H
#define COMMAND_CLASS_USER_CREDENTIAL_H

#include "command_class_user_credential_attribute_store.hpp"
#include "command_class_user_credential_mqtt.hpp"
#include "command_class_user_credential_events.hpp"

namespace zwave_command_class
{

    class command_class_user_credential final : public command_class_user_credential_attribute_store, public command_class_user_credential_mqtt
    {

        public:
            command_class_user_credential();
            ~command_class_user_credential() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            sl_status_t on_user_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload) override;
            sl_status_t on_credential_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload) override;
            sl_status_t on_all_users_checksum_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload) override;
            sl_status_t on_user_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload) override;
            sl_status_t on_credential_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_credential_attribute_map_t payload) override;

            sl_status_t on_user_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_credential_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_credential_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_credential_learn_start_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_credential_learn_cancel_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_credential_association_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_all_users_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_credential_checksum_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_admin_pin_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_admin_pin_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

        private:
            static sl_status_t on_user_code_presence_query(const user_credential_presence_query_payload_t &payload);
            static attribute_store::attribute desired_child(attribute_store::attribute group, attribute_store_type_t type);
            void request_user_capabilities(attribute_store::attribute endpoint);
            void request_credential_capabilities(attribute_store::attribute endpoint);
            void request_admin_pin_code(attribute_store::attribute endpoint);
            static void forget_stored_users_and_credentials(attribute_store::attribute endpoint);
            void request_checksum_or_users(attribute_store::attribute endpoint);
            void request_user(attribute_store::attribute endpoint, uint16_t user_unique_identifier);
            void request_credential(attribute_store::attribute endpoint, uint16_t user_unique_identifier, uint8_t credential_type, uint16_t credential_slot);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CREDENTIAL_H
