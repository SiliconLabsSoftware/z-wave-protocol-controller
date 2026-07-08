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

#include "security_keys_dump_writer.hpp"

#include "log.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace security
{
    namespace
    {
        constexpr std::string_view LOG_TAG = "security_keys_dump_writer";

        // Split "/a/b/c.bin" -> "/a/b" (parent). Returns "." if no slash.
        std::string parent_dir(const std::string &path)
        {
            const auto pos = path.find_last_of('/');
            if (pos == std::string::npos) {
                return ".";
            }
            if (pos == 0) {
                return "/";
            }
            return path.substr(0, pos);
        }

        // Check the parent directory exists, is a real directory (not a symlink),
        // and is not world-writable.
        sl_status_t validate_parent_dir(const std::string &dir)
        {
            struct stat st {};
            if (lstat(dir.c_str(), &st) != 0) {
                const int err = errno;
                sl_log_error(LOG_TAG.data(), "Cannot stat parent dir '%s': %s", dir.c_str(), std::strerror(err));
                return (err == ENOENT) ? SL_STATUS_NOT_FOUND : SL_STATUS_FAIL;
            }
            if (S_ISLNK(st.st_mode)) {
                sl_log_error(LOG_TAG.data(), "Refusing to write into symlinked parent dir '%s'", dir.c_str());
                return SL_STATUS_PERMISSION;
            }
            if (!S_ISDIR(st.st_mode)) {
                sl_log_error(LOG_TAG.data(), "Parent '%s' is not a directory", dir.c_str());
                return SL_STATUS_FAIL;
            }
            if ((st.st_mode & S_IWOTH) != 0) {
                sl_log_error(LOG_TAG.data(), "Refusing to write into world-writable parent dir '%s' (mode 0%o)", dir.c_str(), st.st_mode & 07777);
                return SL_STATUS_PERMISSION;
            }
            return SL_STATUS_OK;
        }

        sl_status_t make_parent_dir(const std::string &dir)
        {
            if (dir.empty() || dir == "/" || dir == ".") {
                return SL_STATUS_OK;
            }
            struct stat st {};
            if (lstat(dir.c_str(), &st) == 0) {
                return SL_STATUS_OK;
            }
            if (errno != ENOENT) {
                sl_log_error(LOG_TAG.data(), "lstat('%s') failed: %s", dir.c_str(), std::strerror(errno));
                return SL_STATUS_FAIL;
            }

            sl_status_t st_parent = make_parent_dir(parent_dir(dir));
            if (st_parent != SL_STATUS_OK) {
                return st_parent;
            }
            if (mkdir(dir.c_str(), S_IRWXU) != 0 && errno != EEXIST) {
                const int err = errno;
                sl_log_error(LOG_TAG.data(), "mkdir('%s', 0700) failed: %s", dir.c_str(), std::strerror(err));
                return (err == EACCES) ? SL_STATUS_PERMISSION : SL_STATUS_FAIL;
            }
            return SL_STATUS_OK;
        }

        int open_tmp_for_write(const std::string &tmp_path)
        {
            const int flags   = O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC;
            const mode_t mode = S_IRUSR | S_IWUSR;  // 0600

            int fd = ::open(tmp_path.c_str(), flags, mode);
            if (fd >= 0 || errno != EEXIST) {
                return fd;
            }

            struct stat st {};
            if (lstat(tmp_path.c_str(), &st) != 0) {
                if (errno == ENOENT) {
                    return ::open(tmp_path.c_str(), flags, mode);
                }
                sl_log_error(LOG_TAG.data(), "lstat(%s) failed after EEXIST: %s", tmp_path.c_str(), std::strerror(errno));
                return -1;
            }
            if (S_ISLNK(st.st_mode)) {
                sl_log_error(LOG_TAG.data(), "Refusing to replace symlink temp file '%s'", tmp_path.c_str());
                errno = ELOOP;
                return -1;
            }
            if (!S_ISREG(st.st_mode)) {
                sl_log_error(LOG_TAG.data(), "Refusing to replace non-regular temp file '%s'", tmp_path.c_str());
                errno = EINVAL;
                return -1;
            }

            sl_log_warning(LOG_TAG.data(), "Removing stale temp file '%s'", tmp_path.c_str());
            if (::unlink(tmp_path.c_str()) != 0) {
                sl_log_error(LOG_TAG.data(), "unlink(%s) failed: %s", tmp_path.c_str(), std::strerror(errno));
                return -1;
            }

            return ::open(tmp_path.c_str(), flags, mode);
        }
    }  // namespace

    sl_status_t write_security_keys_dump(const std::string &path, const uint8_t *data, std::size_t len)
    {
        if (path.empty()) {
            return SL_STATUS_INVALID_PARAMETER;
        }
        if (len > 0 && data == nullptr) {
            return SL_STATUS_NULL_POINTER;
        }

        const std::string dir = parent_dir(path);
        sl_status_t st        = make_parent_dir(dir);
        if (st != SL_STATUS_OK) {
            return st;
        }
        st = validate_parent_dir(dir);
        if (st != SL_STATUS_OK) {
            return st;
        }

        // Write to <path>.tmp first, then atomic rename.
        const std::string tmp_path = path + ".tmp";

        const int fd = open_tmp_for_write(tmp_path);
        if (fd < 0) {
            const int err = errno;
            sl_log_error(LOG_TAG.data(), "open(%s) failed: %s", tmp_path.c_str(), std::strerror(err));
            return (err == EACCES || err == ELOOP) ? SL_STATUS_PERMISSION : SL_STATUS_FAIL;
        }

        std::size_t written = 0;
        while (written < len) {
            const ssize_t n = ::write(fd, data + written, len - written);
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                sl_log_error(LOG_TAG.data(), "write(%s) failed: %s", tmp_path.c_str(), std::strerror(errno));
                ::close(fd);
                ::unlink(tmp_path.c_str());
                return SL_STATUS_FAIL;
            }
            written += static_cast<std::size_t>(n);
        }

        if (::fsync(fd) != 0) {
            sl_log_error(LOG_TAG.data(), "fsync(%s) failed: %s", tmp_path.c_str(), std::strerror(errno));
            ::close(fd);
            ::unlink(tmp_path.c_str());
            return SL_STATUS_FAIL;
        }
        if (::close(fd) != 0) {
            sl_log_error(LOG_TAG.data(), "close(%s) failed: %s", tmp_path.c_str(), std::strerror(errno));
            ::unlink(tmp_path.c_str());
            return SL_STATUS_FAIL;
        }

        if (::rename(tmp_path.c_str(), path.c_str()) != 0) {
            const int err = errno;
            sl_log_error(LOG_TAG.data(), "rename(%s -> %s) failed: %s", tmp_path.c_str(), path.c_str(), std::strerror(err));
            ::unlink(tmp_path.c_str());
            return (err == EACCES) ? SL_STATUS_PERMISSION : SL_STATUS_FAIL;
        }

        return SL_STATUS_OK;
    }
}  // namespace security
