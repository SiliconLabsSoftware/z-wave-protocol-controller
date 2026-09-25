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

#ifndef COMMAND_CLASS_BASIC_H
#define COMMAND_CLASS_BASIC_H

#include "command_class_basic_mqtt.hpp"
#include "command_class_basic_attribute_store.hpp"
#include "command_class_basic_types.hpp"

namespace zwave_command_class
{

    class command_class_basic final : public command_class_basic_attribute_store, public command_class_basic_mqtt
    {

        public:
            command_class_basic();
            ~command_class_basic() = default;

        private:
            sl_status_t on_basic_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            static sl_status_t on_command_class_basic_get_interview_requested(command_class_basic_types::basic_get_interview_payload_t payload);
            static void on_basic_version_reported(attribute_store_node_t version_node, attribute_store_change_t change);
            static void on_basic_get_resolution_give_up(attribute_store_node_t group_node);

            // Basic is never advertised (CC:0020.01.00.21.003/004). Interview support
            // is inferred from a Basic Report (CL:0020.01.21.02.2); Version often returns 0.
            static bool has_basic_report(const attribute_store::attribute &endpoint_node);

        protected:
            sl_status_t on_basic_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_basic_attribute_map_t payload) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_BASIC_H
