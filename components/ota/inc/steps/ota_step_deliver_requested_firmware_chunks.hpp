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

#ifndef OTA_STEP_DELIVER_REQUESTED_FIRMWARE_CHUNKS_HPP
#define OTA_STEP_DELIVER_REQUESTED_FIRMWARE_CHUNKS_HPP

#include "ota_step.hpp"

namespace ota
{

    using state_machine::StepResult;

    /**
     * @brief Responds to Firmware Update MD Get with firmware report batches.
     */
    class OtaStepDeliverRequestedFirmwareChunks : public OtaStep
    {
        public:
            OtaStepDeliverRequestedFirmwareChunks() = default;

            std::string name() const override;
            bool handles_external_event(ota_external_event_t event_type) const override;
            StepResult on_enter(OtaSession &session) override;
            StepResult handle_event(OtaSession &session, std::optional<ota_external_event_data> event) override;

        private:
            static StepResult send_abort_md_report(OtaSession &session, uint16_t report_number);
            static StepResult send_firmware_report_batch(OtaSession &session, uint8_t number_of_reports, uint16_t report_number);
    };

}  // namespace ota

#endif  // OTA_STEP_DELIVER_REQUESTED_FIRMWARE_CHUNKS_HPP
