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

#include "component_connector.hpp"
#include "log.h"
#include <string_view>

[[maybe_unused]] static constexpr std::string_view LOG_TAG = "component_connector";

// Static member definitions
std::map<uint32_t, std::vector<component_connector_event_handler_t>> component_connector::event_handlers;
std::mutex component_connector::event_handlers_mutex;
::threading::safe_queue<async_event_data> component_connector::event_queue;

// Constructor
component_connector::component_connector() : threading::threading("Component Connector")
{
    // Constructor does not initialize - initialization happens via initialize() method
}

// Initializable interface
sl_status_t component_connector::initialize()
{
    sl_log_info(LOG_TAG.data(), "Component connector initialized");
    return SL_STATUS_OK;
}

int component_connector::shutdown()
{
    stop();  // Stop the background thread
    return 0;
}

std::string component_connector::name() const
{
    return "Component Connector";
}

sl_status_t component_connector::connect(const uint32_t event, component_connector_event_handler_t event_handler)
{
    std::lock_guard<std::mutex> lock(event_handlers_mutex);
    event_handlers[event].push_back(event_handler);
    return SL_STATUS_OK;
}

void component_connector::fire_event(const uint32_t event)
{
    fire_event(event, std::any());
}

std::future<sl_status_t> component_connector::fire_event_async(const uint32_t event)
{
    return fire_event_async(event, std::any());
}

// Internal static fire_event for use by static callbacks
sl_status_t component_connector::fire_event_internal(const uint32_t event, const std::any payload, std::any &result)
{
    // Copy handlers while holding the lock, then release lock before calling them
    // This prevents deadlock if handlers call connect() or fire_event()
    std::vector<component_connector_event_handler_t> handlers_to_call;
    {
        std::lock_guard<std::mutex> lock(event_handlers_mutex);
        auto it = event_handlers.find(event);
        if (it != event_handlers.end()) {
            handlers_to_call = it->second;  // Copy the handlers
            sl_log_debug(LOG_TAG.data(), "Firing event %u to %zu handler(s)", event, handlers_to_call.size());
        } else {
            sl_log_debug(LOG_TAG.data(), "No handlers registered for event %u", event);
            return SL_STATUS_OK;
        }
    }

    // Call handlers without holding the lock to prevent deadlock
    for (const auto &handler: handlers_to_call) {
        if (handler(event, payload, result) != SL_STATUS_OK) {
            return SL_STATUS_FAIL;
        }
    }

    return SL_STATUS_OK;
}

// Thread processing loop - processes events from the queue
void component_connector::run()
{
    // Block for a short period when idle to avoid busy-spinning
    constexpr uint32_t idle_timeout_ms = 1;
    std::optional<async_event_data> ev = event_queue.pop(idle_timeout_ms);

    if (ev.has_value()) {
        async_event_data event_data = ev.value();

        try {
            // Process event synchronously (same as current fire_event_internal)
            std::any result;
            sl_status_t status = fire_event_internal(event_data.event, event_data.payload, result);

            // Set status promise
            if (event_data.status_promise) {
                event_data.status_promise->set_value(status);
            }

            // Set result promise if present
            if (event_data.result_promise) {
                if (result.has_value()) {
                    event_data.result_promise->set_value(result);
                } else {
                    // No result provided, set empty any
                    event_data.result_promise->set_value(std::any {});
                }
            }
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Exception processing event %u: %s", event_data.event, e.what());
            if (event_data.status_promise) {
                event_data.status_promise->set_value(SL_STATUS_FAIL);
            }
            if (event_data.result_promise) {
                event_data.result_promise->set_value(std::any {});
            }
        } catch (...) {
            sl_log_error(LOG_TAG.data(), "Unknown exception processing event %u", event_data.event);
            if (event_data.status_promise) {
                event_data.status_promise->set_value(SL_STATUS_FAIL);
            }
            if (event_data.result_promise) {
                event_data.result_promise->set_value(std::any {});
            }
        }
    }

    if (should_stop()) {
        return;
    }
}