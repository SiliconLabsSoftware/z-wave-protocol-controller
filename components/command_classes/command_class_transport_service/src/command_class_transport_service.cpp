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

#include "command_class_transport_service.hpp"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_transport_service";

    command_class_transport_service::command_class_transport_service() {}

    sl_status_t command_class_transport_service::control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        (void)connection_info;
        (void)frame_data;
        (void)frame_length;
        sl_log_warning(LOG_TAG.data(),
                       "Incoming application level frame for the Transport Service "
                       "Command Class. This must not have happened, it should have "
                       "been processed by the transport layer.");
        return SL_STATUS_NOT_SUPPORTED;
    }

}  // namespace zwave_command_class
