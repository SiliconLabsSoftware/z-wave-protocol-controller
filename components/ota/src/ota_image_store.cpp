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

#include "ota_image_store.hpp"
#include "log.h"
#include "zpc_config.h"

#include <filesystem>
#include <fstream>
#include <string_view>
#include <algorithm>

namespace ota
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_image_store";

    static constexpr std::string_view allowed_extension = ".gbl";
    static constexpr size_t kMaxImageSize               = 10 * 1024 * 1024;

    static bool is_gbl(const std::filesystem::path &p)
    {
        auto ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == allowed_extension;
    }

    static bool valid_name(std::string_view name)
    {
        return !name.empty() && name.find('/') == std::string::npos && name.find("..") == std::string::npos;
    }

    std::filesystem::path OTAImageStore::image_path(const std::string &name)
    {
        const zpc_config_t *cfg = zpc_get_config();
        if ((cfg == nullptr) || (cfg->ota_cache_path == nullptr)) {
            return {};
        }

        std::filesystem::path base(cfg->ota_cache_path);
        return base / name;  // safe due to valid_name()
    }

    sl_status_t OTAImageStore::store_image(const std::string &name, const std::vector<uint8_t> &data)
    {
        if (!valid_name(name) || data.empty()) {
            return SL_STATUS_FAIL;
        }

        if (data.size() > kMaxImageSize) {
            sl_log_error(LOG_TAG.data(), "Image '%s' exceeds maximum size (%zu > %zu bytes)", name.c_str(), data.size(), kMaxImageSize);
            return SL_STATUS_FAIL;
        }

        std::filesystem::path path = image_path(name);
        if (!is_gbl(path)) {
            sl_log_error(LOG_TAG.data(), "Only gbl files are allowed: %s", name.c_str());
            return SL_STATUS_FAIL;
        }

        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            sl_log_error(LOG_TAG.data(), "Failed to create directory %s: %s", path.parent_path().string().c_str(), ec.message().c_str());
            return SL_STATUS_FAIL;
        }

        // Atomic write
        auto tmp = path;
        tmp += ".tmp";

        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            sl_log_error(LOG_TAG.data(), "Failed to open temp file: %s", tmp.string().c_str());
            return SL_STATUS_FAIL;
        }

        ofs.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));

        if (!ofs) {
            sl_log_error(LOG_TAG.data(), "Write error for file: %s", tmp.string().c_str());
            return SL_STATUS_FAIL;
        }

        ofs.flush();
        if (!ofs) {
            sl_log_error(LOG_TAG.data(), "Flush error for file: %s", tmp.string().c_str());
            return SL_STATUS_FAIL;
        }

        std::filesystem::rename(tmp, path, ec);
        if (ec) {
            sl_log_error(LOG_TAG.data(), "Rename failed %s -> %s: %s", tmp.string().c_str(), path.string().c_str(), ec.message().c_str());
            return SL_STATUS_FAIL;
        }

        sl_log_info(LOG_TAG.data(), "Stored image '%s' (%zu bytes)", name.c_str(), data.size());
        return SL_STATUS_OK;
    }

    std::vector<std::string> OTAImageStore::list_images()
    {
        std::vector<std::string> result;

        const zpc_config_t *cfg = zpc_get_config();
        if ((cfg == nullptr) || (cfg->ota_cache_path == nullptr)) {
            return result;
        }

        std::filesystem::path base(cfg->ota_cache_path);

        std::error_code ec;
        if (!std::filesystem::exists(base, ec) || ec || !std::filesystem::is_directory(base, ec) || ec) {
            return result;
        }

        for (const auto &entry: std::filesystem::directory_iterator(base, ec)) {
            if (ec) {
                sl_log_error(LOG_TAG.data(), "Directory iteration failed for %s: %s", base.string().c_str(), ec.message().c_str());
                break;
            }

            std::error_code file_ec;
            if (!entry.is_regular_file(file_ec) || file_ec) {
                continue;
            }

            if (is_gbl(entry.path())) {
                result.emplace_back(entry.path().filename().string());
            }
        }

        return result;
    }

    sl_status_t OTAImageStore::remove_image(const std::string &name)
    {
        if (!valid_name(name)) {
            return SL_STATUS_FAIL;
        }

        std::filesystem::path path = image_path(name);
        if (!is_gbl(path)) {
            sl_log_error(LOG_TAG.data(), "Invalid file type for removal: %s", name.c_str());
            return SL_STATUS_FAIL;
        }

        std::error_code ec;
        if (!std::filesystem::remove(path, ec)) {
            if (ec) {
                sl_log_error(LOG_TAG.data(), "Failed to remove image %s: %s", name.c_str(), ec.message().c_str());
                return SL_STATUS_FAIL;
            }
            sl_log_warning(LOG_TAG.data(), "Image not found for removal: %s", name.c_str());
            return SL_STATUS_NOT_FOUND;
        }

        sl_log_info(LOG_TAG.data(), "Removed image '%s'", name.c_str());
        return SL_STATUS_OK;
    }

    std::vector<uint8_t> OTAImageStore::get_image(const std::string &name)
    {
        if (!valid_name(name)) {
            return {};
        }

        std::filesystem::path path = image_path(name);
        if (!is_gbl(path)) {
            return {};
        }

        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        if (!ifs) {
            sl_log_warning(LOG_TAG.data(), "Image not found: %s", name.c_str());
            return {};
        }

        std::streampos size = ifs.tellg();
        if (size <= 0 || static_cast<size_t>(size) > kMaxImageSize) {
            sl_log_error(LOG_TAG.data(), "Invalid image size for %s", name.c_str());
            return {};
        }

        ifs.seekg(0, std::ios::beg);
        if (!ifs) {
            return {};
        }

        std::vector<uint8_t> data(static_cast<size_t>(size));

        ifs.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(size));

        if (!ifs || ifs.gcount() != static_cast<std::streamsize>(size)) {
            sl_log_error(LOG_TAG.data(), "Incomplete read for '%s'", name.c_str());
            return {};
        }

        return data;
    }

}  // namespace ota
