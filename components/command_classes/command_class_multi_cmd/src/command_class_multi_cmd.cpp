
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

#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_multi_cmd.hpp"
#include "command_class_multi_cmd_constants.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

// ZPC components
#include "log.h"
#include "zwave_command_class_indices.h"
#include "zwave_command_class_manager.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_multi_cmd";
    using command_class_multi_cmd_constants::encap_header_size;
    using command_class_multi_cmd_constants::inner_min_length;

    command_class_multi_cmd::command_class_multi_cmd()
    {
        // Constructor body - can be empty or contain initialization logic
    }

    sl_status_t command_class_multi_cmd::support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        if (frame_length < encap_header_size || frame_data[COMMAND_CLASS_INDEX] != COMMAND_CLASS_MULTI_CMD || frame_data[COMMAND_INDEX] != MULTI_CMD_ENCAP) {
            return SL_STATUS_NOT_SUPPORTED;
        }

        const uint8_t number_of_commands = frame_data[2];
        uint16_t offset                  = encap_header_size;

        sl_log_debug(LOG_TAG.data(), "MULTI_CMD_ENCAP received with %u encapsulated command(s)", number_of_commands);

        for (uint8_t i = 0; i < number_of_commands; i++) {
            if (offset >= frame_length) {
                sl_log_warning(LOG_TAG.data(), "MULTI_CMD_ENCAP truncated at command %u of %u", i + 1, number_of_commands);
                return SL_STATUS_FAIL;
            }

            const uint8_t command_length = frame_data[offset];
            offset++;

            if (command_length < inner_min_length || offset + command_length > frame_length) {
                sl_log_warning(LOG_TAG.data(), "MULTI_CMD_ENCAP command %u has invalid length %u", i + 1, command_length);
                return SL_STATUS_FAIL;
            }

            // Same re-injection as Supervision: unwrap and dispatch the inner command.
            const sl_status_t status = zwave_command_class_manager::dispatch(connection_info, &frame_data[offset], command_length);
            if (status != SL_STATUS_OK) {
                sl_log_debug(LOG_TAG.data(), "Encapsulated command %u dispatch status 0x%04X; continuing", i + 1, status);
            }

            offset += command_length;
        }

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
