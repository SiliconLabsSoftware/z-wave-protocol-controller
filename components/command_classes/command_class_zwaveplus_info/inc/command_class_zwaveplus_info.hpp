
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

#ifndef COMMAND_CLASS_ZWAVEPLUS_INFO_H
#define COMMAND_CLASS_ZWAVEPLUS_INFO_H

#include <any>
#include "component_connector_types.hpp"
#include "command_class_zwaveplus_info_mqtt.hpp"
#include "command_class_zwaveplus_info_attribute_store.hpp"
#include "command_class_zwaveplus_info_types.hpp"
#include "attribute.hpp"

namespace zwave_command_class
{

    class command_class_zwaveplus_info final : public command_class_zwaveplus_info_attribute_store, public command_class_zwaveplus_info_mqtt
    {

        public:
            command_class_zwaveplus_info();
            ~command_class_zwaveplus_info() = default;

        private:
            sl_status_t on_zwaveplus_info_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_zwaveplus_info_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_zwaveplus_info_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_zwaveplus_info_attribute_map_t payload) override;
            static sl_status_t on_zwaveplus_info_get_interview_requested(command_class_zwaveplus_info_types::zwaveplus_info_get_payload_t payload);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_ZWAVEPLUS_INFO_H
