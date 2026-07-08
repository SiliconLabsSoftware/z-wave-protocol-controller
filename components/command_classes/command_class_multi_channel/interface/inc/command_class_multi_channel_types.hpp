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

#ifndef COMMAND_CLASS_MULTI_CHANNEL_TYPES_H
#define COMMAND_CLASS_MULTI_CHANNEL_TYPES_H

#include "attribute.hpp"
#include "attribute_store.h"
#include "command_class_multi_channel_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_multi_channel_types
    {
        struct command_class_multi_channel_poll_endpoint_capabilities_payload_t {
                uint8_t endpoint_id;
                attribute_store_node_t node;
                bool is_first_endpoint;
        };

        struct command_class_multi_channel_end_point_find_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct command_class_multi_channel_end_point_find_report_payload_t {
                attribute_store::attribute device_endpoint_node;
                multi_channel_end_point_find_report_vg_t endpoints;
        };

        struct command_class_multi_channel_commands_capability_get_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t endpoint_id;
        };

        struct command_class_multi_channel_commands_capability_report_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t endpoint_id;
        };

        struct command_class_multi_channel_get_list_of_endpoints_payload_t {
                attribute_store::attribute device_endpoint_node;
                multi_channel_end_point_find_report_vg_t endpoints;
        };

        struct command_class_multi_channel_end_point_get_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct command_class_multi_channel_end_point_report_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t individual_end_points;
                uint8_t aggregated_end_points;
                uint8_t dynamic;
                uint8_t identical;
        };

    }  // namespace command_class_multi_channel_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_MULTI_CHANNEL_TYPES_H
