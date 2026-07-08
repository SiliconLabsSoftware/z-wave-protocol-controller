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

#ifndef OTA_STEP_RECORD_PENDING_TRANSFER_ABORT_HPP
#define OTA_STEP_RECORD_PENDING_TRANSFER_ABORT_HPP

#include "ota_step.hpp"

namespace ota
{

    using state_machine::StepResult;

    /**
     * @brief Sets abort_requested; next MD Get sends corrupted last frame per spec.
     */
    class OtaStepRecordPendingTransferAbort : public OtaStep
    {
        public:
            OtaStepRecordPendingTransferAbort() = default;

            std::string name() const override;
            bool handles_external_event(ota_external_event_t event_type) const override;
            StepResult on_enter(OtaSession &session) override;
            StepResult handle_event(OtaSession &session, std::optional<ota_external_event_data> event) override;
    };

}  // namespace ota

#endif  // OTA_STEP_RECORD_PENDING_TRANSFER_ABORT_HPP
