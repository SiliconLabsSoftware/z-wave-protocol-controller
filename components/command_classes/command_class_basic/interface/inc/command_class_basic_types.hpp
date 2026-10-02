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

#ifndef COMMAND_CLASS_BASIC_TYPES_H
#define COMMAND_CLASS_BASIC_TYPES_H

#include "command_class_basic_generated_types.hpp"
#include "attribute.hpp"

namespace zwave_command_class
{
    namespace command_class_basic_types
    {
        struct basic_get_interview_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct basic_report_received_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct basic_get_resolution_give_up_payload_t {
                attribute_store::attribute device_endpoint_node;
        };
    }  // namespace command_class_basic_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_BASIC_TYPES_H
