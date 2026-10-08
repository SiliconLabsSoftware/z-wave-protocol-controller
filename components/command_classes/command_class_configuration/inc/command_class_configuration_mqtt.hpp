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

#ifndef COMMAND_CLASS_CONFIGURATION_MQTT_H
#define COMMAND_CLASS_CONFIGURATION_MQTT_H

#include "command_class_configuration_core.hpp"

namespace zwave_command_class
{

    class command_class_configuration_mqtt : public virtual command_class_configuration_core
    {

        public:
            command_class_configuration_mqtt();
            ~command_class_configuration_mqtt() = default;

            static sl_status_t mqtt_on_configuration_bulk_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_bulk_set_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_set_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_name_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_info_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_properties_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_default_reset_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_properties_discover_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_configuration_parameter_scan_command(attribute_store::attribute &endpoint_node, std::string payload);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_CONFIGURATION_MQTT_H
