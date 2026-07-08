
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

#ifndef COMMAND_CLASS_WAKE_UP_H
#define COMMAND_CLASS_WAKE_UP_H

#include "command_class_wake_up_mqtt.hpp"
#include "command_class_wake_up_attribute_store.hpp"
#include "command_class_wake_up_types.hpp"

namespace zwave_command_class
{

    class command_class_wake_up final : public command_class_wake_up_attribute_store, public command_class_wake_up_mqtt
    {

        public:
            command_class_wake_up();
            ~command_class_wake_up() = default;
            static void on_wake_up_interval_set_interview_resolution(attribute_store_node_t node);
            /// MQTT / user Interval Set resolution: queues Interval Get only (no interview event).
            static void on_wake_up_interval_set_user_resolution(attribute_store_node_t node);

        private:
            static void on_wake_up_no_more_information_deferred(attribute_store_node_t node_id_node);
            static void send_wake_up_no_more_information(attribute_store_node_t node_id_node);
            static void on_wake_up_no_more_information_resolution_listener(attribute_store_node_t node_id_node);
            static void on_wake_up_no_more_information_sent_listener(attribute_store_node_t wunmi_group_node);
            /// Register resolution-idle listener that sends Wake Up No More Information.
            static void arm_no_more_information_on_resolution_idle(attribute_store_node_t node_id_node);
            static sl_status_t on_arm_no_more_information_requested(const command_class_wake_up_types::wake_up_arm_no_more_information_payload_t &payload);

            sl_status_t on_wake_up_interval_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_wake_up_attribute_map_t payload) override;
            sl_status_t on_wake_up_interval_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_wake_up_attribute_map_t payload) override;
            sl_status_t on_wake_up_notification_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_wake_up_attribute_map_t payload) override;
            static sl_status_t on_wake_up_capabilities_get_interview_requested(command_class_wake_up_types::wake_up_capabilities_get_payload_t payload);
            static sl_status_t on_wake_up_interval_get_interview_requested(command_class_wake_up_types::wake_up_interval_get_payload_t payload);
            static sl_status_t on_wake_up_interval_set_interview_requested(command_class_wake_up_types::wake_up_interval_set_payload_t payload);
            static sl_status_t on_wake_up_interval_requested(const command_class_wake_up_types::wake_up_interval_requested_payload_t &request, wake_up_interval_report_seconds_t &result);

        protected:
            sl_status_t on_wake_up_interval_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_WAKE_UP_H
