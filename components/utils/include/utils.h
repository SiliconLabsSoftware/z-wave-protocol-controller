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

#ifndef UTILS_H
#define UTILS_H

#include "zwave_node_id_definitions.h"
#include "sl_status.h"
#include "zwave_controller_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a zwave_dsk_t to a String representation.
 *
 * String representation in format
 * "xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx" as decimal numbers
 *
 * @param src DSK to write to string
 * @param dst Buffer to store string representation in
 * @param dst_max_len Size of the dst buffer to prevent overflow,
 *                    shall be large enough for the formatted string plus null
 *                    terminator (C++ callers may use DSK_STR_LEN from utils.hpp)
 * @return SL_STATUS_OK for success
 * @return SL_STATUS_WOULD_OVERFLOW if char buffer is too small
 */
sl_status_t convert_dsk_to_dsk_str(const zwave_dsk_t src, char *dst, size_t dst_max_len);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
