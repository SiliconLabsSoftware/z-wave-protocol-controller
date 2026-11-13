
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

#ifndef COMMAND_CLASS_BARRIER_OPERATOR_H
#define COMMAND_CLASS_BARRIER_OPERATOR_H

#include <optional>

#include "command_class_barrier_operator_mqtt.hpp"
#include "command_class_barrier_operator_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_barrier_operator final : public command_class_barrier_operator_attribute_store, public command_class_barrier_operator_mqtt
    {

        public:
            command_class_barrier_operator();
            ~command_class_barrier_operator() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;
            sl_status_t on_barrier_operator_signal_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_barrier_operator_attribute_map_t payload) override;
            sl_status_t on_barrier_operator_event_signaling_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_barrier_operator_attribute_map_t payload) override;
            sl_status_t on_barrier_operator_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_barrier_operator_event_signal_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_barrier_operator_event_signaling_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

        private:
            static bool is_supported_subsystem_type(uint8_t subsystem_type);
            static std::optional<uint8_t> next_supported_subsystem(const std::vector<uint8_t> &bit_mask, uint8_t after_subsystem_type);
            static void start_default_subsystem_activation(attribute_store::attribute endpoint_node, uint8_t after_subsystem_type);
            static void on_default_subsystem_activation_completed(attribute_store_node_t set_group_node);
            void require_supported_subsystem_reports(attribute_store::attribute endpoint_node, const std::vector<uint8_t> &bit_mask);
            void request_next_supported_subsystem_report(attribute_store::attribute endpoint_node, uint8_t after_subsystem_type);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_BARRIER_OPERATOR_H
