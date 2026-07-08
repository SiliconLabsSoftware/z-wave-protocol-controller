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

#ifndef DEVICE_INTERVIEWER_MQTT_API_H
#define DEVICE_INTERVIEWER_MQTT_API_H

#include "mqtt_api_base.hpp"
#include "sl_status.h"
#include "component_connector_types.hpp"
#include <string>

namespace zwave_command_class
{
    /**
     * @brief MQTT API for device interviewer
     *
     * Listens to COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED and publishes a
     * message when a device interview has fully terminated — i.e. after every
     * command class on_interview-triggered resolution has completed, so the
     * device is actually ready for clients.
     */
    class DeviceInterviewerMqttApi : public MqttApiBase
    {
        public:
            DeviceInterviewerMqttApi();
            ~DeviceInterviewerMqttApi() = default;

            void setup_mqtt_api() override;

        private:
            inline static std::string MQTT_API_INTERVIEW_TERMINATED_TOPIC = "Interview/Report";

            static sl_status_t on_interview_terminated(const component_connector_interview_done_payload_t &payload);
    };
}  // namespace zwave_command_class

#endif  // DEVICE_INTERVIEWER_MQTT_API_H
