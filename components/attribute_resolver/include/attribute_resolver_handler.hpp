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

#ifndef ATTRIBUTE_RESOLVER_HANDLE_H
#define ATTRIBUTE_RESOLVER_HANDLE_H

#include <any>
#include "threading.hpp"
#include "init_builder.hpp"
#include "safe_queue.hpp"
#include "attribute_store.h"
#include "clock_platform.h"
#include "sl_status.h"
#include <string>

namespace zwave_component
{
    class attribute_resolver_handler : public threading::threading, public Initializable
    {
        public:
            attribute_resolver_handler();
            ~attribute_resolver_handler();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        public:
            enum class attribute_resolver_event_t {
                NEXT_EVENT,
                WATCH_EVENT,
                TIMER_SET_EVENT,
            };

            struct attribute_resolver_event_data {
                    attribute_resolver_event_t event;
                    std::any data;
            };

            static ::threading::safe_queue<attribute_resolver_event_data> event_queue;

        private:
            void run() override;
    };
}  // namespace zwave_component

#endif  // ATTRIBUTE_RESOLVER_HANDLE_H
