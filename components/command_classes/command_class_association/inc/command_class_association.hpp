
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

#ifndef COMMAND_CLASS_ASSOCIATION_H
#define COMMAND_CLASS_ASSOCIATION_H

#include "command_class_association_mqtt.hpp"
#include "command_class_association_attribute_store.hpp"
#include "command_class_association_types.hpp"

namespace zwave_command_class
{

    class command_class_association final : public command_class_association_attribute_store, public command_class_association_mqtt
    {

        public:
            command_class_association();
            ~command_class_association() = default;

        private:
            static sl_status_t on_association_groupings_get_requested(component_connector_association_groupings_get_payload_t payload);
            static sl_status_t on_association_supported_groupings_count_requested(component_connector_association_groupings_get_payload_t payload, uint8_t &result);
            static sl_status_t on_association_get_interview_requested(component_connector_association_get_payload_t payload);
            static sl_status_t on_association_set_requested(component_connector_association_set_payload_t payload);

            sl_status_t on_association_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_attribute_map_t payload) override;

            sl_status_t on_association_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map) override;
            sl_status_t on_association_remove_support_received(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map) override;

            sl_status_t on_association_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_association_groupings_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_association_specific_group_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;

            sl_status_t on_association_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_association_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_ASSOCIATION_H
