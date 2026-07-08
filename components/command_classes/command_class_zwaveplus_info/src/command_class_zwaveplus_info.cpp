
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
#include "command_class_zwaveplus_info.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "command_class_zwaveplus_info_events.hpp"
#include "zpc_attribute_store.h"
#include "zpc_attribute_store_network_helper.h"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_zwaveplus_info";

    command_class_zwaveplus_info::command_class_zwaveplus_info()
    {
        component_connector connector;
        connector.connect_typed<command_class_zwaveplus_info_events_t, command_class_zwaveplus_info_types::zwaveplus_info_get_payload_t>(command_class_zwaveplus_info_events_t::COMMAND_CLASS_ZWAVEPLUS_INFO_GET_INTERVIEW, [](const command_class_zwaveplus_info_types::zwaveplus_info_get_payload_t &p) {
            return zwave_command_class::command_class_zwaveplus_info::on_zwaveplus_info_get_interview_requested(p);
        });
    }

    sl_status_t command_class_zwaveplus_info::on_zwaveplus_info_get_interview_requested(command_class_zwaveplus_info_types::zwaveplus_info_get_payload_t payload)
    {
        auto group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(zwaveplus_info_get_group_attributes_t::ZWAVEPLUS_INFO_GET_GROUP));
        command_class_zwaveplus_info_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_zwaveplus_info::on_zwaveplus_info_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_zwaveplus_info_attribute_map_t payload)
    {
        command_class_zwaveplus_info_types::zwaveplus_info_report_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_zwaveplus_info_events_t::COMMAND_CLASS_ZWAVEPLUS_INFO_REPORT_RECEIVED), callback_payload);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_zwaveplus_info::on_zwaveplus_info_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_zwaveplus_info_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        report_frame.add_raw_byte(static_cast<uint8_t>(command_class_zwaveplus_info_core::properties.supported_version));
        report_frame.add_raw_byte(static_cast<uint8_t>(ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_CONTROLLER_CENTRAL_STATIC));
        report_frame.add_raw_byte(static_cast<uint8_t>(ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE));
        report_frame.add_raw_byte(static_cast<uint8_t>((ICON_TYPE_GENERIC_CENTRAL_CONTROLLER >> 8) & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>(ICON_TYPE_GENERIC_CENTRAL_CONTROLLER & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>((ICON_TYPE_GENERIC_CENTRAL_CONTROLLER >> 8) & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>(ICON_TYPE_GENERIC_CENTRAL_CONTROLLER & 0xFF));

        frame = report_frame.generate_frame();

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class