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
#include "zwave_node_id_definitions.h"
#include <cstdint>
#include <string>

namespace zwave_command_class
{
    /**
     * @brief MQTT API for device interviewer
     *
     * Publishes a message when a device interview has fully terminated. The
     * completion event is handled on the Device Interviewer worker so report
     * data can be fetched without blocking the Component Connector worker.
     */
    class DeviceInterviewerMqttApi : public MqttApiBase
    {
        public:
            DeviceInterviewerMqttApi();
            ~DeviceInterviewerMqttApi() = default;

            void setup_mqtt_api() override;

            static sl_status_t publish_interview_terminated_report(const component_connector_interview_done_payload_t &payload);

        private:
            inline static std::string MQTT_API_INTERVIEW_TERMINATED_TOPIC = "Interview/Report";
    };
}  // namespace zwave_command_class

#endif  // DEVICE_INTERVIEWER_MQTT_API_H
