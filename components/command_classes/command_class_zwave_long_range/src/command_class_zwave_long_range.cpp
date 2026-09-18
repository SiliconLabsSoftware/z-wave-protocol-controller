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

#include "command_class_zwave_long_range.hpp"
#include "zwapi_protocol_controller.h"
#include "zwave_controller_keyset.h"
#include "zwave_command_class_indices.h"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_zwave_long_range";

    command_class_zwave_long_range::command_class_zwave_long_range() {}

    sl_status_t command_class_zwave_long_range::support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        if (frame_length <= COMMAND_INDEX) {
            return SL_STATUS_NOT_SUPPORTED;
        }

        sl_log_info(LOG_TAG.data(), "Protocol command received from NodeID %d:%d", connection_info->remote.node_id, connection_info->remote.endpoint_id);

        sl_status_t status = zwapi_transfer_protocol_cc(connection_info->remote.node_id, zwave_controller_get_key_from_encapsulation(connection_info->encapsulation), frame_length, frame_data);

        switch (status) {
            case SL_STATUS_OK:
                sl_log_info(LOG_TAG.data(), "Command from NodeID %d:%d was handled successfully.", connection_info->remote.node_id, connection_info->remote.endpoint_id);
                break;

            case SL_STATUS_FAIL:
                sl_log_warning(LOG_TAG.data(),
                               "Command from NodeID %d:%d had an error during handling. "
                               "Not all parameters were accepted",
                               connection_info->remote.node_id,
                               connection_info->remote.endpoint_id);
                break;

            case SL_STATUS_BUSY:
                sl_log_warning(LOG_TAG.data(),
                               "Frame handler is busy and could not handle frame from "
                               "NodeID %d:%d correctly.",
                               connection_info->remote.node_id,
                               connection_info->remote.endpoint_id);
                break;

            case SL_STATUS_NOT_SUPPORTED:
                sl_log_warning(LOG_TAG.data(),
                               "Command from NodeID %d:%d got rejected because it is not supported. "
                               "It was possibly also rejected due to security filtering",
                               connection_info->remote.node_id,
                               connection_info->remote.endpoint_id);
                break;

            default:
                sl_log_warning(LOG_TAG.data(), "Command from NodeID %d:%d had an unexpected return status: 0x%04X\n", connection_info->remote.node_id, connection_info->remote.endpoint_id, status);
                break;
        }
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
