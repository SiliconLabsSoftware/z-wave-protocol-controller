/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by
 * the sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

/**
 * \file
 *         Timer component implementation using C++ chrono
 * \brief  C++ chrono-based timer implementation and C API wrapper
 */

#include "timer.hpp"
#include <functional>

namespace timer
{

    // Global timer manager instance
    static TimerManager *g_timer_manager = nullptr;

    void initialize_timer_manager()
    {
        if (g_timer_manager == nullptr) {
            g_timer_manager = new TimerManager();
            g_timer_manager->init();
        }
    }

    TimerManager *get_timer_manager()
    {
        if (g_timer_manager == nullptr) {
            initialize_timer_manager();
        }
        return g_timer_manager;
    }

}  // namespace timer

extern "C" {

void timer_init(void)
{
    timer::initialize_timer_manager();
}

void timer_set(struct timer_handle_t *t, uint64_t interval, void (*callback)(void *), void *ptr)
{
    if (t == nullptr) {
        return;
    }

    auto *manager = timer::get_timer_manager();
    if (t->ptr == nullptr) {
        t->ptr = manager->create_timer();
    } else {
        if (!manager->timer_exists(t->ptr)) {
            t->ptr = manager->create_timer();
        }
    }

    std::function<void()> callback_wrapper = [callback, ptr]() {
        if (callback) {
            callback(ptr);
        }
    };

    manager->set_timer(t->ptr, interval, std::move(callback_wrapper));
}

void timer_stop(struct timer_handle_t *t)
{
    if (t == nullptr || t->ptr == nullptr) {
        return;
    }
    timer::get_timer_manager()->stop_timer(t->ptr);
}

void timer_restart(struct timer_handle_t *t)
{
    if (t == nullptr || t->ptr == nullptr) {
        return;
    }
    timer::get_timer_manager()->restart_timer(t->ptr);
}

void timer_reset(struct timer_handle_t *t)
{
    if (t == nullptr || t->ptr == nullptr) {
        return;
    }
    timer::get_timer_manager()->reset_timer(t->ptr);
}

bool timer_expired(struct timer_handle_t *t)
{
    if (t == nullptr || t->ptr == nullptr) {
        return true;  // Not set = expired
    }
    return timer::get_timer_manager()->expired_timer(t->ptr);
}

bool timer_running(struct timer_handle_t *t)
{
    if (t == nullptr || t->ptr == nullptr) {
        return false;  // Not set = not running
    }
    return timer::get_timer_manager()->is_timer_running(t->ptr);
}

}  // extern "C"
