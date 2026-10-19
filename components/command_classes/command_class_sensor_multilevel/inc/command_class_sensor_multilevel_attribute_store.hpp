
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

#ifndef COMMAND_CLASS_SENSOR_MULTILEVEL_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_SENSOR_MULTILEVEL_ATTRIBUTE_STORE_H

#include "command_class_sensor_multilevel_core.hpp"
#include "command_class_sensor_multilevel_types.hpp"  // command_class_sensor_multilevel_types

#include <vector>

namespace zwave_command_class
{

    class command_class_sensor_multilevel_attribute_store : public virtual command_class_sensor_multilevel_core
    {

        public:
            command_class_sensor_multilevel_attribute_store();
            ~command_class_sensor_multilevel_attribute_store() = default;

            static std::vector<uint8_t> get_supported_sensor_bit_mask(attribute_store::attribute endpoint_node);
            static bool get_supported_scale_bit_mask_for_sensor_type(attribute_store::attribute endpoint_node, uint8_t sensor_type, uint8_t &out_scale_bit_mask);
            static bool has_report_for_sensor_type(attribute_store::attribute endpoint_node, uint8_t sensor_type);

        protected:
            static attribute_store::attribute find_group_by_sensor_type(attribute_store::attribute endpoint_node, attribute_store_type_t group_type, attribute_store_type_t sensor_type_attr, uint8_t sensor_type);

        private:
            sl_status_t on_sensor_multilevel_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_multilevel_attribute_map_t attribute_map) override;
            sl_status_t on_sensor_multilevel_supported_sensor_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_multilevel_attribute_map_t attribute_map) override;
            sl_status_t on_sensor_multilevel_supported_scale_report_received_store(attribute_store::attribute endpoint_node, command_class_sensor_multilevel_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_SENSOR_MULTILEVEL_ATTRIBUTE_STORE_H
