
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

#ifndef COMMAND_CLASS_SWITCH_COLOR_H
#define COMMAND_CLASS_SWITCH_COLOR_H

#include "command_class_switch_color_mqtt.hpp"
#include "command_class_switch_color_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_switch_color final : public command_class_switch_color_attribute_store, public command_class_switch_color_mqtt
    {

        public:
            command_class_switch_color();
            ~command_class_switch_color() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            sl_status_t on_switch_color_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_switch_color_attribute_map_t payload) override;
            sl_status_t on_switch_color_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_switch_color_attribute_map_t payload) override;

            sl_status_t on_switch_color_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_switch_color_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_switch_color_start_level_change_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_switch_color_stop_level_change_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SWITCH_COLOR_H
