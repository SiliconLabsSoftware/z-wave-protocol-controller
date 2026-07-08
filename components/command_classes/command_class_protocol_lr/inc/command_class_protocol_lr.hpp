
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

#ifndef COMMAND_CLASS_PROTOCOL_LR_H
#define COMMAND_CLASS_PROTOCOL_LR_H

#include "command_class_protocol_lr_attribute_store.hpp"
#include "attribute_store_defined_attribute_types.h"

namespace zwave_command_class
{

    class command_class_protocol_lr : public command_class_protocol_lr_attribute_store
    {
        public:
            command_class_protocol_lr();
            ~command_class_protocol_lr() = default;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_PROTOCOL_LR_H
