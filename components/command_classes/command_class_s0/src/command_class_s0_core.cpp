
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
#include "command_class_s0_core.hpp"

// ZPC
#include "zwave_command_class_base.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_frame_parser.hpp"         // zwave_frame_parser
#include "zwave_command_class_indices.h"  // COMMAND_INDEX
#include "attribute_resolver.hpp"         // attribute_resolver
#include "zpc_mqtt_utils.hpp"             // zpc_mqtt::utils::get_base_topic_from_attribute

#include "command_class_s0_constants.hpp"
#include "command_class_s0_types.hpp"

#include "attribute_callbacks.hpp"
#include "log.h"

// JSON
#include <nlohmann/json.hpp>
#include <sys/types.h>

// Format
#include "fmt/format.h"

namespace zwave_command_class
{
    // Log tag
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_s0_core";

    const attribute_list_registration_t attributes = {};

    const command_class_properties cc_properties = {
      .command_class_id                 = COMMAND_CLASS_SECURITY,
      .supported_handler_minimal_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NONE,
      .command_class_name               = "S0",
      .comments                         = "",
      .supported_version                = 1,
      .manual_security_validation       = true,
    };

    command_class_s0_core::command_class_s0_core() : zwave_command_class_base(cc_properties, attributes, "S0") {}

    sl_status_t command_class_s0_core::control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        if (connection_info->encapsulation != ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_0) {
            return SL_STATUS_FAIL;
        }

        attribute_store::attribute endpoint_node(command_class_utils::get_endpoint_node(connection_info));
        // Create parser
        zwave_frame_parser parser(frame_data, frame_length);
        // Store frame name in here to be able to log it in case of error
        std::string debug_frame_name;
        // Report args struct
        const report_received_args report_args = {endpoint_node, parser, connection_info};

        // We encapsulate the switch in a try catch to logs errors
        // This avoids to have to catch exceptions in each command handler
        try {
            switch (frame_data[COMMAND_INDEX]) {
                case static_cast<uint8_t>(command_class_s0_commands_t::S0_COMMANDS_SUPPORTED_REPORT):
                    return on_s0_commands_supported_report_received(report_args);
                default:
                    return SL_STATUS_NOT_SUPPORTED;
            }
            sl_log_debug(LOG_TAG.data(), "%s frame received", debug_frame_name.c_str());
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Error while parsing %s frame : %s", debug_frame_name.c_str(), e.what());
            return SL_STATUS_FAIL;
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_s0_core::support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        return SL_STATUS_OK;
    }

    sl_status_t command_class_s0_core::on_s0_commands_supported_report_received(const report_received_args &args)
    {
        auto endpoint_node          = args.endpoint_node;
        auto report_frame_parser    = args.report_frame_parser;
        auto frame_length           = report_frame_parser.get_frame_length();
        const auto *connection_info = args.connection_info;
        command_class_s0_attribute_map_t attribute_map;

        reports_to_follow_t reports_to_follow = report_frame_parser.read_byte();
        attribute_map.insert({"reports_to_follow", reports_to_follow});

        attribute_store::attribute reports_to_follow_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(s0_reports_to_follow_attributes_t::S0_REPORTS_TO_FOLLOW_ATTRIBUTE));
        reports_to_follow_t previous_reports_to_follow    = reports_to_follow_node.reported_exists() ? reports_to_follow_node.reported<reports_to_follow_t>() : reports_to_follow_t(0);
        attribute_map.insert({"previous_reports_to_follow", previous_reports_to_follow});

        if (frame_length > command_class_s0_constants::SECURE_SUPPORTED_COMMAND_CLASSES_INDEX) {

            uint8_t supported_cc_len = frame_length - command_class_s0_constants::SECURE_SUPPORTED_COMMAND_CLASSES_INDEX;
            std::vector<uint8_t> supported_cc_list;

            if ((supported_cc_len > 0) && (supported_cc_len < ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH)) {
                supported_cc_list = report_frame_parser.read_sequential<std::vector<uint8_t>>(supported_cc_len);
                attribute_map.insert({"supported_cc_list", supported_cc_list});
            } else {
                sl_log_error(LOG_TAG.data(), "Error while parsing S0_COMMANDS_SUPPORTED_REPORT frame");
                return SL_STATUS_FAIL;
            }
        }

        on_s0_commands_supported_report_received_store(endpoint_node, attribute_map);
        return on_s0_commands_supported_report_parsed(connection_info, endpoint_node, attribute_map);
    }

    sl_status_t command_class_s0_core::on_s0_commands_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint_node, command_class_s0_attribute_map_t attribute_map)
    {
        return SL_STATUS_OK;
    }

    sl_status_t command_class_s0_core::on_s0_commands_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_s0_attribute_map_t attribute_map)
    {
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class