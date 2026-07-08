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

#ifndef ZWAVE_TX_PROCESS_HPP
#define ZWAVE_TX_PROCESS_HPP

#include "threading.hpp"
#include "safe_queue.hpp"
#include "zwave_tx_process.h"
#include "init_builder.hpp"

namespace zwave_component
{
    /**
     * @brief Event structure for Z-Wave TX process
     */
    struct zwave_tx_event {
            zwave_tx_events_t event_type;
            void *data;
    };

    /**
     * @brief Z-Wave TX Process class using C++ threading
     */
    class zwave_tx_process : public threading::threading, public Initializable
    {
        public:
            zwave_tx_process();
            virtual ~zwave_tx_process();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

            /**
             * @brief Post an event to the process
             * @param event_type The event type
             * @param data Optional data pointer
             */
            void post_event(zwave_tx_events_t event_type, void *data = nullptr);

        private:
            void run() override;
            static void handle_event(zwave_tx_event event);
            void initialize_internal();
            void cleanup();

            ::threading::safe_queue<zwave_tx_event> event_queue;
            bool initialized;
    };
}  // namespace zwave_component

#endif  // ZWAVE_TX_PROCESS_HPP