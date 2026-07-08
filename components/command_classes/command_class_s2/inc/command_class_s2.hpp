
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

#ifndef COMMAND_CLASS_S2_H
#define COMMAND_CLASS_S2_H

#include <any>

#include "command_class_s2_attribute_store.hpp"
#include "command_class_s2_types.hpp"
#include "attribute_store.h"
#include "zwapi_protocol_transport.h"

namespace zwave_command_class
{

    class command_class_s2 final : public command_class_s2_attribute_store
    {

        public:
            command_class_s2();
            ~command_class_s2() = default;

        private:
            sl_status_t on_s2_commands_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint_node, command_class_s2_attribute_map_t attribute_map) override;
            static sl_status_t commands_supported_get(uint8_t *frame, uint16_t *frame_len);
            static sl_status_t s2_supported_get(command_class_s2_types::s2_supported_get_payload_t payload_struct);
            static sl_status_t s2_get_supported_command_class_list(const command_class_s2_types::s2_get_supported_command_class_list_payload_t &payload_struct, std::vector<uint8_t> &result);

            static void on_s2_supported_get_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user);
            static void fire_s2_supported_get_tx_failed(zwave_node_id_t node_id, uint8_t endpoint_id, uint8_t status);

            static zwave_node_id_t s_last_node_id;

            static sl_status_t nls_state_set(zwave_node_id_t node_id);

            static void on_nls_state_set_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user);

            static void on_nls_state_get_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user);

            static void on_nls_state_desired_change(attribute_store_node_t node, attribute_store_change_t change);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_S2_H
