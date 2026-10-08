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

#ifndef COMMAND_CLASS_CONFIGURATION_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_CONFIGURATION_ATTRIBUTE_STORE_H

#include "command_class_configuration_core.hpp"
#include "command_class_configuration_types.hpp"

namespace zwave_command_class
{

    class command_class_configuration_attribute_store : public virtual command_class_configuration_core
    {

        public:
            command_class_configuration_attribute_store();
            ~command_class_configuration_attribute_store() = default;

            sl_status_t on_configuration_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map) override;
            sl_status_t on_configuration_name_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map) override;
            sl_status_t on_configuration_info_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map) override;
            sl_status_t on_configuration_properties_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map) override;
            sl_status_t on_configuration_bulk_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map) override;

            static attribute_store::attribute emplace_parameter(attribute_store::attribute endpoint_node, uint16_t parameter_number);
            static int64_t decode_configuration_value(const std::vector<uint8_t> &bytes, uint8_t format);
            static std::vector<uint8_t> encode_configuration_value(int64_t value, uint8_t size);
            static uint8_t normalize_format(uint8_t raw_format_field);
            static uint8_t normalize_flag(uint8_t raw_flag_field);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_CONFIGURATION_ATTRIBUTE_STORE_H
