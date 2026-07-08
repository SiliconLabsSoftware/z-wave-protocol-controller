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

#ifndef ZPC_STATUS_H
#define ZPC_STATUS_H

#include "sl_status.h"

/**
 * @brief ZPC-specific status codes extending sl_status_t.
 *
 * Values start at SL_STATUS_PRINT_INFO_MESSAGE (0x0900) + 0x1000 = 0x1900
 * to leave ample room for future upstream sl_status additions.
 */
enum class zpc_status_t : sl_status_t {
    NETWORK_MANAGEMENT_BUSY = 0x1901,
    FACTORY_RESET_ONGOING   = 0x1902,
};

#endif  // ZPC_STATUS_H
