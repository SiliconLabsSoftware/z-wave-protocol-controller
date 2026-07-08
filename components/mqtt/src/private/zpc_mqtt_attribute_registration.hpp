
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

#ifndef ZPC_MQTT_ATTRIBUTE_REGISTRATION
#define ZPC_MQTT_ATTRIBUTE_REGISTRATION

// ZPC
#include "attribute.hpp"  // attribute_store_type_t, attribute_store::attribute

// Cpp
#include <string>
#include <map>

// Others
#include "zpc_mqtt_definitions.hpp"  // mqtt_value_encoder
namespace zpc_mqtt
{
    /**
     * @brief Class to register attributes for the ZPC MQTT component
     *
     * This class is used to register attributes for the ZPC MQTT component.
     *
     * @see zpc_mqtt_command_registration
     */
    class zpc_mqtt_attribute_registration
    {
        public:
            // MQTT attribute identifiers
            struct mqtt_identifiers {
                    std::string command_class_name;
                    std::string attribute_name;
            };

            zpc_mqtt_attribute_registration()  = default;
            ~zpc_mqtt_attribute_registration() = default;

            /**
             * @brief Register an attribute type for the ZPC MQTT component.
             *
             * @param attribute_type The attribute type to register.
             * @param command_class_name The name of the command class
             * @param attribute_name The name of the attribute
             */
            void register_attribute_type(attribute_store_type_t attribute_type, const std::string &command_class_name, const std::string &attribute_name);

            /**
             * @brief Allow to transform a value before publishing it
             *
             * Some attributes need transformation into string before being published.
             * Use this function to register a function that will be applied before publishing the value.
             *
             * @param attribute_type The attribute type to register.
             * @param encoder The encoder to apply
             *
             */
            void register_encoder(attribute_store_type_t attribute_type, const mqtt_value_encoder &encoder);

            /**
             * @brief Get the topic for an attribute
             *
             * It will crawl up the attribute tree to get the full path of the attribute. If
             * the attribute is nested it will return the full path of the attribute.
             *
             * Consider the following tree:
             * |_ U8_ATTRIBUTE (12)
             *  |_ NESTED_ATTRIBUTE (15)
             *
             * A path for NESTED_ATTRIBUTE will be : Attribute/U8_ATTRIBUTE/12/NESTED_ATTRIBUTE
             * A path for U8_ATTRIBUTE will be : Attribute/U8_ATTRIBUTE
             *
             * @param attribute The attribute to get the topic from
             */
            std::string get_attribute_topic(const attribute_store::attribute &attribute) const;

            /**
             * @brief Get complete path for an attribute
             *
             * Complete path include, base topic, attribute topic and state.
             *
             * zpc/{home_id}/{node_id}/ep{endpoint_id}/Attribute/{attribute_name}/[Desired|Reported]
             *
             * @param attribute The attribute to get the complete path from
             * @param state The state of the attribute (DESIRED_ATTRIBUTE or REPORTED_ATTRIBUTE)
             */
            std::string get_complete_attribute_topic(const attribute_store::attribute &attribute, attribute_store_node_value_state_t state) const;

            /**
             * @brief Get the payload for an attribute
             *
             * The payload will be in JSON format : {"value":<value>}
             *
             * @param attribute The attribute to get the payload from
             * @param state The state of the attribute (DESIRED_ATTRIBUTE or REPORTED_ATTRIBUTE)
             */
            std::string get_attribute_payload(const attribute_store::attribute &attribute, attribute_store_node_value_state_t state);

        private:
            /**
             * @brief Helper function that put an attribute value to a topic
             *
             * Consider the following tree:
             * |_ U8_ATTRIBUTE (12)
             *  |_ NESTED_ATTRIBUTE (15)
             *
             * attribute_value_to_topic(U8_ATTRIBUTE) will return : U8_ATTRIBUTE/12/
             * attribute_value_to_topic(NESTED_ATTRIBUTE) will return : NESTED_ATTRIBUTE/15/
             *
             * @param attribute The attribute to get the value from
             */
            std::string attribute_value_to_topic(const attribute_store::attribute &attribute) const;

            /**
             * @brief Check if an encoder is available for an attribute type
             *
             * @param attribute_type The attribute type to check
             *
             * @return true if an encoder is available, false otherwise
             */
            bool is_encoder_available(attribute_store_type_t attribute_type) const;

            // Internal registration map
            std::map<attribute_store_type_t, mqtt_identifiers> mqtt_registration_map;
            // Attribute encoders
            // Some attributes need transformation into string before being published
            std::map<attribute_store_type_t, mqtt_value_encoder> encoders;
    };
}  // namespace zpc_mqtt

#endif  // ZPC_MQTT_ATTRIBUTE_REGISTRATION