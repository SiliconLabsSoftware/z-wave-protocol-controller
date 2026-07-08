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

#ifndef STATE_MACHINE_STEP_BASE_HPP
#define STATE_MACHINE_STEP_BASE_HPP

#include "step_result.hpp"
#include "sl_status.h"

#include <optional>
#include <string>

namespace state_machine
{

    /**
     * @brief Generic base class for state machine steps.
     *
     * @tparam SessionType  The session/context struct used by the concrete
     *                      state machine. Must have a public `current_state` field.
     * @tparam EventType    The event data struct delivered to the state machine.
     *
     * Each step represents one phase of a state-machine-driven process.
     * Steps handle events and return semantic result codes; the engine
     * resolves the next state via the transition table.
     */
    template<typename SessionType, typename EventType> class step_base
    {
        public:
            virtual ~step_base() = default;

            /**
             * @brief Called when the state machine enters this step.
             *
             * Evaluate skip conditions here. Return skip() if the step is not
             * applicable, or stay() to proceed. When stay() is returned the engine
             * immediately calls handle_event(session, std::nullopt) so the step can
             * perform its initial action.
             *
             * @param session  The active session context
             * @return skip() or stay()
             */
            virtual StepResult on_enter(SessionType &session) = 0;

            /**
             * @brief Handle an event in this step.
             *
             * Called with std::nullopt immediately after on_enter returns stay() --
             * the step should perform its initial action and return stay().
             * Called with a real event when an external event arrives.
             *
             * @param session  The active session context
             * @param event    The external event, or std::nullopt on initial entry
             * @return Result indicating what happened (done/skip/stay/fail)
             */
            virtual StepResult handle_event(SessionType &session, std::optional<EventType> event) = 0;

            /**
             * @brief Get the human-readable name of this step (used for logging).
             * @return Step name
             */
            virtual std::string name() const = 0;

        protected:
            /** Step completed normally; engine follows the DONE transition. */
            static StepResult done(sl_status_t status = SL_STATUS_OK)
            {
                return StepResult(status, StepResultCode::DONE);
            }

            /** Step was bypassed; engine follows the SKIP transition. */
            static StepResult skip(sl_status_t status = SL_STATUS_OK)
            {
                return StepResult(status, StepResultCode::SKIP);
            }

            /** Waiting for an external event; no state transition. */
            static StepResult stay(sl_status_t status = SL_STATUS_OK)
            {
                return StepResult(status, StepResultCode::STAY);
            }

            /** Unrecoverable error; engine transitions to the failed state. */
            static StepResult fail(sl_status_t status = SL_STATUS_FAIL)
            {
                return StepResult(status, StepResultCode::FAIL);
            }
    };

}  // namespace state_machine

#endif  // STATE_MACHINE_STEP_BASE_HPP
