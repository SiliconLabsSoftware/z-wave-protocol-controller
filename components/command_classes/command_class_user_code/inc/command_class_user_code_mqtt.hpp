
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

#ifndef COMMAND_CLASS_USER_CODE_MQTT_H
#define COMMAND_CLASS_USER_CODE_MQTT_H

#include "command_class_user_code_core.hpp"

namespace zwave_command_class
{

    class command_class_user_code_mqtt : public virtual command_class_user_code_core
    {

        public:
            command_class_user_code_mqtt();
            ~command_class_user_code_mqtt() = default;

            static sl_status_t mqtt_on_user_code_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_user_code_set_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_users_number_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_extended_user_code_set_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_extended_user_code_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_user_code_capabilities_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_user_code_keypad_mode_set_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_user_code_keypad_mode_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_admin_code_set_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_admin_code_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_user_code_checksum_get_command(attribute_store::attribute &endpoint_node, std::string payload);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_USER_CODE_MQTT_H
