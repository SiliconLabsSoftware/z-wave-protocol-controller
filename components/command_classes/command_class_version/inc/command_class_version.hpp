
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

#ifndef COMMAND_CLASS_VERSION_H
#define COMMAND_CLASS_VERSION_H

#include <any>
#include "command_class_version_mqtt.hpp"
#include "command_class_version_attribute_store.hpp"
#include "component_connector_types.hpp"
#include "command_class_version_types.hpp"
#include "command_class_version_events.hpp"

namespace zwave_command_class
{

    class command_class_version final : public command_class_version_attribute_store, public command_class_version_mqtt
    {

        public:
            command_class_version();
            ~command_class_version() = default;

        private:
            void on_nif_attribute_update(attribute_store_node_t updated_node, attribute_store_change_t change);
            void on_secure_nif_attribute_update(attribute_store_node_t updated_node, attribute_store_change_t change);
            sl_status_t on_version_command_class_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload) override;
            sl_status_t on_version_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload) override;
            sl_status_t on_version_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload) override;
            static sl_status_t on_version_cc_get_requested(command_class_version_types::command_class_version_cc_get_payload_t payload);
            static sl_status_t on_version_get_interview_requested(command_class_version_types::command_class_version_get_payload_t payload);
            static sl_status_t on_version_capabilities_get_interview_requested(command_class_version_types::command_class_version_get_payload_t payload);
            static sl_status_t on_version_zwave_software_get_interview_requested(command_class_version_types::command_class_version_get_payload_t payload);
            static sl_status_t on_get_version_report_requested(const command_class_version_types::command_class_get_version_report_payload_t &payload_struct, command_class_version_types::command_class_get_version_report_payload_t &result_struct);
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;
            sl_status_t on_version_zwave_software_report_received_store(attribute_store::attribute endpoint_node, command_class_version_attribute_map_t attribute_map) override;

        protected:
            sl_status_t on_version_command_class_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_version_command_class_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_version_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_version_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_version_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_version_capabilities_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_version_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_version_zwave_software_get_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_VERSION_H
