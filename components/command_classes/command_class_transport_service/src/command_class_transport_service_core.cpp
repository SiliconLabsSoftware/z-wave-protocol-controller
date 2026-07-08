
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

#include <string_view>

// Base class
#include "command_class_transport_service_core.hpp"

// ZPC
#include "zwave_command_class_base.h"
#include "zwave_frame_parser.hpp"         // zwave_frame_parser
#include "zwave_command_class_indices.h"  // COMMAND_INDEX
#include "attribute_resolver.hpp"         // attribute_resolver
#include "zpc_mqtt_utils.hpp"             // zpc_mqtt::utils::get_base_topic_from_attribute

#include "attribute_callbacks.hpp"
#include "log.h"

#include "zwapi_protocol_controller.h"

// JSON
#include <nlohmann/json.hpp>
#include <sys/types.h>

// Format
#include "fmt/format.h"

namespace zwave_command_class
{
    // Log tag
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_transport_service_core";

    const attribute_list_registration_t attributes = {};

    const command_class_properties cc_properties = {
      .command_class_id                 = 85,
      .supported_handler_minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NONE,
      .command_class_name               = "Transport Service",
      .comments                         = "",
      .supported_version                = 2,
    };

    command_class_transport_service_core::command_class_transport_service_core() : zwave_command_class_base(cc_properties, attributes, "TransportService") {}

    sl_status_t command_class_transport_service_core::support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        // Nothing is really supported here. Transport service is a transport exclusively.
        return SL_STATUS_NOT_SUPPORTED;
    }

    sl_status_t command_class_transport_service_core::control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        // Nothing is really controlled here. Transport service is a transport exclusively.
        sl_log_warning(LOG_TAG.data(),
                       "Incoming application level frame for the Transport Service "
                       "Command Class. This must not have happened, it should have "
                       "been processed by the transport layer.");
        return SL_STATUS_NOT_SUPPORTED;
    }

}  // namespace zwave_command_class