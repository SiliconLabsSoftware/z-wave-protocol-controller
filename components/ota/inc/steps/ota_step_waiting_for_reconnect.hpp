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

#ifndef OTA_STEP_WAITING_FOR_RECONNECT_HPP
#define OTA_STEP_WAITING_FOR_RECONNECT_HPP

#include "ota_step.hpp"

namespace ota
{

    using state_machine::StepResult;

    /**
     * @brief Seconds slept before the post-OTA NOP probe.
     *
     * Controller margin on the device-advertised Wait Time. Published MQTT
     * `waittime` stays that advertised value.
     *   - advertised 0 (or missing, stored as 0) → 5 s
     *   - advertised < 20 → advertised + 5 s
     *   - advertised < 60 → advertised + 10 s
     *   - otherwise → advertised + 30 s
     */
    uint32_t post_ota_probe_delay_s(uint16_t advertised_wait_time_s);

    /**
     * @brief Waits for the node to restart and become reachable again after firmware activation.
     *
     * Entered after:
     *   - An immediate-activation success (status 0xFF): WaitTime from the MD Status Report.
     *   - A delayed-activation confirmation (Activation Status 0xFF): WaitTime from the
     *     Activation Status Report.
     *
     * Flow (all synchronous in on_enter):
     *   1. Sleep post_ota_probe_delay_s(session.wait_time) seconds. session.wait_time
     *      stays the device-advertised Wait Time.
     *   2. Send a Z-Wave NOP and block until the TX callback fires.
     *   3. On ACK: return done() to advance to TRIGGER_INTERVIEW.
     *   4. On failure: sleep NOP_RETRY_DELAY_S seconds and retry from step 2.
     */
    class OtaStepWaitingForReconnect : public OtaStep
    {
        public:
            std::string name() const override;
            bool handles_external_event(ota_external_event_t event_type) const override;
            StepResult on_enter(OtaSession &session) override;
            StepResult handle_event(OtaSession &session, std::optional<ota_external_event_data> event) override;

        private:
            static bool send_nop_sync(OtaSession &session);
    };

}  // namespace ota

#endif  // OTA_STEP_WAITING_FOR_RECONNECT_HPP
