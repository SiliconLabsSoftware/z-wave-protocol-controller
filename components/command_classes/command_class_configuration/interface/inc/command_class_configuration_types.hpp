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

#ifndef COMMAND_CLASS_CONFIGURATION_TYPES_H
#define COMMAND_CLASS_CONFIGURATION_TYPES_H

#include "command_class_configuration_generated_types.hpp"
#include "attribute_store.h"

namespace zwave_command_class
{
    namespace command_class_configuration_types
    {
        // User attributes start after generated group enums (offset 57).
        enum class configuration_parameter_attributes_t : attribute_store_type_t {
            PARAMETER_ID = (112 << 8) | 58,
            size,
            format,
            min_value,
            max_value,
            default_value,
            value,
            name,
            info,
            read_only,
            altering_capabilities,
            advanced,
            no_bulk_support,
        };

        enum class configuration_endpoint_attributes_t : attribute_store_type_t {
            discovery_complete = (112 << 8) | 72,
            default_resets_every_parameter,
            properties_walk_active,
            parameter_scan_active,
            parameter_scan_number,
            parameter_scan_size_index,
            parameter_scan_phase,
            parameter_scan_reports_remaining,
            parameter_scan_probe_first,
            parameter_scan_probe_second,
            name_reports_to_follow,
            info_reports_to_follow,
        };

        enum class parameter_scan_phase_t : uint8_t {
            SIZE_PROBE                  = 0,
            DEFAULT_PROBE_READ_DEFAULTS = 1,
            DEFAULT_PROBE_PARTIAL_RESET = 2,
        };
    }  // namespace command_class_configuration_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_CONFIGURATION_TYPES_H
