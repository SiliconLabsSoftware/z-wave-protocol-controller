
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

#ifndef COMMAND_CLASS_BARRIER_OPERATOR_ATTRIBUTE_STORE_H
#define COMMAND_CLASS_BARRIER_OPERATOR_ATTRIBUTE_STORE_H

#include "command_class_barrier_operator_core.hpp"
#include "command_class_barrier_operator_types.hpp"  // command_class_barrier_operator_types

namespace zwave_command_class
{

    class command_class_barrier_operator_attribute_store : public virtual command_class_barrier_operator_core
    {

        public:
            command_class_barrier_operator_attribute_store();
            ~command_class_barrier_operator_attribute_store() = default;

            static attribute_store::attribute find_report_group_by_subsystem_type(attribute_store::attribute endpoint_node, uint8_t subsystem_type);
            static attribute_store::attribute find_or_create_report_group_by_subsystem_type(attribute_store::attribute endpoint_node, uint8_t subsystem_type);
            static bool has_report_for_subsystem_type(attribute_store::attribute endpoint_node, uint8_t subsystem_type);
            static std::vector<uint8_t> get_supported_subsystems(attribute_store::attribute endpoint_node);

            sl_status_t on_barrier_operator_report_received_store(attribute_store::attribute endpoint_node, command_class_barrier_operator_attribute_map_t attribute_map) override;
            sl_status_t on_barrier_operator_signal_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_barrier_operator_attribute_map_t attribute_map) override;
            sl_status_t on_barrier_operator_event_signaling_report_received_store(attribute_store::attribute endpoint_node, command_class_barrier_operator_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_BARRIER_OPERATOR_ATTRIBUTE_STORE_H
