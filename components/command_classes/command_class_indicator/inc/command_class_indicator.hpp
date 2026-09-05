
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

#ifndef COMMAND_CLASS_INDICATOR_H
#define COMMAND_CLASS_INDICATOR_H

#include "attribute.hpp"
#include "command_class_indicator_mqtt.hpp"
#include "command_class_indicator_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_indicator final : public command_class_indicator_attribute_store, public command_class_indicator_mqtt
    {

        public:
            command_class_indicator();
            ~command_class_indicator() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

        private:
            sl_status_t on_indicator_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_indicator_attribute_map_t payload) override;
            sl_status_t on_indicator_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_indicator_attribute_map_t payload) override;

            sl_status_t on_indicator_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_indicator_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_indicator_supported_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_indicator_description_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

            sl_status_t on_indicator_supported_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_indicator_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_indicator_description_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_indicator_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map) override;

            void send_indicator_report_to_lifeline(const zwave_controller_connection_info_t *connection_info);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_INDICATOR_H
