
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

#ifndef COMMAND_CLASS_FIRMWARE_UPDATE_MD_H
#define COMMAND_CLASS_FIRMWARE_UPDATE_MD_H

#include "command_class_firmware_update_md_mqtt.hpp"
#include "command_class_firmware_update_md_attribute_store.hpp"
#include "command_class_firmware_update_md_types.hpp"

namespace zwave_command_class
{

    class command_class_firmware_update_md final : public command_class_firmware_update_md_attribute_store, public command_class_firmware_update_md_mqtt
    {

        public:
            command_class_firmware_update_md();
            ~command_class_firmware_update_md() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

        private:
            sl_status_t on_firmware_md_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_firmware_update_md_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;

            static sl_status_t on_firmware_md_get_requested(attribute_store::attribute endpoint_node);
            static sl_status_t on_firmware_update_md_request_get_requested(const command_class_firmware_update_md_types::command_class_firmware_update_md_request_get_payload_t &payload);
            static sl_status_t on_firmware_update_md_report_requested(const command_class_firmware_update_md_types::command_class_firmware_update_md_report_payload_t &payload);
            static sl_status_t on_firmware_update_activation_set_requested(const command_class_firmware_update_md_types::command_class_firmware_update_md_activation_set_payload_t &payload);
            // Assemble frame overrides for outgoing commands
            sl_status_t on_firmware_update_md_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_firmware_update_md_request_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_firmware_update_activation_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_firmware_update_md_prepare_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

            // Parsed report overrides — fire component_connector events
            sl_status_t on_firmware_md_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload) override;
            sl_status_t on_firmware_update_md_request_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload) override;
            sl_status_t on_firmware_update_md_get_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload) override;
            sl_status_t on_firmware_update_md_status_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload) override;
            sl_status_t on_firmware_update_activation_status_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload) override;
            sl_status_t on_firmware_update_md_prepare_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_FIRMWARE_UPDATE_MD_H
