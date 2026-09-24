/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "attribute_timeouts.h"

// Includes from other ZPC components
#include "attribute_store_helper.h"
#include "log.h"

// Timer component
#include "timer.hpp"

// Generic includes
#include <map>
#include <mutex>
#include <vector>

constexpr char LOG_TAG[] = "attribute_timeouts";

// Private functions prototypes
static void attribute_timeout_restart_watch_timer();
static void attribute_timeout_invoke_timeout_functions(void *user);
static void on_attribute_node_deleted(attribute_store_node_t deleted_node);

///////////////////////////////////////////////////////////////////////////////
// Local definitions
///////////////////////////////////////////////////////////////////////////////
typedef struct attribute_timeout {
        // Timestamp from when we can call the callback
        clock_time_t timestamp;
        // Callback function to invoke
        attribute_timeout_callback_t callback_function;
} attribute_timeout_t;

///////////////////////////////////////////////////////////////////////////////
// Private variables
///////////////////////////////////////////////////////////////////////////////
// List of registered timeouts for attributes.
static std::multimap<attribute_store_node_t, attribute_timeout_t> attribute_timeouts;
static std::mutex timeout_list_mutex;
static std::mutex timeout_callback_mutex;

// Private timer for timeouts
static struct timer_handle_t watch_timer = {nullptr};

///////////////////////////////////////////////////////////////////////////////
// Private helper functions
///////////////////////////////////////////////////////////////////////////////
static bool attribute_timeout_cancel_callback_safe(attribute_store_node_t node, attribute_timeout_callback_t callback_function)
{
    auto range = attribute_timeouts.equal_range(node);
    for (auto it = range.first; it != range.second; it++) {
        if (it->second.callback_function == callback_function) {
            attribute_timeouts.erase(it);
            return true;
        }
    }
    return false;
}

static void attribute_timeout_restart_watch_timer()
{
    // Find out among all timeouts, who is next to "expire"
    clock_time_t next_timeout                  = 0;
    clock_time_t now                           = clock_time();
    attribute_store_node_t next_node_to_expire = ATTRIBUTE_STORE_INVALID_NODE;

    // Go through all the nodes pending a Get Command response
    for (auto item: attribute_timeouts) {
        clock_time_t timeout = item.second.timestamp;

        if ((timeout > now) && ((next_timeout == 0) || (timeout < next_timeout))) {
            next_timeout        = timeout;
            next_node_to_expire = item.first;
        }
    }

    // Do we have a next timeout?
    if (next_timeout != 0) {
        clock_time_t time_until_new_timeout = next_timeout - now;
        timer_set(&watch_timer, time_until_new_timeout, attribute_timeout_invoke_timeout_functions, nullptr);

        sl_log_debug(LOG_TAG,
                     "(Re-)Started attribute watch timer for %u ms. "
                     "Next node to expire: %d",
                     time_until_new_timeout,
                     next_node_to_expire);
    }
}

static void attribute_timeout_invoke_timeout_functions(void *user)
{
    std::lock_guard<std::mutex> callback_lock(timeout_callback_mutex);
    while (true) {
        attribute_store_node_t node           = ATTRIBUTE_STORE_INVALID_NODE;
        attribute_timeout_callback_t callback = nullptr;

        {
            std::lock_guard<std::mutex> lock(timeout_list_mutex);
            clock_time_t now = clock_time();
            auto it          = attribute_timeouts.begin();
            for (; it != attribute_timeouts.end(); ++it) {
                if (it->second.timestamp <= now) {
                    break;
                }
            }

            if (it == attribute_timeouts.end()) {
                attribute_timeout_restart_watch_timer();
                return;
            }

            node     = it->first;
            callback = it->second.callback_function;
            attribute_timeouts.erase(it);
        }

        sl_log_debug(LOG_TAG, "Timeout for Attribute ID %d. Invoking callback", node);
        callback(node);
    }
}

static void on_attribute_node_deleted(attribute_store_node_t deleted_node)
{
    std::lock_guard<std::mutex> lock(timeout_list_mutex);

    // Cancel all the callbacks for that node, if we had any.
    if (attribute_timeouts.contains(deleted_node)) {
        attribute_timeouts.erase(deleted_node);
        // Check if that affects our timer:
        attribute_timeout_restart_watch_timer();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t attribute_timeouts_init()
{
    attribute_store_register_delete_callback(&on_attribute_node_deleted);
    std::lock_guard<std::mutex> lock(timeout_list_mutex);
    attribute_timeouts.clear();
    return SL_STATUS_OK;
}

int attribute_timeouts_teardown()
{
    std::lock_guard<std::mutex> callback_lock(timeout_callback_mutex);
    std::lock_guard<std::mutex> lock(timeout_list_mutex);
    attribute_timeouts.clear();
    timer_stop(&watch_timer);
    return 0;
}

sl_status_t attribute_timeout_set_callback(attribute_store_node_t node, clock_time_t duration, attribute_timeout_callback_t callback_function)
{
    // Don't crash by accepting any function.
    if (callback_function == nullptr) {
        sl_log_warning(LOG_TAG, "Attribute timeout callback rejected for Attribute ID %d", node);
        return SL_STATUS_FAIL;
    }
    if (ATTRIBUTE_STORE_INVALID_NODE == node) {
        sl_log_debug(LOG_TAG,
                     "Attribute timeout callback rejected due to"
                     "invalid Attribute (ID = ATTRIBUTE_STORE_INVALID_NODE)");
        return SL_STATUS_FAIL;
    }

    if (duration == 0) {
        // Don't bother starting the timer and call the callback immediately
        callback_function(node);
        return SL_STATUS_OK;
    }

    attribute_timeout new_timeout = {};
    new_timeout.callback_function = callback_function;
    new_timeout.timestamp         = clock_time() + duration;

    std::lock_guard<std::mutex> lock(timeout_list_mutex);

    // Replace the same node/callback pair as one operation.
    attribute_timeout_cancel_callback_safe(node, callback_function);
    attribute_timeouts.insert(std::make_pair(node, new_timeout));

    // Make sure our timer runs against the nearest timeout:
    attribute_timeout_restart_watch_timer();

    sl_log_debug(LOG_TAG, "Starting timeout for Attribute ID %d with duration: %lu ms", node, duration);
    return SL_STATUS_OK;
}

bool attribute_timeout_is_callback_active(attribute_store_node_t node, attribute_timeout_callback_t callback_function)
{
    std::lock_guard<std::mutex> lock(timeout_list_mutex);
    auto range = attribute_timeouts.equal_range(node);
    for (auto it = range.first; it != range.second; it++) {
        if (it->second.callback_function == callback_function) {
            return true;
        }
    }

    return false;
}

sl_status_t attribute_timeout_cancel_callback(attribute_store_node_t node, attribute_timeout_callback_t callback_function)
{
    std::lock_guard<std::mutex> lock(timeout_list_mutex);
    return attribute_timeout_cancel_callback_safe(node, callback_function) ? SL_STATUS_OK : SL_STATUS_NOT_FOUND;
}
