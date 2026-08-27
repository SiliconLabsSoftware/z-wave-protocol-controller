/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMMAND_CLASS_FIRMWARE_UPDATE_MD_CONSTANTS_H
#define COMMAND_CLASS_FIRMWARE_UPDATE_MD_CONSTANTS_H

#include <cstdint>

#include "command_class_firmware_update_md_generated_types.hpp"

namespace zwave_command_class
{
    namespace command_class_firmware_update_md_constants
    {
        enum class request_report_status : uint8_t {
            not_upgradable = 0x03,
        };

        enum class prepare_report_status : uint8_t {
            not_downloadable = 0x03,
        };

        constexpr uint16_t prepare_report_checksum_when_not_ok = 0x0000;
    }  // namespace command_class_firmware_update_md_constants
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_FIRMWARE_UPDATE_MD_CONSTANTS_H
