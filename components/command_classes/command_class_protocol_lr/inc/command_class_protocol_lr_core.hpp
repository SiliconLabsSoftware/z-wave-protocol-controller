
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

#ifndef COMMAND_CLASS_PROTOCOL_LR_CORE_H
#define COMMAND_CLASS_PROTOCOL_LR_CORE_H

// Base class
#include "zwave_command_class_base.h"

// ZPC
#include "attribute_store_defined_attribute_types.h"  // ZWAVE_CC_VERSION_ATTRIBUTE

#include "zwave_frame_parser.hpp"  // zwave_frame_parser
#include "attribute.hpp"           // attribute_store::attribute

namespace zwave_command_class
{

    class command_class_protocol_lr_core : public zwave_command_class_base
    {
        public:
            // Constructor
            command_class_protocol_lr_core();
            ~command_class_protocol_lr_core() = default;

            sl_status_t support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length) override;
            bool has_support_handler() const override
            {
                return true;
            }
    };
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_PROTOCOL_LR_CORE_H
