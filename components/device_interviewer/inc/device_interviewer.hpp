
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

#ifndef DEVICE_INTERVIEWER_H
#define DEVICE_INTERVIEWER_H

#include <any>
#include "sl_status.h"
#include "component_connector_types.hpp"
#include "component_connector_common_events.hpp"
#include "attribute.hpp"
#include "zpc_attribute_store_type_registration.h"
#include "device_interviewer_attribute_store.hpp"
#include "attribute_store_type_registration.h"
#include "attribute_store_defined_attribute_types.h"
#include "command_class_s2_types.hpp"
#include "device_interviewer_types.hpp"
#include "threading.hpp"
#include "init_builder.hpp"
#include "safe_queue.hpp"
#include "component_connector.hpp"
#include "interview_state_machine.hpp"
#include "device_interviewer_mqtt_api.hpp"
#include <memory>

namespace zwave_command_class
{
    // Forward declaration
    class InterviewStateMachine;

    class device_interviewer : public threading::threading, public Initializable
    {
        public:
            device_interviewer();
            ~device_interviewer() = default;

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

            // Threading interface
            void run() override;

            // Event queue for processing events on device_interviewer thread
            static ::threading::safe_queue<device_interviewer_external_event_data> external_event_queue;

        private:
            const std::vector<attribute_schema_t> attributes = {
              // Node Information Group
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), "NODE_INFORMATION_GROUP", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::listening_protocol), "Listening Protocol", static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::optional_protocol), "Optional Protocol", static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::basic_device_class), "Basic Device Class", static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::generic_device_class), "Generic Device Class", static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::specific_device_class), "Specific Device Class", static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::command_class_list), "Command Class List", static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), BYTE_ARRAY_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(node_information_group_attributes_t::command_class_list_length), "Command Class List Length", static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP), U8_STORAGE_TYPE},
            };

            // State machine for managing interview process
            std::unique_ptr<InterviewStateMachine> state_machine;

            // MQTT API for publishing interview terminated
            DeviceInterviewerMqttApi device_interviewer_mqtt_api;

            // Synchronous handler for getting node information (doesn't need state machine)
            static sl_status_t on_get_node_information(const device_interviewer_get_node_information_payload_t &payload_struct, device_interviewer_get_node_information_payload_t &result_struct);

            /**
             * @brief Queue an event to the device_interviewer thread for processing
             *
             * This is the central point for queueing events. All external events that need
             * to be processed by the state machine should go through this method.
             *
             * Event Flow:
             * 1. External events (from other components) -> register_event_handlers() -> queue_event() -> external_event_queue
             * 2. run() thread pops from external_event_queue -> state_machine->process_event()
             * 3. State machine routes to appropriate InterviewStep
             * 4. InterviewStep may fire new events (via component_connector) to request data
             * 5. Those events trigger responses that come back through step 1
             *
             * @param event_type The type of event to queue
             * @param payload The event payload (will be stored as std::any)
             */
            template<typename PayloadType> static void queue_event(device_interviewer_external_event_t event_type, const PayloadType &payload)
            {
                device_interviewer_external_event_data ev;
                ev.event   = event_type;
                ev.payload = payload;
                external_event_queue.push(ev);
            }

            template<typename PayloadType> static void queue_event(device_interviewer_external_event_t event_type, const PayloadType &payload, attribute_store::attribute endpoint_node)
            {
                device_interviewer_external_event_data ev;
                ev.event                = event_type;
                ev.payload              = payload;
                ev.device_endpoint_node = endpoint_node;
                external_event_queue.push(ev);
            }

            /**
             * @brief Register event handlers for external events
             *
             * This method registers all event handlers that listen to external events
             * (from other components) and queue them for processing on the device_interviewer thread.
             */
            void register_event_handlers();

            /**
             * @brief Trigger the start of an interview
             *
             * This method is used to trigger the start of an interview from an external event.
             *
             * @param p The payload containing the interview information
             * @return The status of the interview start operation
             */
            sl_status_t trigger_start_interview(const component_connector_node_added_payload_t &p);
    };
}  // namespace zwave_command_class
#endif  // DEVICE_INTERVIEWER_H
