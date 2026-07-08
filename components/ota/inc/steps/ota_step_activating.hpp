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

#ifndef OTA_STEP_ACTIVATING_HPP
#define OTA_STEP_ACTIVATING_HPP

#include "ota_step.hpp"

namespace ota
{

    using state_machine::StepResult;

    /**
     * @brief Entered after sending Firmware Update Activation Set.
     *
     * Waits for a Firmware Update Activation Status Report (0xFF = success).
     * On success, stores WaitTime in the session and transitions to
     * WAITING_FOR_RECONNECT so the node can restart before re-interview.
     *
     * Spec note (CC version caveat): For CC version ≤ 6 the device is not
     * required to wait for the Activation Set before applying the firmware;
     * a spontaneous status report may arrive before the Activation Set is
     * acknowledged. This step logs a warning in that case but otherwise
     * follows the same path.
     */
    class OtaStepActivating : public OtaStep
    {
        public:
            OtaStepActivating() = default;

            std::string name() const override;
            bool handles_external_event(ota_external_event_t event_type) const override;
            StepResult on_enter(OtaSession &session) override;
            StepResult handle_event(OtaSession &session, std::optional<ota_external_event_data> event) override;
    };

}  // namespace ota

#endif  // OTA_STEP_ACTIVATING_HPP
