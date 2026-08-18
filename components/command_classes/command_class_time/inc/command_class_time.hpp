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

#ifndef COMMAND_CLASS_TIME_H
#define COMMAND_CLASS_TIME_H

#include "command_class_time_attribute_store.hpp"

namespace zwave_command_class
{

    class command_class_time final : public command_class_time_attribute_store
    {

        public:
            command_class_time();
            ~command_class_time() = default;

        private:
            sl_status_t on_time_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_date_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_time_offset_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_TIME_H
