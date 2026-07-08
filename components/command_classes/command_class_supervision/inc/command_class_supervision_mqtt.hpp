
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

#ifndef COMMAND_CLASS_SUPERVISION_MQTT_H
#define COMMAND_CLASS_SUPERVISION_MQTT_H

#include "command_class_supervision_core.hpp"

namespace zwave_command_class
{

    class command_class_supervision_mqtt : public virtual command_class_supervision_core
    {

        public:
            command_class_supervision_mqtt();
            ~command_class_supervision_mqtt() = default;

        private:
            void clear_attribute(attribute_store::attribute &endpoint_node, attribute_store_type_t attribute_type);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SUPERVISION_MQTT_H
