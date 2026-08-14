
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

#include <cstdint>
#include <fmt/base.h>
#include <fmt/format.h>
#include <type_traits>
#include <string_view>

// Base class
#include "command_class_version.hpp"
#include "attribute_callbacks.hpp"
#include "attribute_resolver.hpp"
#include "attribute_resolver_rule.h"
#include "attribute_resolver.h"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "command_class_version_events.hpp"
#include "zwave_command_class_indices.h"
#include "zwapi_init.h"
#include "zpc_config.h"
#include "zwave_command_class_manager.h"

#include "zpc_attribute_store_network_helper.h"

#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "device_interviewer.hpp"
#include "device_interviewer_events.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_version";

    command_class_version::command_class_version()
    {
        // Constructor body - can be empty or contain initialization logic
        component_connector connector;
        connector.connect_typed<command_class_version_events_t, command_class_version_types::command_class_version_cc_get_payload_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CC_GET, [](const command_class_version_types::command_class_version_cc_get_payload_t &p) {
            return zwave_command_class::command_class_version::on_version_cc_get_requested(p);
        });

        connector.connect_typed<command_class_version_events_t, command_class_version_types::command_class_get_version_report_payload_t, command_class_version_types::command_class_get_version_report_payload_t>(
          command_class_version_events_t::COMMAND_CLASS_GET_VERSION_REPORT,
          [](const command_class_version_types::command_class_get_version_report_payload_t &p, command_class_version_types::command_class_get_version_report_payload_t &r) { return zwave_command_class::command_class_version::on_get_version_report_requested(p, r); });

        connector.connect_typed<command_class_version_events_t, command_class_version_types::command_class_version_get_payload_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_GET_INTERVIEW, [](const command_class_version_types::command_class_version_get_payload_t &p) {
            return zwave_command_class::command_class_version::on_version_get_interview_requested(p);
        });

        connector.connect_typed<command_class_version_events_t, command_class_version_types::command_class_version_get_payload_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CAPABILITIES_GET_INTERVIEW, [](const command_class_version_types::command_class_version_get_payload_t &p) {
            return zwave_command_class::command_class_version::on_version_capabilities_get_interview_requested(p);
        });

        connector.connect_typed<command_class_version_events_t, command_class_version_types::command_class_version_get_payload_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_ZWAVE_SOFTWARE_GET_INTERVIEW, [](const command_class_version_types::command_class_version_get_payload_t &p) {
            return zwave_command_class::command_class_version::on_version_zwave_software_get_interview_requested(p);
        });
    }

    void command_class_version::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)endpoint_node;
        (void)supported_version;
    }

    sl_status_t command_class_version::on_version_cc_get_requested(command_class_version_types::command_class_version_cc_get_payload_t payload)
    {
        // We cannot ask for the version of extended CCs.
        if (payload.command_class >= EXTENDED_COMMAND_CLASS_IDENTIFIER_START) {
            return SL_STATUS_OK;
        }

        // CC:0086.01.00.21.001 + CL:0086.01.51.01.1: Version CC MUST reside on the
        // Root Device. Always create the GET group under ep0 so the resolver sends
        // to ep0 and the incoming report (always from ep0) stops the right group.
        auto target_node                = payload.device_endpoint_node;
        zwave_node_id_t node_id         = 0;
        zwave_endpoint_id_t endpoint_id = 0;
        if (attribute_store_network_helper_get_zwave_ids_from_node(target_node, &node_id, &endpoint_id) == SL_STATUS_OK && endpoint_id != 0) {
            auto ep0 = attribute_store_get_endpoint_0_node(target_node.parent());
            if (ep0 != ATTRIBUTE_STORE_INVALID_NODE) {
                target_node = attribute_store::attribute(ep0);
            }
        }

        auto group_node = target_node.emplace_node(static_cast<attribute_store_type_t>(version_command_class_get_group_attributes_t::VERSION_COMMAND_CLASS_GET_GROUP));

        auto requested_command_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(version_command_class_get_group_attributes_t::requested_command_class));
        requested_command_class_node.set_desired<uint8_t>(payload.command_class);
        command_class_version_core::start_group_resolution(group_node, {.retry_count = payload.retry_count});

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_command_class_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload)
    {
        command_class_version_types::command_class_version_cc_get_payload_t payload_map_version;
        payload_map_version.device_endpoint_node = endpoint;

        uint8_t command_class                      = 0;
        command_class                              = get_value_or_default(payload, "requested_command_class", command_class);
        payload_map_version.is_first_command_class = false;
        payload_map_version.command_class          = command_class;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_CC_GET), payload_map_version);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_get_interview_requested(command_class_version_types::command_class_version_get_payload_t payload)
    {
        auto version_get_group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_get_group_attributes_t::VERSION_GET_GROUP));
        command_class_version_core::start_group_resolution(version_get_group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_capabilities_get_interview_requested(command_class_version_types::command_class_version_get_payload_t payload)
    {
        auto capabilities_group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_capabilities_get_group_attributes_t::VERSION_CAPABILITIES_GET_GROUP));
        command_class_version_core::start_group_resolution(capabilities_group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_zwave_software_get_interview_requested(command_class_version_types::command_class_version_get_payload_t payload)
    {
        auto group_node = payload.device_endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_get_group_attributes_t::VERSION_ZWAVE_SOFTWARE_GET_GROUP));
        command_class_version_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload)
    {
        command_class_version_types::command_class_version_report_callback_payload_t callback_payload;
        callback_payload.device_endpoint_node = endpoint;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_GET_DONE), callback_payload);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_capabilities_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload)
    {
        command_class_version_types::command_class_version_get_payload_t payload_struct;
        payload_struct.device_endpoint_node = endpoint;
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_VERSION_CAPABILITIES_REPORT), payload_struct);

        uint8_t z_wave_software = 0;
        z_wave_software         = get_value_or_default(payload, "z_wave_software", z_wave_software);
        command_class_version_types::command_class_version_capabilities_report_callback_payload_t interviewer_payload;
        interviewer_payload.device_endpoint_node = endpoint;
        interviewer_payload.z_wave_software      = z_wave_software;
        connector.fire_event(static_cast<uint32_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_CAPABILITIES_DONE), interviewer_payload);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_zwave_software_report_received_store(attribute_store::attribute endpoint_node, command_class_version_attribute_map_t attribute_map)
    {
        sl_status_t status = command_class_version_attribute_store::on_version_zwave_software_report_received_store(endpoint_node, attribute_map);
        if (status == SL_STATUS_OK) {
            command_class_version_types::command_class_version_report_callback_payload_t callback_payload;
            callback_payload.device_endpoint_node = endpoint_node;
            component_connector connector;
            connector.fire_event(static_cast<uint32_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_VERSION_ZWAVE_SOFTWARE_DONE), callback_payload);
        }
        return status;
    }

    sl_status_t command_class_version::on_version_command_class_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto *frame_generator = args.get_frame_generator;
        auto version_node     = args.node;

        auto requested_command_class_node = version_node.emplace_node(static_cast<attribute_store_type_t>(version_command_class_get_group_attributes_t::requested_command_class));
        if (!requested_command_class_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(requested_command_class_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_version::on_version_command_class_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_version_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        uint8_t requested_command_class = 0;
        requested_command_class         = get_value_or_default(attribute_map, "requested_command_class", requested_command_class);

        report_frame.add_raw_byte(requested_command_class);

        auto supported_version = zwave_command_class_manager::get_version(requested_command_class);
        report_frame.add_raw_byte(supported_version);

        frame = report_frame.generate_frame();

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_get_version_report_requested(const command_class_version_types::command_class_get_version_report_payload_t &payload_struct, command_class_version_types::command_class_get_version_report_payload_t &result_struct)
    {
        result_struct.device_node = payload_struct.device_node;

        auto device_node     = payload_struct.device_node;
        auto endpoint_0_node = device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID);

        auto group_node                       = endpoint_0_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::VERSION_REPORT_GROUP));
        auto z_wave_library_type_node         = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::z_wave_library_type));
        auto z_wave_protocol_version_node     = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::z_wave_protocol_version));
        auto z_wave_protocol_sub_version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::z_wave_protocol_sub_version));
        auto firmware_0_version_node          = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::firmware_0_version));
        auto firmware_0_sub_version_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::firmware_0_sub_version));
        auto hardware_version_node            = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::hardware_version));
        auto number_of_firmware_targets_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::number_of_firmware_targets));

        result_struct.z_wave_library_type         = z_wave_library_type_node.reported_exists() ? std::optional<uint8_t>(z_wave_library_type_node.reported<uint8_t>()) : std::nullopt;
        result_struct.z_wave_protocol_version     = z_wave_protocol_version_node.reported_exists() ? std::optional<uint8_t>(z_wave_protocol_version_node.reported<uint8_t>()) : std::nullopt;
        result_struct.z_wave_protocol_sub_version = z_wave_protocol_sub_version_node.reported_exists() ? std::optional<uint8_t>(z_wave_protocol_sub_version_node.reported<uint8_t>()) : std::nullopt;
        result_struct.firmware_0_version          = firmware_0_version_node.reported_exists() ? std::optional<uint8_t>(firmware_0_version_node.reported<uint8_t>()) : std::nullopt;
        result_struct.firmware_0_sub_version      = firmware_0_sub_version_node.reported_exists() ? std::optional<uint8_t>(firmware_0_sub_version_node.reported<uint8_t>()) : std::nullopt;
        result_struct.hardware_version            = hardware_version_node.reported_exists() ? std::optional<uint8_t>(hardware_version_node.reported<uint8_t>()) : std::nullopt;
        result_struct.number_of_firmware_targets  = number_of_firmware_targets_node.reported_exists() ? std::optional<uint8_t>(number_of_firmware_targets_node.reported<uint8_t>()) : std::nullopt;

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_version_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        auto library_type = static_cast<uint8_t>(zwapi_get_library_type());

        zwapi_protocol_version_information_t protocol_info = {};
        zwapi_get_protocol_version(&protocol_info);

        uint8_t firmware_major = 0;
        uint8_t firmware_minor = 0;
        zwapi_get_app_version(&firmware_major, &firmware_minor);

        const auto *config       = zpc_get_config();
        uint8_t hardware_version = static_cast<uint8_t>(config->hardware_version);
        uint8_t firmware_targets = 0;

        report_frame.add_raw_byte(library_type);
        report_frame.add_raw_byte(protocol_info.major_version);
        report_frame.add_raw_byte(protocol_info.minor_version);
        report_frame.add_raw_byte(firmware_major);
        report_frame.add_raw_byte(firmware_minor);
        report_frame.add_raw_byte(hardware_version);
        report_frame.add_raw_byte(firmware_targets);

        frame = report_frame.generate_frame();

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_capabilities_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_version_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        uint8_t properties1 = static_cast<uint8_t>(version_capabilities_report_properties1_attribute_masks_t::version_mask) | static_cast<uint8_t>(version_capabilities_report_properties1_attribute_masks_t::command_class_mask);

        report_frame.add_raw_byte(properties1);

        frame = report_frame.generate_frame();

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version::on_version_zwave_software_get_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_version_attribute_map_t payload)
    {
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class