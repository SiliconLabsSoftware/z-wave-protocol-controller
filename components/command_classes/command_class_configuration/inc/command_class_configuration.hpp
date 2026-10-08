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

#ifndef COMMAND_CLASS_CONFIGURATION_H
#define COMMAND_CLASS_CONFIGURATION_H

#include <vector>

#include "command_class_configuration_mqtt.hpp"
#include "command_class_configuration_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_configuration final : public command_class_configuration_attribute_store, public command_class_configuration_mqtt
    {

        public:
            command_class_configuration();
            ~command_class_configuration() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            static void on_configuration_get_resolution_give_up(attribute_store_node_t group_node_id);

            static void start_properties_walk(attribute_store::attribute endpoint_node);
            static void start_parameter_scan(attribute_store::attribute endpoint_node);
            static void continue_parameter_scan_after_give_up(attribute_store::attribute endpoint_node);
            static void advance_parameter_scan(attribute_store::attribute endpoint_node, bool parameter_found);
            static void request_configuration_get(attribute_store::attribute endpoint_node, uint8_t parameter_number);
            static void request_configuration_set(attribute_store::attribute endpoint_node, uint8_t parameter_number, uint8_t size, int64_t value, bool use_default);
            static void request_properties_get(attribute_store::attribute endpoint_node, uint16_t parameter_number);
            static void clear_reported_values_and_refresh(attribute_store::attribute endpoint_node);
            static bool bulk_set_allowed(attribute_store::attribute endpoint_node, uint16_t parameter_offset, uint8_t number_of_parameters);

        private:
            sl_status_t control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length) override;
            sl_status_t handle_configuration_bulk_report(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length);

            sl_status_t on_configuration_properties_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload) override;
            sl_status_t on_configuration_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload) override;
            sl_status_t on_configuration_name_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload) override;
            sl_status_t on_configuration_info_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload) override;
            sl_status_t on_configuration_bulk_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload) override;

            sl_status_t on_configuration_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_configuration_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_configuration_name_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_configuration_info_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_configuration_properties_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_configuration_bulk_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_configuration_bulk_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;

            void register_default_reset_set_rule();

            static void finish_size_probe_and_maybe_start_default_probe(attribute_store::attribute endpoint_node);
            static void start_default_bit_probe(attribute_store::attribute endpoint_node);
            static void handle_default_probe_report(attribute_store::attribute endpoint, uint8_t parameter_number, int64_t value);
            static uint8_t configuration_version(attribute_store::attribute endpoint_node);
            uint16_t apply_parameter_0_next_quirk(uint16_t parameter_number, uint16_t next_parameter_number);

            std::vector<uint8_t> pending_properties_frame_;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_CONFIGURATION_H
