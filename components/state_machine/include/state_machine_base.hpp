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

#ifndef STATE_MACHINE_BASE_HPP
#define STATE_MACHINE_BASE_HPP

#include "step_base.hpp"
#include "step_result.hpp"
#include "log.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

namespace state_machine
{

    /**
     * @brief Generic table-driven state machine engine.
     *
     * @tparam StateEnum    An enum (class) whose values represent states.
     * @tparam SessionType  The session/context struct. Must have a public
     *                      `StateEnum current_state` field.
     * @tparam EventType    The event data struct delivered to the state machine.
     *
     * The engine provides:
     *   - A step registry: state -> step handler
     *   - A transition table: (state, result_code) -> next_state
     *   - transition_to_state() and apply_transition() implementing the
     *     on_enter -> handle_event(nullopt) -> apply_transition dispatch loop.
     *
     * Session management (creating/storing/routing sessions) is intentionally
     * left to the consumer.
     */
    template<typename StateEnum, typename SessionType, typename EventType> class state_machine_base
    {
        public:
            using Step          = step_base<SessionType, EventType>;
            using TransitionKey = std::pair<StateEnum, StepResultCode>;

            /**
             * @brief Construct the state machine engine.
             * @param failed_state  The state value that represents an unrecoverable failure.
             * @param log_tag       A short string used as the logging tag (e.g. "interview_sm").
             */
            explicit state_machine_base(StateEnum failed_state, const char *log_tag = "state_machine") : failed_state_(failed_state), log_tag_(log_tag)
            {
                // Compile-time check: SessionType must have a current_state field of
                // the correct enum type.
                static_assert(std::is_same_v<decltype(SessionType::current_state), StateEnum>, "SessionType must have a public 'StateEnum current_state' field");
            }

            virtual ~state_machine_base() = default;

        protected:
            // ------------------------------------------------------------------
            // Step and transition registration (called by the consumer's ctor)
            // ------------------------------------------------------------------

            /**
             * @brief Register a step handler for the given state.
             */
            void register_step(StateEnum state, std::unique_ptr<Step> step)
            {
                step_registry_[state] = std::move(step);
            }

            /**
             * @brief Add a transition: (from_state, result_code) -> to_state.
             */
            void set_transition(StateEnum from_state, StepResultCode code, StateEnum to_state)
            {
                transition_table_[TransitionKey(from_state, code)] = to_state;
            }

            // ------------------------------------------------------------------
            // Core engine (mirrors InterviewStateMachine logic)
            // ------------------------------------------------------------------

            /**
             * @brief Transition the session to a new state and run the on_enter /
             *        handle_event(nullopt) dispatch loop.
             *
             * The loop continues as long as steps return an immediate result
             * (done/skip/fail). It stops when a step returns stay().
             */
            void transition_to_state(SessionType &session, StateEnum new_state)
            {
                auto from_step = step_registry_.find(session.current_state);
                auto to_step   = step_registry_.find(new_state);

                std::string from_name = (from_step != step_registry_.end()) ? from_step->second->name() : "UNKNOWN";
                std::string to_name   = (to_step != step_registry_.end()) ? to_step->second->name() : "UNKNOWN";

                sl_log_debug(log_tag_, "Transitioning from state %s to %s", from_name.c_str(), to_name.c_str());

                session.current_state = new_state;

                if (new_state == failed_state_) {
                    sl_log_error(log_tag_, "State machine entered FAILED state");
                }

                auto step_it = step_registry_.find(new_state);
                if (step_it != step_registry_.end()) {
                    std::string step_name = step_it->second->name();
                    sl_log_info(log_tag_, "Entering step: %s", step_name.c_str());

                    StepResult entry_result = step_it->second->on_enter(session);
                    if (entry_result.code == StepResultCode::STAY) {
                        entry_result = step_it->second->handle_event(session, std::nullopt);
                    }
                    apply_transition(session, entry_result);
                }
            }

            /**
             * @brief Resolve the next state from the transition table and call
             *        transition_to_state(). STAY results are silently ignored;
             *        FAIL transitions directly to the failed state.
             */
            void apply_transition(SessionType &session, const StepResult &result)
            {
                if (result.code == StepResultCode::STAY) {
                    return;
                }

                if (result.code == StepResultCode::FAIL) {
                    transition_to_state(session, failed_state_);
                    return;
                }

                auto key = TransitionKey(session.current_state, result.code);
                auto it  = transition_table_.find(key);
                if (it != transition_table_.end()) {
                    transition_to_state(session, it->second);
                } else {
                    auto step_it           = step_registry_.find(session.current_state);
                    std::string state_name = (step_it != step_registry_.end()) ? step_it->second->name() : "UNKNOWN";
                    sl_log_error(log_tag_, "No transition defined for state %s with result code %d", state_name.c_str(), static_cast<int>(result.code));
                }
            }

            /**
             * @brief Look up a step handler by state.
             * @return Pointer to the step, or nullptr if no step is registered.
             */
            Step *get_step(StateEnum state) const
            {
                auto it = step_registry_.find(state);
                return (it != step_registry_.end()) ? it->second.get() : nullptr;
            }

            /**
             * @brief True if the transition table defines (from_state, code) -> next state.
             */
            bool has_transition(StateEnum from_state, StepResultCode code) const
            {
                return transition_table_.find(TransitionKey(from_state, code)) != transition_table_.end();
            }

        private:
            std::map<StateEnum, std::unique_ptr<Step>> step_registry_;
            std::map<TransitionKey, StateEnum> transition_table_;
            StateEnum failed_state_;
            const char *log_tag_;
    };

}  // namespace state_machine

#endif  // STATE_MACHINE_BASE_HPP
