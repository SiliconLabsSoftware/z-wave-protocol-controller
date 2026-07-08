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

#ifndef COMMAND_CLASS_WAKE_UP_TYPES_H
#define COMMAND_CLASS_WAKE_UP_TYPES_H

#include "attribute.hpp"
#include "command_class_wake_up_generated_types.hpp"
#include "zwave_node_id_definitions.h"

namespace zwave_command_class
{
    namespace command_class_wake_up_types
    {
        struct wake_up_capabilities_get_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct wake_up_capabilities_report_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct wake_up_interval_get_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct wake_up_interval_report_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint32_t seconds;
        };

        struct wake_up_interval_set_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint32_t interval;
                zwave_node_id_t node_id;
        };

        /// Payload for interview Interval Set attribute resolution completion (endpoint only).
        struct wake_up_interval_set_interview_resolution_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct wake_up_notification_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct wake_up_no_more_information_sent_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        /// Payload to arm No More Information when the node's resolution becomes idle.
        struct wake_up_arm_no_more_information_payload_t {
                attribute_store::attribute device_node_id_node;
        };

        struct wake_up_interval_requested_payload_t {
                attribute_store::attribute device_endpoint_node;
        };
    }  // namespace command_class_wake_up_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_WAKE_UP_TYPES_H
