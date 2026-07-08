/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by
 * the sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "threading.hpp"
#include "log.h"

namespace threading
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "threading";

    // Define static member
    std::atomic<bool> threading::kill_switch_is_active {false};

    threading::threading(const std::string &thread_name) : thread_name(thread_name), should_stop_flag(false) {}

    threading::~threading()
    {
        stop();
    }

    void threading::start()
    {
        if (thread.joinable()) {
            sl_log_warning(LOG_TAG.data(), "Thread '%s' is already running", thread_name.c_str());
            return;
        }

        should_stop_flag = false;
        thread           = std::thread([this]() { thread_loop(); });
    }

    void threading::stop()
    {
        if (!thread.joinable()) {
            return;  // Already stopped or never started
        }

        // Signal thread to stop
        should_stop_flag = true;

        // Wait for thread to finish
        if (thread.joinable()) {
            thread.join();
        }
    }

    void threading::kill_switch_activate()
    {
        threading::kill_switch_is_active = true;
    }

    bool threading::is_kill_switch_activated()
    {
        return threading::kill_switch_is_active;
    }

    void threading::thread_loop()
    {
        sl_log_debug(LOG_TAG.data(), "Thread '%s' execution started", thread_name.c_str());

        try {
            // Call run() in a loop until should_stop() is true
            // Derived classes should check should_stop() in their run() implementation
            while (!should_stop_flag && !threading::kill_switch_is_active) {
                run();
            }
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Exception in thread '%s' run(): %s", thread_name.c_str(), e.what());
        } catch (...) {
            sl_log_error(LOG_TAG.data(), "Unknown exception in thread '%s' run()", thread_name.c_str());
        }

        sl_log_debug(LOG_TAG.data(), "Thread '%s' execution completed", thread_name.c_str());
    }

}  // namespace threading
