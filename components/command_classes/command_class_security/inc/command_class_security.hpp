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

#ifndef COMMAND_CLASS_SECURITY_H
#define COMMAND_CLASS_SECURITY_H

#include "command_class_security_attribute_store.hpp"
#include "command_class_security_types.hpp"

namespace zwave_command_class
{

    class command_class_security final : public command_class_security_attribute_store
    {

        public:
            command_class_security();
            ~command_class_security() = default;

        private:
            sl_status_t control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length) override;
            sl_status_t on_security_commands_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint_node, command_class_security_attribute_map_t attribute_map) override;
            static sl_status_t s0_supported_get(command_class_security_types::s0_supported_get_payload_t payload_struct);
            static sl_status_t commands_supported_get(uint8_t *frame, uint16_t *frame_length);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SECURITY_H
