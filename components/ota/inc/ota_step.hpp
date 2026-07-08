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

#ifndef OTA_STEP_HPP
#define OTA_STEP_HPP

#include "ota_external_event_types.hpp"
#include "update_manager_types.hpp"
#include "step_base.hpp"

namespace ota
{
    using StepResult = state_machine::StepResult;

    /**
     * @brief Base class for OTA upload steps (parallel to InterviewStep).
     */
    class OtaStep : public state_machine::step_base<OtaSession, ota_external_event_data>
    {
        public:
            ~OtaStep() override = default;

            /**
             * @brief Whether this step handles the given external event in its current state.
             */
            virtual bool handles_external_event(ota_external_event_t event_type) const = 0;
    };

}  // namespace ota

#endif  // OTA_STEP_HPP
