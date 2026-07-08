/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

/**
 * @brief Write a byte buffer to a file with security-relevant hardening.
 *
 * Specifically:
 *  - Creates any missing parent directories with mode 0700. Pre-existing
 *    parent directories are NOT modified; their permissions are vetted
 *    by the checks below.
 *  - Refuses if the (already-existing) parent directory is world-writable.
 *  - Refuses if the parent directory is a symlink.
 *  - Writes to "<path>.tmp" with O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW.
 *    A stale ".tmp" from a prior crash is removed and the write retried.
 *  - fsync()s, then rename()s to the final path atomically.
 *  - File is created mode 0600.
 */

#ifndef SECURITY_KEYS_DUMP_WRITER_HPP
#define SECURITY_KEYS_DUMP_WRITER_HPP

#include "sl_status.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace security
{
    /**
     * @brief Atomically write data to path with hardened permissions.
     *
     * @param[in] path  Final destination path (absolute recommended).
     * @param[in] data  Pointer to bytes to write.
     * @param[in] len   Length of the data to write.
     *
     * @return SL_STATUS_OK on success.
     * @return SL_STATUS_NOT_FOUND if the parent directory does not exist.
     * @return SL_STATUS_PERMISSION if the parent directory is world-writable
     *         or a symlink, or if open/write/fsync/rename returns EACCES.
     * @return SL_STATUS_INVALID_PARAMETER if path is empty.
     * @return SL_STATUS_FAIL on other I/O errors.
     */
    sl_status_t write_security_keys_dump(const std::string &path, const uint8_t *data, std::size_t len);
}  // namespace security

#endif  // SECURITY_KEYS_DUMP_WRITER_HPP
