
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
#include <string_view>
#include "sl_status.h"

// Base class
#include "command_class_version.hpp"
#include "command_class_version_types.hpp"
#include "command_class_version_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_version_attribute_store";

    command_class_version_attribute_store::command_class_version_attribute_store() {}

    sl_status_t command_class_version_attribute_store::on_version_command_class_report_received_store(attribute_store::attribute endpoint_node, command_class_version_attribute_map_t attribute_map)
    {
        version_command_class_report_requested_command_class_t requested_command_class = 0;
        requested_command_class                                                        = get_value_or_default(attribute_map, "requested_command_class", requested_command_class);

        version_command_class_report_command_class_version_t command_class_version = 0;
        command_class_version                                                      = get_value_or_default(attribute_map, "command_class_version", command_class_version);

        auto version_node = endpoint_node.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(requested_command_class));
        version_node.set_reported<version_command_class_report_command_class_version_t>(command_class_version);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version_attribute_store::on_version_report_received_store(attribute_store::attribute endpoint_node, command_class_version_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::VERSION_REPORT_GROUP));

        version_report_z_wave_library_type_t z_wave_library_type = 0;
        z_wave_library_type                                      = get_value_or_default(attribute_map, "z_wave_library_type", z_wave_library_type);
        auto z_wave_library_type_node                            = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::z_wave_library_type));
        z_wave_library_type_node.set_reported<version_report_z_wave_library_type_t>(z_wave_library_type);

        version_report_z_wave_protocol_version_t z_wave_protocol_version = 0;
        z_wave_protocol_version                                          = get_value_or_default(attribute_map, "z_wave_protocol_version", z_wave_protocol_version);
        auto z_wave_protocol_version_node                                = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::z_wave_protocol_version));
        z_wave_protocol_version_node.set_reported<version_report_z_wave_protocol_version_t>(z_wave_protocol_version);

        version_report_z_wave_protocol_sub_version_t z_wave_protocol_sub_version = 0;
        z_wave_protocol_sub_version                                              = get_value_or_default(attribute_map, "z_wave_protocol_sub_version", z_wave_protocol_sub_version);
        auto z_wave_protocol_sub_version_node                                    = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::z_wave_protocol_sub_version));
        z_wave_protocol_sub_version_node.set_reported<version_report_z_wave_protocol_sub_version_t>(z_wave_protocol_sub_version);

        version_report_firmware_0_version_t firmware_0_version = 0;
        firmware_0_version                                     = get_value_or_default(attribute_map, "firmware_0_version", firmware_0_version);
        auto firmware_0_version_node                           = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::firmware_0_version));
        firmware_0_version_node.set_reported<version_report_firmware_0_version_t>(firmware_0_version);

        version_report_firmware_0_sub_version_t firmware_0_sub_version = 0;
        firmware_0_sub_version                                         = get_value_or_default(attribute_map, "firmware_0_sub_version", firmware_0_sub_version);
        auto firmware_0_sub_version_node                               = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::firmware_0_sub_version));
        firmware_0_sub_version_node.set_reported<version_report_firmware_0_sub_version_t>(firmware_0_sub_version);

        version_report_hardware_version_t hardware_version = 0;
        hardware_version                                   = get_value_or_default(attribute_map, "hardware_version", hardware_version);
        auto hardware_version_node                         = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::hardware_version));
        hardware_version_node.set_reported<version_report_hardware_version_t>(hardware_version);

        version_report_number_of_firmware_targets_t number_of_firmware_targets = 0;
        number_of_firmware_targets                                             = get_value_or_default(attribute_map, "number_of_firmware_targets", number_of_firmware_targets);
        auto number_of_firmware_targets_node                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(version_report_group_attributes_t::number_of_firmware_targets));
        number_of_firmware_targets_node.set_reported<version_report_number_of_firmware_targets_t>(number_of_firmware_targets);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version_attribute_store::on_version_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_version_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_capabilities_report_group_attributes_t::VERSION_CAPABILITIES_REPORT_GROUP));

        uint8_t version   = 0;
        version           = get_value_or_default(attribute_map, "version", version);
        auto version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(version_capabilities_report_group_attributes_t::version));
        version_node.set_reported<uint8_t>(version);

        uint8_t command_class   = 0;
        command_class           = get_value_or_default(attribute_map, "command_class", command_class);
        auto command_class_node = group_node.emplace_node(static_cast<attribute_store_type_t>(version_capabilities_report_group_attributes_t::command_class));
        command_class_node.set_reported<uint8_t>(command_class);

        uint8_t z_wave_software   = 0;
        z_wave_software           = get_value_or_default(attribute_map, "z_wave_software", z_wave_software);
        auto z_wave_software_node = group_node.emplace_node(static_cast<attribute_store_type_t>(version_capabilities_report_group_attributes_t::z_wave_software));
        z_wave_software_node.set_reported<uint8_t>(z_wave_software);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_version_attribute_store::on_version_zwave_software_report_received_store(attribute_store::attribute endpoint_node, command_class_version_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::VERSION_ZWAVE_SOFTWARE_REPORT_GROUP));

        version_zwave_software_report_sdk_version_t sdk_version = 0;
        sdk_version                                             = get_value_or_default(attribute_map, "sdk_version", sdk_version);
        auto sdk_version_node                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::sdk_version));
        sdk_version_node.set_reported<version_zwave_software_report_sdk_version_t>(sdk_version);

        version_zwave_software_report_application_framework_api_version_t application_framework_api_version = 0;
        application_framework_api_version                                                                   = get_value_or_default(attribute_map, "application_framework_api_version", application_framework_api_version);
        auto application_framework_api_version_node                                                         = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::application_framework_api_version));
        application_framework_api_version_node.set_reported<version_zwave_software_report_application_framework_api_version_t>(application_framework_api_version);

        version_zwave_software_report_application_framework_build_number_t application_framework_build_number = 0;
        application_framework_build_number                                                                    = get_value_or_default(attribute_map, "application_framework_build_number", application_framework_build_number);
        auto application_framework_build_number_node                                                          = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::application_framework_build_number));
        application_framework_build_number_node.set_reported<version_zwave_software_report_application_framework_build_number_t>(application_framework_build_number);

        version_zwave_software_report_host_interface_version_t host_interface_version = 0;
        host_interface_version                                                        = get_value_or_default(attribute_map, "host_interface_version", host_interface_version);
        auto host_interface_version_node                                              = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::host_interface_version));
        host_interface_version_node.set_reported<version_zwave_software_report_host_interface_version_t>(host_interface_version);

        version_zwave_software_report_host_interface_build_number_t host_interface_build_number = 0;
        host_interface_build_number                                                             = get_value_or_default(attribute_map, "host_interface_build_number", host_interface_build_number);
        auto host_interface_build_number_node                                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::host_interface_build_number));
        host_interface_build_number_node.set_reported<version_zwave_software_report_host_interface_build_number_t>(host_interface_build_number);

        version_zwave_software_report_z_wave_protocol_build_number_t z_wave_protocol_build_number = 0;
        z_wave_protocol_build_number                                                              = get_value_or_default(attribute_map, "z_wave_protocol_build_number", z_wave_protocol_build_number);
        auto z_wave_protocol_build_number_node                                                    = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::z_wave_protocol_build_number));
        z_wave_protocol_build_number_node.set_reported<version_zwave_software_report_z_wave_protocol_build_number_t>(z_wave_protocol_build_number);

        version_zwave_software_report_application_version_t application_version = 0;
        application_version                                                     = get_value_or_default(attribute_map, "application_version", application_version);
        auto application_version_node                                           = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::application_version));
        application_version_node.set_reported<version_zwave_software_report_application_version_t>(application_version);

        version_zwave_software_report_application_build_number_t application_build_number = 0;
        application_build_number                                                          = get_value_or_default(attribute_map, "application_build_number", application_build_number);
        auto application_build_number_node                                                = group_node.emplace_node(static_cast<attribute_store_type_t>(version_zwave_software_report_group_attributes_t::application_build_number));
        application_build_number_node.set_reported<version_zwave_software_report_application_build_number_t>(application_build_number);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class