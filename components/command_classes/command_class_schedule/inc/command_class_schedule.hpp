
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

#ifndef COMMAND_CLASS_SCHEDULE_H
#define COMMAND_CLASS_SCHEDULE_H

#include "command_class_schedule_mqtt.hpp"
#include "command_class_schedule_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_schedule final : public command_class_schedule_attribute_store, public command_class_schedule_mqtt
    {

        public:
            command_class_schedule();
            ~command_class_schedule() = default;

        protected:
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            sl_status_t on_schedule_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_schedule_attribute_map_t payload) override;

            sl_status_t on_schedule_supported_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_command_schedule_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_command_schedule_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_schedule_remove_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_schedule_state_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_schedule_state_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_schedule_supported_commands_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

        private:
            static void start_schedule_supported_get(attribute_store::attribute endpoint_node, uint8_t schedule_id_block);
            static void start_schedule_supported_commands_get(attribute_store::attribute endpoint_node, uint8_t schedule_id_block);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SCHEDULE_H
