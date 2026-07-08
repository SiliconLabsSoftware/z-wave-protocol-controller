
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
#include <cstring>

// Base class
#include "command_class_protocol.hpp"
#include "component_connector.hpp"
#include "command_class_protocol_events.hpp"

#include "sl_status.h"

#include "zwapi_protocol_controller.h"
#include "zwave_controller.h"
#include "zwave_controller_utils.h"
#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"
#include "zwave_command_class_supervision.h"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_protocol";

    protocol_metadata_t command_class_protocol::s_metadata = {};

    const zwave_controller_callbacks_t command_class_protocol::s_protocol_callbacks = {
      .on_protocol_frame_received        = &command_class_protocol::on_protocol_frame_received,
      .on_protocol_cc_encryption_request = &command_class_protocol::on_protocol_cc_encryption_request,
    };

    command_class_protocol::command_class_protocol()
    {

        component_connector connector;
        connector.connect_typed<command_class_protocol_events_t, command_class_protocol_types::command_class_protocol_commands_request_node_info_payload_t>(
          command_class_protocol_events_t::COMMAND_CLASS_PROTOCOL_COMMANDS_REQUEST_NODE_INFO,
          [](const command_class_protocol_types::command_class_protocol_commands_request_node_info_payload_t &p) { return zwave_command_class::command_class_protocol::on_commands_request_node_info(p); });

        zwave_controller_register_callbacks(&s_protocol_callbacks);
    }

    sl_status_t command_class_protocol::on_commands_request_node_info(command_class_protocol_types::command_class_protocol_commands_request_node_info_payload_t payload)
    {
        zwapi_request_node_info(payload.node_id);
        return SL_STATUS_OK;
    }

    void command_class_protocol::on_send_protocol_data_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
    {
        protocol_metadata_t *meta = static_cast<protocol_metadata_t *>(user);

        if (status == TRANSMIT_COMPLETE_FAIL || status == TRANSMIT_COMPLETE_VERIFIED) {
            zwave_controller_request_protocol_cc_encryption_callback(status, tx_info, meta->session_id);
        } else {
            sl_log_debug(LOG_TAG.data(), "Send Protocol Data callback, status: %d", status);
        }
    }

    void command_class_protocol::on_protocol_cc_encryption_request(const zwave_node_id_t destination_node_id,
                                                                   const uint8_t payload_length,
                                                                   const uint8_t *const payload,
                                                                   const uint8_t protocol_metadata_length,
                                                                   const uint8_t *const protocol_metadata_data,
                                                                   const uint8_t use_supervision,
                                                                   const uint8_t session_id)
    {
        zwave_controller_connection_info_t connection_info = {};
        zwave_tx_options_t tx_options                      = {};
        constexpr uint8_t number_of_expected_responses     = 1;
        constexpr uint32_t discard_timeout_ms              = 5000;

        zwave_tx_scheme_get_node_connection_info(destination_node_id, 0, &connection_info);
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_MAX_PRIORITY, number_of_expected_responses, discard_timeout_ms, &tx_options);

        tx_options.transport.is_protocol_frame = true;

        s_metadata.session_id  = session_id;
        s_metadata.data_length = protocol_metadata_length;
        std::memcpy(s_metadata.data, protocol_metadata_data, protocol_metadata_length);

        if (use_supervision != 0U) {
            zwave_command_class_supervision_send_data(&connection_info, payload_length, payload, &tx_options, &on_send_protocol_data_complete, static_cast<void *>(&s_metadata), nullptr);
        } else {
            zwave_tx_send_data(&connection_info, payload_length, payload, &tx_options, &on_send_protocol_data_complete, static_cast<void *>(&s_metadata), nullptr);
        }
    }

    void command_class_protocol::on_protocol_frame_received(const zwave_controller_connection_info_t *connection_info, const zwave_rx_receive_options_t *rx_options, const uint8_t *frame_data, uint16_t frame_length)
    {
        (void)rx_options;

        sl_log_debug(LOG_TAG.data(), "Protocol frame received from NodeID %d", connection_info->remote.node_id);

        uint8_t decryption_key = zwave_controller_get_key_from_encapsulation(connection_info->encapsulation);

        sl_status_t status = zwapi_transfer_protocol_cc(connection_info->remote.node_id, decryption_key, frame_length, frame_data);

        if (status != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG.data(), "Failed to transfer protocol CC from NodeID %d, status: 0x%04X", connection_info->remote.node_id, status);
        }
    }

}  // namespace zwave_command_class