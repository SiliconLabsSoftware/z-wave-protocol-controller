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

#ifndef COMMAND_CLASS_METER_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_METER_ATTRIBUTE_STORE_H

#include "command_class_meter_core.hpp"
#include "command_class_meter_types.hpp"

#include <vector>

namespace zwave_command_class
{

    class command_class_meter_attribute_store : public virtual command_class_meter_core
    {

        public:
            command_class_meter_attribute_store();
            ~command_class_meter_attribute_store() = default;

            sl_status_t on_meter_report_received_store(attribute_store::attribute endpoint_node, command_class_meter_attribute_map_t attribute_map) override;
            sl_status_t on_meter_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_meter_attribute_map_t attribute_map) override;

            static bool get_reported_meter_reset_supported(attribute_store::attribute endpoint_node);

        protected:
            static attribute_store::attribute find_meter_report_group(attribute_store::attribute endpoint_node, uint8_t scale, uint8_t rate_type);
            static attribute_store::attribute find_or_create_meter_report_group(attribute_store::attribute endpoint_node, uint8_t scale, uint8_t rate_type);
            static bool get_supported_scale_bitmask(attribute_store::attribute endpoint_node, uint8_t &scale_supported_0, uint8_t &m_s_t, std::vector<uint8_t> &scale_supported);
            static bool get_supported_rate_type_masked(attribute_store::attribute endpoint_node, uint8_t &rate_type_masked);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_METER_ATTRIBUTE_STORE_H
