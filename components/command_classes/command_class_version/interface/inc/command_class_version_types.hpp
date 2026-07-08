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

#ifndef COMMAND_CLASS_VERSION_TYPES_H
#define COMMAND_CLASS_VERSION_TYPES_H

#include "command_class_version_generated_types.hpp"
#include "attribute_store.h"

#include <optional>

namespace zwave_command_class
{
    namespace command_class_version_types
    {
        struct command_class_version_cc_get_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t command_class;
                bool is_first_command_class;
                uint8_t retry_count = 5;
        };

        struct command_class_version_get_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct command_class_version_capabilities_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct command_class_version_report_callback_payload_t {
                attribute_store::attribute device_endpoint_node;
        };

        struct command_class_version_capabilities_report_callback_payload_t {
                attribute_store::attribute device_endpoint_node;
                uint8_t z_wave_software = 0;
        };

        struct command_class_get_version_report_payload_t {
                attribute_store::attribute device_node;
                std::optional<uint8_t> z_wave_library_type;
                std::optional<uint8_t> z_wave_protocol_version;
                std::optional<uint8_t> z_wave_protocol_sub_version;
                std::optional<uint8_t> firmware_0_version;
                std::optional<uint8_t> firmware_0_sub_version;
                std::optional<uint8_t> hardware_version;
                std::optional<uint8_t> number_of_firmware_targets;
        };
    }  // namespace command_class_version_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_VERSION_TYPES_H
