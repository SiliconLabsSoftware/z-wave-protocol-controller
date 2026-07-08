/* # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 */

#include "clock_platform.h"

#include <chrono>

namespace
{
    using Clock        = std::chrono::steady_clock;
    using Milliseconds = std::chrono::milliseconds;

    const Clock::time_point &get_epoch()
    {
        static const Clock::time_point epoch = Clock::now();
        return epoch;
    }
}  // namespace

extern "C" {

int clock_lte(clock_time_t t1, clock_time_t t2)
{
    if (t1 == t2) {
        return 1;
    }
    // Handle wrap-around for unsigned types (RFC 1982)
    clock_time_t diff         = t1 - t2;
    clock_time_t inverse_diff = t2 - t1;
    return static_cast<int>(inverse_diff < diff);
}

clock_time_t clock_time(void)
{
    auto duration = Clock::now() - get_epoch();
    auto ms       = std::chrono::duration_cast<Milliseconds>(duration);
    return static_cast<clock_time_t>(ms.count());
}
}
