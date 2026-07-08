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

#ifndef COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_TYPES_H
#define COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_TYPES_H

#include "attribute_store.h"
#include "attribute.hpp"
#include "zwave_generic_types.h"
#include "command_class_multi_channel_association_generated_types.hpp"

namespace zwave_command_class
{
    /**
     * @brief Payload for Multi Channel Association Groupings Get request and
     *        Groupings Report event.
     *
     * Request: endpoint_node set; supported_groupings ignored.
     * Report:  endpoint_node and supported_groupings set by the CC when
     *         firing COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT.
     */
    struct component_connector_multi_channel_association_groupings_get_payload_t {
            attribute_store::attribute endpoint_node;
            uint8_t supported_groupings = 0;
    };

    struct component_connector_multi_channel_association_get_payload_t {
            attribute_store::attribute endpoint_node;
            uint8_t grouping_identifier;
    };

    struct component_connector_multi_channel_association_report_payload_t {
            attribute_store::attribute endpoint_node;
            uint8_t grouping_identifier;
    };

    struct component_connector_multi_channel_association_set_payload_t {
            attribute_store::attribute endpoint_node;
            uint8_t grouping_identifier;
            zwave_node_id_t node_id;
            uint8_t endpoint_id;
    };

    namespace command_class_multi_channel_association_types
    {}  // namespace command_class_multi_channel_association_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_TYPES_H
