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

#ifndef ZWAVE_RX_PROCESS_HPP
#define ZWAVE_RX_PROCESS_HPP

#include "threading.hpp"
#include <mutex>
#include "safe_queue.hpp"
#include "init_builder.hpp"

namespace zwave_component
{
    class zwave_rx_process : public threading::threading, public Initializable
    {
        public:
            zwave_rx_process();
            virtual ~zwave_rx_process();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

            ::threading::safe_queue<int> poll_queue;

        private:
            void run() override;

        protected:
            int zpc_connection_fd;
            bool initialized;
    };
}  // namespace zwave_component

#endif  // ZWAVE_RX_PROCESS_HPP