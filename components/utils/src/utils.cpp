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

#include "utils.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <charconv>
#include <iterator>
#include <string_view>
#include <system_error>

sl_status_t Utils::convert_dsk_str_to_dsk(const char *src, zwave_dsk_t dst)
{
    if (src == nullptr || dst == nullptr) {
        return SL_STATUS_INVALID_PARAMETER;
    }
    std::string_view const sv {src, static_cast<std::size_t>(std::find(src, src + DSK_STR_LEN, '\0') - src)};
    if (sv.empty()) {
        return SL_STATUS_FAIL;
    }
    const char *p         = sv.data();
    const char *const end = sv.data() + sv.size();
    unsigned int dst_idx  = 0;
    do {
        if (dst_idx >= ZWAVE_DSK_LENGTH) {
            return SL_STATUS_WOULD_OVERFLOW;
        }
        int num_val  = 0;
        auto const r = std::from_chars(p, end, num_val, 10);
        if (r.ec != std::errc {} || r.ptr == p || num_val < 0 || num_val > 0xFFFF) {
            return SL_STATUS_FAIL;
        }
        dst[dst_idx++] = static_cast<uint8_t>((num_val >> 8) & 0xFF);
        dst[dst_idx++] = static_cast<uint8_t>(num_val & 0xFF);
        p              = r.ptr;
        if (p < end) {
            ++p;
        }
    } while (p < end);
    return SL_STATUS_OK;
}

sl_status_t Utils::convert_dsk_to_dsk_str(const zwave_dsk_t src, char *dst, size_t dst_max_len)
{
    if (src == nullptr || dst == nullptr) {
        return SL_STATUS_INVALID_PARAMETER;
    }
    if (dst_max_len < DSK_STR_LEN) {
        return SL_STATUS_WOULD_OVERFLOW;
    }
    size_t index = 0;
    for (size_t i = 0; i < sizeof(zwave_dsk_t); i += 2) {
        int const d       = (src[i] << 8) | src[i + 1];
        size_t const room = dst_max_len - index;
        auto const r      = fmt::format_to_n(&dst[index], room, FMT_STRING("{:05d}-"), d);
        if (r.size > room) {
            return SL_STATUS_WOULD_OVERFLOW;
        }
        index += r.size;
    }
    if (index > 0) {
        dst[index - 1] = '\0';
    }
    return SL_STATUS_OK;
}

std::string Utils::byte_array_to_string(const std::vector<uint8_t> &byte_array)
{
    std::string out;
    out.reserve(byte_array.size() * 2U);
    for (uint8_t const byte: byte_array) {
        fmt::format_to(std::back_inserter(out), FMT_STRING("{:02X}"), byte);
    }
    return out;
}

extern "C" {

sl_status_t convert_dsk_to_dsk_str(const zwave_dsk_t src, char *dst, size_t dst_max_len)
{
    return Utils::convert_dsk_to_dsk_str(src, dst, dst_max_len);
}

}  // extern "C"
