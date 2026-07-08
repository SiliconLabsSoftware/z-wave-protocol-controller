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

#ifndef OTA_STATE_MACHINE_HPP
#define OTA_STATE_MACHINE_HPP

#include "sl_status.h"
#include "state_machine_base.hpp"
#include "ota_external_event_types.hpp"
#include "update_manager_types.hpp"

#include <unordered_map>

namespace ota
{

    /**
     * @brief OTA Firmware Manager state machine (parallel to InterviewStateMachine).
     *
     * Inherits the generic state_machine_base engine. Multiple sessions can coexist,
     * but only one active data transfer (upload phase) is allowed at a time. A new
     * start request is rejected with an MQTT error report if any session is currently
     * in an active transfer state.
     */
    class OtaStateMachine : public state_machine::state_machine_base<OtaState, OtaSession, ota_external_event_data>
    {
        public:
            OtaStateMachine();
            ~OtaStateMachine() = default;

            /**
             * @brief Process an incoming OTA event.
             *
             * Routes the event to the correct session by event.node_id. Start
             * commands are rejected when any session is in an active data-transfer
             * state, or when the target node already has an active session.
             *
             * @param event  The event to process.
             * @return       Status of processing (parallel to InterviewStateMachine).
             */
            sl_status_t process_event(const ota_external_event_data &event);

        private:
            /// All active OTA sessions, keyed by node ID.
            std::unordered_map<zwave_node_id_t, OtaSession> sessions;

            void register_transitions();

            /**
             * @brief Register all OTA steps.
             */
            void register_steps();

            void start_ota(zwave_node_id_t node_id, const std::string &image_name, bool wait_for_activation);

            /// Returns true if any session is currently in an active data-transfer state.
            bool is_active_transfer_in_progress() const;
    };

}  // namespace ota

#endif  // OTA_STATE_MACHINE_HPP
