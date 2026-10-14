
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

#include <cstddef>
#include <cstdint>
#include <vector>

#include "command_class_user_code_mqtt.hpp"
#include "command_class_user_code_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_user_code final : public command_class_user_code_attribute_store, public command_class_user_code_mqtt
    {

        public:
            command_class_user_code();
            ~command_class_user_code() = default;

        private:
            sl_status_t on_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_extended_user_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_extended_user_code_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_user_code_keypad_mode_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_admin_code_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;

            sl_status_t on_user_code_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_user_code_attribute_map_t payload) override;

            static size_t count_set_bits(const std::vector<uint8_t> &bit_mask);

        protected:
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CODE_H
