
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

#ifndef COMMAND_CLASS_INDICATOR_MQTT_H
#define COMMAND_CLASS_INDICATOR_MQTT_H

#include "command_class_indicator_core.hpp"

namespace zwave_command_class
{

    class command_class_indicator_mqtt : public virtual command_class_indicator_core
    {

        public:
            command_class_indicator_mqtt();
            ~command_class_indicator_mqtt() = default;

            static sl_status_t mqtt_on_indicator_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_indicator_set_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_indicator_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload);
            static sl_status_t mqtt_on_indicator_description_get_command(attribute_store::attribute &endpoint_node, std::string payload);

            /**
             * @brief Called after an incoming Indicator Set is stored.
             *
             * Default implementation publishes MQTT `Indicator/Report/IndicatorSet` for CTT
             * and internal use. Override to drive a physical indicator on a product build.
             */
            virtual void publish_indicator_set_received(attribute_store::attribute endpoint_node, uint8_t indicator_0_value, uint8_t indicator_object_count, const indicator_set_vg1_t &set_vg1);

        private:
            void clear_attribute(attribute_store::attribute &endpoint_node, attribute_store_type_t attribute_type);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_INDICATOR_MQTT_H
