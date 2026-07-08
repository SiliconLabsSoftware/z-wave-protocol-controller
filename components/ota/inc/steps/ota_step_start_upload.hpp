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

#ifndef OTA_STEP_START_UPLOAD_HPP
#define OTA_STEP_START_UPLOAD_HPP

#include "ota_step.hpp"

#include <string_view>

namespace ota
{

    using state_machine::StepResult;

    /**
     * @brief Start upload step: Validates image exists and sends
     *        Firmware Update MD Request Get.
     *
     * On entry, checks the image store for the requested image. If not found,
     * returns fail(). Otherwise sends the Request Get and waits for the
     * Request Report.
     */
    class OtaStepStartUpload : public OtaStep
    {
        public:
            OtaStepStartUpload() = default;

            std::string name() const override;
            bool handles_external_event(ota_external_event_t event_type) const override;
            StepResult on_enter(OtaSession &session) override;
            StepResult handle_event(OtaSession &session, std::optional<ota_external_event_data> event) override;

        private:
            static void publish_report(const OtaSession &session, std::string_view report_status, std::string_view report_reason = {});
            static StepResult handle_request_report(OtaSession &session, const ZwaveReportPayload &report);
    };

}  // namespace ota

#endif  // OTA_STEP_START_UPLOAD_HPP
