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
#include "attribute_store_process.h"
#include "attribute_store_internal.h"
#include "attribute_store_configuration_internal.h"

// Common component
#include "clock_platform.h"

#include "timer.hpp"

// ZPC components
#include "log.h"

// Generic includes
#include <stdbool.h>

#include "attribute_store_handler.hpp"

// Log tag for this file.
#define LOG_TAG "attribute_store_process"

// Private variables
// Safety timer, that will ensure that we back-up to the datastore following
// at least this interval.
static struct timer_handle_t attribute_store_auto_save_safety_timer = {0};
// Cooldown timer since last attribute modification.
static struct timer_handle_t attribute_store_auto_save_cooldown_timer = {0};

static void attribute_store_auto_save_timer_expired_event(void *ptr);

////////////////////////////////////////////////////////////////////////////////
// Private event functions
////////////////////////////////////////////////////////////////////////////////
static void attribute_store_process_on_restart_auto_save_safety_timer_event()
{
    timer_stop(&attribute_store_auto_save_safety_timer);
    unsigned int auto_save_safety_interval = attribute_store_get_auto_save_safety_interval();
    if (auto_save_safety_interval > 0) {
        sl_log_debug(LOG_TAG,
                     "Restarting Attribute Store auto-save safety timer for "
                     "%d seconds.\n",
                     auto_save_safety_interval);
        timer_set(&attribute_store_auto_save_safety_timer, auto_save_safety_interval * CLOCK_SECOND, attribute_store_auto_save_timer_expired_event, 0);
    }
}

static void attribute_store_process_on_restart_auto_save_cooldown_timer_event()
{
    timer_stop(&attribute_store_auto_save_cooldown_timer);
    unsigned int auto_save_cooldown_interval = attribute_store_get_auto_save_cooldown_interval();
    if (auto_save_cooldown_interval > 0) {
        sl_log_debug(LOG_TAG,
                     "Restarting Attribute Store auto-save cooldown timer for "
                     "%d seconds.\n",
                     auto_save_cooldown_interval);
        timer_set(&attribute_store_auto_save_cooldown_timer, auto_save_cooldown_interval * CLOCK_SECOND, attribute_store_auto_save_timer_expired_event, 0);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Functions shared within the component
////////////////////////////////////////////////////////////////////////////////
void attribute_store_process_restart_auto_save_safety_timer()
{
    if (!timer_running(&attribute_store_auto_save_safety_timer)) {
        attribute_store_process_on_restart_auto_save_safety_timer_event();
    }
}

void attribute_store_process_restart_auto_save_cooldown_timer()
{

    if (!timer_running(&attribute_store_auto_save_cooldown_timer)) {
        attribute_store_process_on_restart_auto_save_cooldown_timer_event();
    }
}

void attribute_store_process_on_attribute_store_saved()
{
    timer_stop(&attribute_store_auto_save_cooldown_timer);
    attribute_store_process_restart_auto_save_safety_timer();
}

static void attribute_store_auto_save_timer_expired_event(void *ptr)
{
    sl_log_debug(LOG_TAG,
                 "Auto-save (cooldown or safety) interval elapsed. "
                 "Saving attributes to the datastore.\n");
    attribute_store_save_to_datastore();
}

namespace zwave_component
{
    zwave_component::attribute_store_handler::attribute_store_handler()
    {
        sl_log_info(LOG_TAG, "Process started. Setting up timers\n");
        attribute_store_process_on_attribute_store_saved();
    }

    zwave_component::attribute_store_handler::~attribute_store_handler()
    {
        sl_log_info(LOG_TAG, "Process exited. Stopping timers\n");
        timer_stop(&attribute_store_auto_save_safety_timer);
        timer_stop(&attribute_store_auto_save_cooldown_timer);
    }

    sl_status_t zwave_component::attribute_store_handler::initialize()
    {
        return SL_STATUS_OK;
    }

    int zwave_component::attribute_store_handler::shutdown()
    {
        return 0;
    }

    std::string zwave_component::attribute_store_handler::name() const
    {
        return "Attribute Store Handler";
    }
}  // namespace zwave_component
