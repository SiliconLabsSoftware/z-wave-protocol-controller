
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

#ifndef COMMAND_CLASS_THERMOSTAT_SETPOINT_H
#define COMMAND_CLASS_THERMOSTAT_SETPOINT_H

#include "command_class_thermostat_setpoint_mqtt.hpp"
#include "command_class_thermostat_setpoint_attribute_store.hpp"
#include "command_class_thermostat_setpoint_events.hpp"
#include "command_class_thermostat_mode_types.hpp"
#include "threading.hpp"

#include <cstdint>
#include <vector>

namespace zwave_command_class
{

    class command_class_thermostat_setpoint final : public command_class_thermostat_setpoint_attribute_store, public command_class_thermostat_setpoint_mqtt
    {

        public:
            command_class_thermostat_setpoint();
            ~command_class_thermostat_setpoint() = default;

        protected:
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

        private:
            sl_status_t on_thermostat_setpoint_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_setpoint_attribute_map_t payload) override;
            sl_status_t on_thermostat_setpoint_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_setpoint_attribute_map_t payload) override;
            sl_status_t on_thermostat_setpoint_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_setpoint_attribute_map_t payload) override;

            sl_status_t on_thermostat_setpoint_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_thermostat_setpoint_capabilities_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_thermostat_setpoint_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;

            static sl_status_t on_thermostat_mode_changed(const command_class_thermostat_mode_types::thermostat_mode_changed_payload_t &payload);

            static int32_t decode_signed_setpoint_value(const std::vector<uint8_t> &bytes, uint8_t size);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_THERMOSTAT_SETPOINT_H
