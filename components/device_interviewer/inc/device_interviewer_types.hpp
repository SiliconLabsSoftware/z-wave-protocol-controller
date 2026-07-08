
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

#ifndef DEVICE_INTERVIEWER_TYPES_H
#define DEVICE_INTERVIEWER_TYPES_H

#include "attribute.hpp"
#include "zwave_keyset_definitions.h"
#include "zwave_generic_types.h"
#include "zwave_controller_connection_info.h"

#include <optional>
#include <vector>

namespace zwave_command_class
{
    struct device_interviewer_get_node_information_payload_t {
            attribute_store::attribute device_node;
            std::optional<uint8_t> listening_protocol;
            std::optional<uint8_t> optional_protocol;
            std::optional<uint8_t> basic_device_class;
            std::optional<uint8_t> generic_device_class;
            std::optional<uint8_t> specific_device_class;
            std::optional<std::vector<uint8_t>> command_class_list;
            std::optional<std::vector<uint8_t>> s2_command_class_list;
            std::optional<std::vector<uint8_t>> s0_command_class_list;
            std::optional<uint32_t> inclusion_protocol;
            std::optional<uint8_t> granted_keys;
    };
}  // namespace zwave_command_class
#endif  // DEVICE_INTERVIEWER_TYPES_H
