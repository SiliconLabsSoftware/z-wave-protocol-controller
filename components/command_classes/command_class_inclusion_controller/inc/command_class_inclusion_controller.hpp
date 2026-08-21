
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

#ifndef COMMAND_CLASS_INCLUSION_CONTROLLER_H
#define COMMAND_CLASS_INCLUSION_CONTROLLER_H

#include "command_class_inclusion_controller_attribute_store.hpp"

#include "component_connector_types.hpp"
#include "timer.hpp"
#include "zwave_node_id_definitions.h"

#include <cstdint>

namespace zwave_command_class
{

    class command_class_inclusion_controller final : public command_class_inclusion_controller_attribute_store
    {

        public:
            command_class_inclusion_controller();
            ~command_class_inclusion_controller() = default;

            sl_status_t on_initiate_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_inclusion_controller_attribute_map_t payload) override;
            sl_status_t on_complete_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_inclusion_controller_attribute_map_t payload) override;

        private:
            // CC:0074.01.01.11.011: 60 s upper bound on the entire INITIATE -> COMPLETE handshake.
            // We use the same timeout for the NIF-wait and node-add stages; the timer is restarted at
            // each successful state transition.
            static constexpr uint32_t SESSION_TIMEOUT_MS = 60000;

            enum class session_state_t : uint8_t {
                idle,
                waiting_nif,
                waiting_node_added,
                // CC:0074.01.01.11.005: when the joining node only advertises S0,
                // the SIS delegates S0 bootstrapping back to the inclusion
                // controller via INITIATE (Step ID = S0_INCLUSION) and waits for
                // the controller's COMPLETE before answering the original INITIATE.
                waiting_ic_complete,
            };

            struct session_t {
                    session_state_t state;
                    zwave_node_id_t inclusion_controller_node_id;
                    zwave_node_id_t included_node_id;
                    uint8_t step_id;
                    zwave_node_info_t node_info;
            };

            static struct timer_handle_t timer;
            static session_t session;

            // Deferred interview request for nodes assigned by another controller.
            //
            // When system_events fires COMPONENT_CONNECTOR_NODE_ID_ASSIGNED_BY_OTHER_CONTROLLER,
            // we don't yet know whether the originating controller is an Inclusion Controller about
            // to hand off via INITIATE. We arm a short timer; if INITIATE arrives first we cancel it
            // (the proxy/S0 path will produce a real NODE_ADDED). If the timer expires we emit
            // COMPONENT_CONNECTOR_NODE_INTERVIEW_REQUESTED so the device_interviewer can probe the
            // node directly (no handoff is coming). At most one such request is in flight on a
            // Z-Wave network at any time.
            static constexpr uint32_t HANDOFF_GRACE_MS = 5000;

            struct deferred_interview_t {
                    bool armed;
                    zwave_node_id_t node_id;
            };

            static struct timer_handle_t deferred_interview_timer;
            static deferred_interview_t deferred_interview;

            static sl_status_t on_node_information_received(const component_connector_node_information_received_payload_t &payload);
            static sl_status_t on_node_added(const component_connector_node_added_payload_t &payload);
            static sl_status_t on_node_id_assigned_by_other_controller(const component_connector_node_id_assigned_by_other_controller_payload_t &payload);
            static sl_status_t on_node_deleted(const component_connector_node_deleted_payload_t &payload);
            static void on_session_timeout(void *data);
            static void on_deferred_interview_timeout(void *data);

            static void send_complete(zwave_node_id_t inclusion_controller_node_id, uint8_t step_id, uint8_t status);
            static void send_initiate(zwave_node_id_t inclusion_controller_node_id, zwave_node_id_t included_node_id, uint8_t step_id);
            static void finish_session_failed();
            static void clear_session();

            static void arm_deferred_interview(zwave_node_id_t node_id);
            static void cancel_deferred_interview(zwave_node_id_t node_id);
            static void fire_node_interview_requested(zwave_node_id_t node_id);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_INCLUSION_CONTROLLER_H
