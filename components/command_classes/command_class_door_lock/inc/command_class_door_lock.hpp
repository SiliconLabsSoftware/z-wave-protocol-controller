
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

#ifndef COMMAND_CLASS_DOOR_LOCK_H
#define COMMAND_CLASS_DOOR_LOCK_H

#include "command_class_door_lock_mqtt.hpp"
#include "command_class_door_lock_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_door_lock final : public command_class_door_lock_attribute_store, public command_class_door_lock_mqtt
    {

        public:
            command_class_door_lock();
            ~command_class_door_lock() = default;

        private:
            sl_status_t on_door_lock_operation_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_door_lock_configuration_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;

        protected:
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_DOOR_LOCK_H
