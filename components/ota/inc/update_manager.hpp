/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef OTA_UPDATE_MANAGER_HPP
#define OTA_UPDATE_MANAGER_HPP

#include "threading.hpp"
#include "init_builder.hpp"
#include "safe_queue.hpp"

#include "update_manager_types.hpp"
#include "ota_mqtt_api.hpp"
#include "ota_state_machine.hpp"

namespace ota
{

    /**
     * @brief Main component class for OTA Firmware Manager.
     *
     * Follows the same pattern as device_interviewer:
     *   - Inherits from threading::threading (dedicated worker thread)
     *   - Inherits from Initializable (registered in InitBuilder)
     *   - Owns an MQTT API, state machine, and image store
     *   - Uses a thread-safe event queue
     */
    class update_manager : public threading::threading, public Initializable
    {
        public:
            update_manager();
            ~update_manager() = default;

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

            // Threading interface
            void run() override;

            /// Shared event queue between MQTT callbacks and the worker thread
            ::threading::safe_queue<ota_external_event_data> event_queue;

        private:
            OTAMqttApi mqttApi;
            OtaStateMachine stateMachine;

            /**
             * @brief Register component_connector event handlers for Z-Wave reports.
             */
            void register_event_handlers();

            /**
             * @brief Resolve node ID from the CC report, build ZwaveReportPayload, and queue for the worker thread.
             */
            sl_status_t queue_firmware_update_md_report(ota_external_event_t event_kind, const zwave_command_class::command_class_firmware_update_md_types::component_connector_firmware_update_md_report_payload_t &cc_payload);

            /**
             * @brief Queue an event from any thread onto the worker queue.
             */
            void queue_event(ota_external_event_t event_kind, const std::any &payload = {});
    };

}  // namespace ota

#endif  // OTA_UPDATE_MANAGER_HPP
