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

#ifndef CLOCK_PLATFORM_H
#define CLOCK_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Clock time type - milliseconds since an arbitrary epoch.
 */
typedef unsigned long clock_time_t;

/**
 * A second, measured in milliseconds.
 */
#define CLOCK_SECOND 1000

/**
 * Check if clock time t1 is less than or equal to clock time t2.
 *
 * Since clock_time_t is an unsigned type, this function correctly handles
 * wrap-around of clock time values.
 *
 * \see E.g., IETF RFC 1982 https://tools.ietf.org/rfc/rfc1982.txt.
 *
 * @param t1 First clock time value
 * @param t2 Second clock time value
 * @return Non-zero if t1 <= t2 (modulo wrap-around), 0 otherwise
 */
int clock_lte(clock_time_t t1, clock_time_t t2);

/**
 * Get the current clock time.
 *
 * This function returns the current system clock time in milliseconds
 * since an arbitrary epoch. Uses a monotonic clock source.
 *
 * \return The current clock time, measured in milliseconds.
 */
clock_time_t clock_time(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_PLATFORM_H */
