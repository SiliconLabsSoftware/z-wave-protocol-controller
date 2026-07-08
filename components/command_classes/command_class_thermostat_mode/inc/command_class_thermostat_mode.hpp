
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

#ifndef COMMAND_CLASS_THERMOSTAT_MODE_H
#define COMMAND_CLASS_THERMOSTAT_MODE_H

#include "command_class_thermostat_mode_mqtt.hpp"
#include "command_class_thermostat_mode_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_thermostat_mode final : public command_class_thermostat_mode_attribute_store, public command_class_thermostat_mode_mqtt
    {

        public:
            command_class_thermostat_mode();
            ~command_class_thermostat_mode() = default;

        protected:
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

        private:
            sl_status_t on_thermostat_mode_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_mode_attribute_map_t payload) override;
            sl_status_t on_thermostat_mode_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_THERMOSTAT_MODE_H
