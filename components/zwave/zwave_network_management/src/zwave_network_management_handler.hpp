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

#ifndef ZWAVE_NETWORK_MANAGEMENT_HANDLER_HPP
#define ZWAVE_NETWORK_MANAGEMENT_HANDLER_HPP

#include "threading.hpp"
#include "safe_queue.hpp"
#include "init_builder.hpp"
#include "zwave_network_management_process.h"
#include "nm_state_machine.h"

namespace zwave_component
{
    class zwave_network_management_handler : public threading::threading, public Initializable
    {
        public:
            zwave_network_management_handler();
            ~zwave_network_management_handler();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        public:
            struct nm_event_data {
                    nm_event_t event;
                    void *data;
                    smartstart_event_data_t smartstart;
            };

            static ::threading::safe_queue<nm_event_data> event_queue;

        private:
            void run() override;
    };
}  // namespace zwave_component

#endif  // ZWAVE_NETWORK_MANAGEMENT_HANDLER_HPP
