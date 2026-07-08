
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

#ifndef COMMAND_CLASS_INDICATOR_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_INDICATOR_ATTRIBUTE_STORE_H

#include "command_class_indicator_core.hpp"
#include "command_class_indicator_types.hpp"  // command_class_indicator_types

namespace zwave_command_class
{

    class command_class_indicator_attribute_store : public virtual command_class_indicator_core
    {

        public:
            command_class_indicator_attribute_store();
            ~command_class_indicator_attribute_store() = default;

            sl_status_t on_indicator_report_received_store(attribute_store::attribute endpoint_node, command_class_indicator_attribute_map_t attribute_map) override;
            sl_status_t on_indicator_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_indicator_attribute_map_t attribute_map) override;
            sl_status_t on_indicator_description_report_received_store(attribute_store::attribute endpoint_node, command_class_indicator_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_INDICATOR_ATTRIBUTE_STORE_H
