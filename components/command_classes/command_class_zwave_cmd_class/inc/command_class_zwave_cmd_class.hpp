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

#ifndef ZWAVE_CMD_CLASS_H
#define ZWAVE_CMD_CLASS_H

#include "command_class_zwave_cmd_class_attribute_store.hpp"
#include "command_class_zwave_cmd_class_types.hpp"
#include "zwave_controller_callbacks.h"
#include "zwapi_protocol_transport.h"

namespace zwave_command_class
{

    class command_class_zwave_cmd_class final : public command_class_zwave_cmd_class_attribute_store
    {

        public:
            command_class_zwave_cmd_class();
            ~command_class_zwave_cmd_class() = default;

        private:
            sl_status_t support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length) override;

            static sl_status_t on_commands_request_node_info(command_class_zwave_cmd_class_types::command_class_protocol_commands_request_node_info_payload_t payload);

            static protocol_metadata_t s_metadata;
            static const zwave_controller_callbacks_t s_protocol_callbacks;

            static void on_protocol_cc_encryption_request(const zwave_node_id_t destination_node_id, const uint8_t payload_length, const uint8_t *const payload, const uint8_t protocol_metadata_length, const uint8_t *const protocol_metadata, const uint8_t use_supervision, const uint8_t session_id);

            static void on_protocol_frame_received(const zwave_controller_connection_info_t *connection_info, const zwave_rx_receive_options_t *rx_options, const uint8_t *frame_data, uint16_t frame_length);

            static void on_send_protocol_data_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user);
    };

}  // namespace zwave_command_class

#endif  // ZWAVE_CMD_CLASS_H
