
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

#ifndef COMMAND_CLASS_PROTOCOL_LR_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_PROTOCOL_LR_ATTRIBUTE_STORE_H

#include "command_class_protocol_lr_core.hpp"

namespace zwave_command_class
{

    class command_class_protocol_lr_attribute_store : public virtual command_class_protocol_lr_core
    {

        public:
            command_class_protocol_lr_attribute_store();
            ~command_class_protocol_lr_attribute_store() = default;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_PROTOCOL_LR_ATTRIBUTE_STORE_H
