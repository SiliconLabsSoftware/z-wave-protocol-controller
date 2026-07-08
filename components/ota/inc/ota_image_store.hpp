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

#ifndef OTA_IMAGE_STORE_HPP
#define OTA_IMAGE_STORE_HPP

#include "sl_status.h"

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace ota
{

    /**
     * @brief Manages firmware image files on the local filesystem.
     *
     * Images are stored under the OTA cache directory obtained from
     * zpc_get_config()->ota_cache_path.
     */
    class OTAImageStore
    {
        public:
            OTAImageStore()  = default;
            ~OTAImageStore() = default;

            /**
             * @brief Store a firmware image to the cache directory.
             * @param name     Filename (relative, no path separators).
             * @param data     Raw binary content.
             * @return SL_STATUS_OK on success.
             */
            static sl_status_t store_image(const std::string &name, const std::vector<uint8_t> &data);

            /**
             * @brief List all image files in the cache directory.
             * @return Vector of filenames.
             */
            static std::vector<std::string> list_images();

            /**
             * @brief Remove an image from the cache directory.
             * @param name  Filename to delete.
             * @return SL_STATUS_OK on success, SL_STATUS_NOT_FOUND if absent.
             */
            static sl_status_t remove_image(const std::string &name);

            /**
             * @brief Read an image from the cache directory.
             * @param name  Filename to read.
             * @return Binary content, or empty vector if the file does not exist or could not be read fully.
             */
            static std::vector<uint8_t> get_image(const std::string &name);

        private:
            /**
             * @brief Build full filesystem path for an image.
             */
            static std::filesystem::path image_path(const std::string &name);
    };

}  // namespace ota

#endif  // OTA_IMAGE_STORE_HPP
