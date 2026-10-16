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
#include "timer.hpp"
#include <ctime>
#include <map>
#include <mutex>

namespace zwave_command_class
{

    class command_class_time final : public command_class_time_attribute_store
    {

        public:
            command_class_time();
            ~command_class_time() = default;
            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

        private:
            using endpoint_key_t = std::pair<zwave_node_id_t, zwave_endpoint_id_t>;
            struct monitored_endpoint_t {
                    zwave_home_id_t home_id;
                    uint8_t version;
                    std::time_t last_report_time;
            };

            static void on_periodic_report_timer(void *data);
            static void schedule_periodic_report_timer();
            static std::map<endpoint_key_t, monitored_endpoint_t> monitored_endpoints;
            static std::map<endpoint_key_t, std::time_t> last_time_get_requests;
            static std::mutex periodic_report_mutex;
            static struct timer_handle_t periodic_report_timer;
            static bool periodic_report_timer_started;
            static bool daylight_saving_state_known;
            static int previous_daylight_saving_state;

            sl_status_t on_time_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_date_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_time_offset_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;

            sl_status_t on_time_offset_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map) override;
            sl_status_t on_date_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map) override;
            sl_status_t on_time_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_time_attribute_map_t attribute_map) override;

            sl_status_t on_time_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_date_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_time_offset_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_TIME_H
