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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "utils.h"

/** Buffer size for a formatted DSK string including null terminator. */
inline constexpr std::size_t DSK_STR_LEN = sizeof("xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx") + 1;

class Utils
{
    public:
        Utils()                         = delete;
        Utils(const Utils &)            = delete;
        Utils &operator=(const Utils &) = delete;
        ~Utils()                        = delete;

        static sl_status_t convert_dsk_str_to_dsk(const char *src, zwave_dsk_t dst);

        static sl_status_t convert_dsk_to_dsk_str(const zwave_dsk_t src, char *dst, size_t dst_max_len);

        static std::string byte_array_to_string(const std::vector<uint8_t> &byte_array);
};

#endif /* UTILS_HPP */
