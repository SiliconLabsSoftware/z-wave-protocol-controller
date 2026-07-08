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

#ifndef STATE_MACHINE_STEP_RESULT_HPP
#define STATE_MACHINE_STEP_RESULT_HPP

#include "sl_status.h"

namespace state_machine
{

    /**
     * @brief Semantic result code returned by steps.
     *
     * Steps report *what happened*, not *where to go next*. The state machine
     * consults the central transition table to resolve the actual next state.
     *
     * - STAY -- waiting for an external event; no state transition
     * - DONE -- step completed via its normal path
     * - SKIP -- step was bypassed (e.g. precondition not met); follow alternate path
     * - FAIL -- unrecoverable error; transition to the failed state
     */
    enum class StepResultCode { STAY, DONE, SKIP, FAIL };

    /**
     * @brief Result from processing an event or entering a step
     */
    struct StepResult {
            sl_status_t status;
            StepResultCode code;

            StepResult(sl_status_t s = SL_STATUS_OK, StepResultCode c = StepResultCode::STAY) : status(s), code(c) {}
    };

}  // namespace state_machine

#endif  // STATE_MACHINE_STEP_RESULT_HPP
