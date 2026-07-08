
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

#ifndef COMMAND_CLASS_SUPERVISION_H
#define COMMAND_CLASS_SUPERVISION_H

#include "command_class_supervision_mqtt.hpp"
#include "command_class_supervision_attribute_store.hpp"
#include "command_class_supervision_types.hpp"

namespace zwave_command_class
{

    class command_class_supervision final : public command_class_supervision_attribute_store, public command_class_supervision_mqtt
    {

        public:
            command_class_supervision();
            ~command_class_supervision() = default;

        private:
            sl_status_t on_supervision_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_supervision_attribute_map_t payload) override;
            sl_status_t on_supervision_get_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_supervision_attribute_map_t payload) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SUPERVISION_H
