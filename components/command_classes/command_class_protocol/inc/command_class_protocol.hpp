
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

#ifndef COMMAND_CLASS_PROTOCOL_H
#define COMMAND_CLASS_PROTOCOL_H

#include "command_class_protocol_attribute_store.hpp"
#include "attribute_store_defined_attribute_types.h"
#include <any>
#include "sl_status.h"
#include "command_class_protocol_types.hpp"
#include "zwave_controller_callbacks.h"
#include "zwapi_protocol_transport.h"

namespace zwave_command_class
{

    class command_class_protocol : public command_class_protocol_attribute_store
    {
        private:
            static sl_status_t on_commands_request_node_info(command_class_protocol_types::command_class_protocol_commands_request_node_info_payload_t payload);

            static protocol_metadata_t s_metadata;
            static const zwave_controller_callbacks_t s_protocol_callbacks;

            static void on_protocol_cc_encryption_request(const zwave_node_id_t destination_node_id, const uint8_t payload_length, const uint8_t *const payload, const uint8_t protocol_metadata_length, const uint8_t *const protocol_metadata, const uint8_t use_supervision, const uint8_t session_id);

            static void on_protocol_frame_received(const zwave_controller_connection_info_t *connection_info, const zwave_rx_receive_options_t *rx_options, const uint8_t *frame_data, uint16_t frame_length);

            static void on_send_protocol_data_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user);

        public:
            command_class_protocol();
            ~command_class_protocol() = default;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_PROTOCOL_H
