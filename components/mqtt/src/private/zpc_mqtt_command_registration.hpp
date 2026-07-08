
/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#ifndef ZPC_MQTT_COMMAND_REGISTRATION
#define ZPC_MQTT_COMMAND_REGISTRATION

// Other components
#include "zpc_mqtt_definitions.hpp"  // mqtt_command_callback

// ZPC
#include "attribute.hpp"  // attribute_store::attribute

// Cpp
#include <string>
#include <map>

namespace zpc_mqtt
{

    /**
     * @brief Class to register commands for the ZPC MQTT component
     *
     * This class is used to register commands for the ZPC MQTT component.
     *
     * @see zpc_mqtt_attribute_registration
     */
    class zpc_mqtt_command_registration
    {
        public:
            zpc_mqtt_command_registration()  = default;
            ~zpc_mqtt_command_registration() = default;

            /**
             * @brief Register a command for the ZPC MQTT component.
             *
             * @param command_class_name The name of the command class
             * @param callback The callback to call when a command for this command class is received
             *
             */
            void register_command(const std::string &command_class_name, const mqtt_command_callback &callback);

            /**
             * @brief Get the command topic for a command class
             *
             * @param command_class_name The name of the command class
             *
             */
            static std::string get_command_topic(const std::string &command_class_name);

            /**
             * @brief Execute the command callback for a topic and a payload
             *
             * This function will extract informations from the topic, such as the endpoint node
             * and the command class name, and try to call the appropriate callback.
             *
             * @param topic The topic of the command
             * @param message The payload of the command
             */
            void execute_command_callback(const std::string &topic, const std::string &payload) const;

        private:
            std::map<std::string, std::vector<mqtt_command_callback>> command_registration_map;

    };  // class zpc_mqtt_command_registration

}  // namespace zpc_mqtt

#endif  // ZPC_MQTT_COMMAND_REGISTRATION