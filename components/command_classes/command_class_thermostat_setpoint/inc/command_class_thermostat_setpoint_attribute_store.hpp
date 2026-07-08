
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

#ifndef COMMAND_CLASS_THERMOSTAT_SETPOINT_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_THERMOSTAT_SETPOINT_ATTRIBUTE_STORE_H

#include "command_class_thermostat_setpoint_core.hpp"
#include "command_class_thermostat_setpoint_types.hpp"  // command_class_thermostat_setpoint_types

#include <vector>

namespace zwave_command_class
{

    class command_class_thermostat_setpoint_attribute_store : public virtual command_class_thermostat_setpoint_core
    {

        public:
            command_class_thermostat_setpoint_attribute_store();
            ~command_class_thermostat_setpoint_attribute_store() = default;

            static bool get_reported_scale_for_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type, uint8_t &out_scale);

            static bool get_reported_capabilities_for_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type, std::vector<uint8_t> &out_min_value, std::vector<uint8_t> &out_max_value);

        private:
            static attribute_store::attribute find_report_group_by_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type);
            static attribute_store::attribute find_capabilities_report_group_by_setpoint_type(attribute_store::attribute endpoint_node, uint8_t setpoint_type);

            sl_status_t on_thermostat_setpoint_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_setpoint_attribute_map_t attribute_map) override;
            sl_status_t on_thermostat_setpoint_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_setpoint_attribute_map_t attribute_map) override;
            sl_status_t on_thermostat_setpoint_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_thermostat_setpoint_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_THERMOSTAT_SETPOINT_ATTRIBUTE_STORE_H
