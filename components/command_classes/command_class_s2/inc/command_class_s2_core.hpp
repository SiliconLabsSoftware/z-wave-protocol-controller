
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

#ifndef COMMAND_CLASS_S2_CORE_H
#define COMMAND_CLASS_S2_CORE_H

// Base class
#include "zwave_command_class_base.h"
#include "command_class_s2_types.hpp"

// ZPC
#include "attribute_store_defined_attribute_types.h"  // ZWAVE_CC_VERSION_ATTRIBUTE

#include "zwave_frame_parser.hpp"  // zwave_frame_parser
#include "attribute.hpp"           // attribute_store::attribute

using namespace zwave_command_class::command_class_s2_types;

namespace zwave_command_class
{

    class command_class_s2_core : public zwave_command_class_base
    {
        public:
            template<typename T> T get_value_or_default(const command_class_s2_attribute_map_t &map, const std::string &key, const T &default_value)
            {
                auto it = map.find(key);
                if (it != map.end() && std::holds_alternative<T>(it->second)) {
                    return std::get<T>(it->second);
                }
                return default_value;
            }

            sl_status_t control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length) override;
            sl_status_t support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length) override;
            bool has_control_handler() const override
            {
                return true;
            }

            bool has_support_handler() const override
            {
                return true;
            }

            virtual sl_status_t on_s2_commands_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint_node, command_class_s2_attribute_map_t attribute_map);
            virtual sl_status_t on_s2_commands_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_s2_attribute_map_t attribute_map);

            sl_status_t on_s2_commands_supported_report_received(const report_received_args &args);

            // Constructor
            command_class_s2_core();
            ~command_class_s2_core() = default;
    };
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_S2_CORE_H
