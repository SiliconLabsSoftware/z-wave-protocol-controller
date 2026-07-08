
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

#ifndef COMMAND_CLASS_NOTIFICATION_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_NOTIFICATION_ATTRIBUTE_STORE_H

#include "command_class_notification_core.hpp"
#include "command_class_notification_types.hpp"  // command_class_notification_types

namespace zwave_command_class
{

    class command_class_notification_attribute_store : public virtual command_class_notification_core
    {

        public:
            command_class_notification_attribute_store();
            ~command_class_notification_attribute_store() = default;

            sl_status_t on_notification_report_received_store(attribute_store::attribute endpoint_node, command_class_notification_attribute_map_t attribute_map) override;
            sl_status_t on_notification_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_notification_attribute_map_t attribute_map) override;
            sl_status_t on_event_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_notification_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_NOTIFICATION_ATTRIBUTE_STORE_H
