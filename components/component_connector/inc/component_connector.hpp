/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMPONENT_CONNECTOR_HPP
#define COMPONENT_CONNECTOR_HPP

#include <cstdint>
#include <functional>
#include "sl_status.h"
#include <any>
#include <map>
#include <mutex>
#include <vector>
#include <memory>
#include <stdexcept>

#include "zwave_controller_types.h"
#include "zwave_controller_connection_info.h"
#include "zwave_keyset_definitions.h"
#include "zwave_network_management_types.h"
#include "init_builder.hpp"
#include "threading.hpp"
#include "safe_queue.hpp"
#include <string>
#include <future>

// Type definition for event handler function
typedef std::function<sl_status_t(uint32_t event, std::any payload, std::any &result)> component_connector_event_handler_t;

// Helper functions to simplify typed event handling in on_event implementations
namespace component_connector_helpers
{
    // Helper for events without result
    template<typename PayloadType> sl_status_t handle_event_typed(std::any payload, std::function<sl_status_t(const PayloadType &)> handler)
    {
        try {
            const PayloadType &payload_typed = std::any_cast<const PayloadType &>(payload);
            return handler(payload_typed);
        } catch (const std::bad_any_cast &) {
            return SL_STATUS_FAIL;
        }
    }

    // Helper for events with result
    template<typename PayloadType, typename ResultType> sl_status_t handle_event_typed(std::any payload, std::any &result, std::function<sl_status_t(const PayloadType &, ResultType &)> handler)
    {
        try {
            const PayloadType &payload_typed = std::any_cast<const PayloadType &>(payload);
            ResultType result_typed;
            sl_status_t status = handler(payload_typed, result_typed);
            if (status == SL_STATUS_OK) {
                result = std::make_any<ResultType>(result_typed);
            }
            return status;
        } catch (const std::bad_any_cast &) {
            return SL_STATUS_FAIL;
        }
    }
}  // namespace component_connector_helpers

// Internal event queue entry for async processing
struct async_event_data {
        uint32_t event;
        std::any payload;
        std::shared_ptr<std::promise<sl_status_t>> status_promise;
        std::shared_ptr<std::promise<std::any>> result_promise;  // Optional, only for events with results
};

class component_connector : public Initializable, public threading::threading
{
    private:
        // Static event handlers inventory (shared across all instances)
        static std::map<uint32_t, std::vector<component_connector_event_handler_t>> event_handlers;
        static std::mutex event_handlers_mutex;

        // Async event queue (static, shared across all instances)
        static ::threading::safe_queue<async_event_data> event_queue;

        // Internal static fire_event for use by static callbacks
        static sl_status_t fire_event_internal(const uint32_t event, const std::any payload, std::any &result);

        // Connect an event handler to one or more events (legacy interface)
        static sl_status_t connect(const uint32_t event, component_connector_event_handler_t event_handler);

    public:
        component_connector();
        ~component_connector() = default;

        // Initializable interface
        sl_status_t initialize() override;
        int shutdown() override;
        std::string name() const override;

        // Threading interface
        void run() override;  // Process events from queue

        // Type-safe typed registration methods - eliminates need for on_event functions
        // Register a single event with typed handler (no payload, no result)
        template<typename EventEnum> sl_status_t connect_typed(EventEnum event, std::function<void()> handler)
        {
            return connect(static_cast<uint32_t>(event), [handler](uint32_t, std::any p, std::any &r) -> sl_status_t {
                handler();
                return SL_STATUS_OK;
            });
        }

        // Register a single event with typed handler (no result)
        template<typename EventEnum, typename PayloadType> sl_status_t connect_typed(EventEnum event, std::function<sl_status_t(const PayloadType &)> handler)
        {
            return connect(static_cast<uint32_t>(event), [handler](uint32_t, std::any p, std::any &r) -> sl_status_t { return component_connector_helpers::handle_event_typed<PayloadType>(p, handler); });
        }

        // Register a single event with typed handler (with result)
        template<typename EventEnum, typename PayloadType, typename ResultType> sl_status_t connect_typed(EventEnum event, std::function<sl_status_t(const PayloadType &, ResultType &)> handler)
        {
            return connect(static_cast<uint32_t>(event), [handler](uint32_t, std::any p, std::any &r) -> sl_status_t { return component_connector_helpers::handle_event_typed<PayloadType, ResultType>(p, r, handler); });
        }

        // ASYNC API - New async methods

        // True fire-and-forget: queues event without returning a future (most efficient)
        void fire_event(const uint32_t event);

        // True fire-and-forget: queues event without returning a future (most efficient)
        template<typename PayloadType> void fire_event(const uint32_t event, const PayloadType &payload)
        {
            async_event_data event_data;
            event_data.event          = event;
            event_data.payload        = payload;
            event_data.status_promise = nullptr;  // No promise = fire-and-forget
            event_data.result_promise = nullptr;

            event_queue.push(std::move(event_data));
        }

        // Async version that returns a future (use when you need to track completion)
        std::future<sl_status_t> fire_event_async(const uint32_t event);

        // Async version that returns a future (use when you need to track completion)
        template<typename PayloadType> std::future<sl_status_t> fire_event_async(const uint32_t event, const PayloadType &payload)
        {
            auto status_promise = std::make_shared<std::promise<sl_status_t>>();
            auto future         = status_promise->get_future();

            async_event_data event_data;
            event_data.event          = event;
            event_data.payload        = payload;
            event_data.status_promise = status_promise;
            event_data.result_promise = nullptr;

            event_queue.push(std::move(event_data));
            return future;
        }

        // Async version with result
        template<typename PayloadType, typename ResultType> std::future<std::pair<sl_status_t, ResultType>> fire_event_async(const uint32_t event, const PayloadType &payload)
        {
            auto status_promise = std::make_shared<std::promise<sl_status_t>>();
            auto result_promise = std::make_shared<std::promise<std::any>>();
            auto status_future  = status_promise->get_future();
            auto result_future  = result_promise->get_future();

            async_event_data event_data;
            event_data.event          = event;
            event_data.payload        = payload;
            event_data.status_promise = status_promise;
            event_data.result_promise = result_promise;

            event_queue.push(std::move(event_data));

            // Return combined future that waits for both status and result
            // Use shared_future which is copyable, or move futures into the lambda
            auto status_shared = status_future.share();
            auto result_shared = result_future.share();
            return std::async(std::launch::deferred, [status_shared, result_shared]() mutable {
                sl_status_t status = status_shared.get();
                if (status == SL_STATUS_OK && result_shared.valid()) {
                    try {
                        std::any result_any = result_shared.get();
                        if (result_any.has_value()) {
                            ResultType result = std::any_cast<ResultType>(result_any);
                            return std::make_pair(status, result);
                        }
                    } catch (const std::bad_any_cast &) {
                        return std::make_pair(SL_STATUS_FAIL, ResultType {});
                    }
                }
                return std::make_pair(status, ResultType {});
            });
        }
};

#endif  // COMPONENT_CONNECTOR_HPP