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
#ifndef THREADING_HPP
#define THREADING_HPP

#include <thread>
#include <atomic>
#include <string>
#include "sl_status.h"

namespace threading
{
    class threading
    {
        public:
            threading(const std::string &thread_name);
            ~threading();

            virtual void start(void);
            void stop(void);
            static void kill_switch_activate(void);
            static bool is_kill_switch_activated(void);

            /**
             * @brief Check if thread should stop (for use in derived class run() loops)
             * @return true if thread should stop, false otherwise
             */
            bool should_stop() const
            {
                return should_stop_flag;
            }

            virtual void run() = 0;

        protected:
            std::string thread_name;
            std::thread thread;
            std::atomic<bool> should_stop_flag;
            static std::atomic<bool> kill_switch_is_active;

        private:
            void thread_loop();
    };
}  // namespace threading

#endif  // THREADING_HPP
