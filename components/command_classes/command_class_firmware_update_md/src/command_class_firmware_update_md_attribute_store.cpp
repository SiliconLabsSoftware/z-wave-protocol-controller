
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
#include "command_class_firmware_update_md.hpp"

#include "command_class_firmware_update_md_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_firmware_update_md_attribute_store";

    command_class_firmware_update_md_attribute_store::command_class_firmware_update_md_attribute_store() {}

    // FIRMWARE_MD_REPORT: manufacturer_id, firmware_0_id, firmware_0_checksum,
    //                     firmware_upgradable, number_of_firmware_targets, max_fragment_size,
    //                     hardware_version
    sl_status_t command_class_firmware_update_md_attribute_store::on_firmware_md_report_received_store(attribute_store::attribute endpoint_node, command_class_firmware_update_md_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::FIRMWARE_MD_REPORT_GROUP));

        firmware_md_report_manufacturer_id_t manufacturer_id = 0;
        manufacturer_id                                      = get_value_or_default(attribute_map, "manufacturer_id", manufacturer_id);
        auto manufacturer_id_node                            = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::manufacturer_id));
        manufacturer_id_node.set_reported<firmware_md_report_manufacturer_id_t>(manufacturer_id);

        firmware_md_report_firmware_0_id_t firmware_0_id = 0;
        firmware_0_id                                    = get_value_or_default(attribute_map, "firmware_0_id", firmware_0_id);
        auto firmware_0_id_node                          = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::firmware_0_id));
        firmware_0_id_node.set_reported<firmware_md_report_firmware_0_id_t>(firmware_0_id);

        firmware_md_report_firmware_0_checksum_t firmware_0_checksum = 0;
        firmware_0_checksum                                          = get_value_or_default(attribute_map, "firmware_0_checksum", firmware_0_checksum);
        auto firmware_0_checksum_node                                = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::firmware_0_checksum));
        firmware_0_checksum_node.set_reported<firmware_md_report_firmware_0_checksum_t>(firmware_0_checksum);

        firmware_md_report_firmware_upgradable_t firmware_upgradable = 0;
        firmware_upgradable                                          = get_value_or_default(attribute_map, "firmware_upgradable", firmware_upgradable);
        auto firmware_upgradable_node                                = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::firmware_upgradable));
        firmware_upgradable_node.set_reported<firmware_md_report_firmware_upgradable_t>(firmware_upgradable);

        firmware_md_report_number_of_firmware_targets_t number_of_firmware_targets = 0;
        number_of_firmware_targets                                                 = get_value_or_default(attribute_map, "number_of_firmware_targets", number_of_firmware_targets);
        auto number_of_firmware_targets_node                                       = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::number_of_firmware_targets));
        number_of_firmware_targets_node.set_reported<firmware_md_report_number_of_firmware_targets_t>(number_of_firmware_targets);

        firmware_md_report_max_fragment_size_t max_fragment_size = 0;
        max_fragment_size                                        = get_value_or_default(attribute_map, "max_fragment_size", max_fragment_size);
        auto max_fragment_size_node                              = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::max_fragment_size));
        max_fragment_size_node.set_reported<firmware_md_report_max_fragment_size_t>(max_fragment_size);

        firmware_md_report_hardware_version_t hardware_version = 0;
        hardware_version                                       = get_value_or_default(attribute_map, "hardware_version", hardware_version);
        auto hardware_version_node                             = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_report_group_attributes_t::hardware_version));
        hardware_version_node.set_reported<firmware_md_report_hardware_version_t>(hardware_version);

        return SL_STATUS_OK;
    }

    // FIRMWARE_UPDATE_MD_GET: number_of_reports, report_number_1, zero, report_number_2
    sl_status_t command_class_firmware_update_md_attribute_store::on_firmware_update_md_get_received_store(attribute_store::attribute endpoint_node, command_class_firmware_update_md_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::FIRMWARE_UPDATE_MD_GET_GROUP));

        firmware_update_md_get_number_of_reports_t number_of_reports = 0;
        number_of_reports                                            = get_value_or_default(attribute_map, "number_of_reports", number_of_reports);
        auto number_of_reports_node                                  = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::number_of_reports));
        number_of_reports_node.set_reported<firmware_update_md_get_number_of_reports_t>(number_of_reports);

        uint8_t report_number_1   = 0;
        report_number_1           = get_value_or_default(attribute_map, "report_number_1", report_number_1);
        auto report_number_1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::report_number_1));
        report_number_1_node.set_reported<uint8_t>(report_number_1);

        uint8_t zero   = 0;
        zero           = get_value_or_default(attribute_map, "zero", zero);
        auto zero_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::zero));
        zero_node.set_reported<uint8_t>(zero);

        firmware_update_md_get_report_number_2_t report_number_2 = 0;
        report_number_2                                          = get_value_or_default(attribute_map, "report_number_2", report_number_2);
        auto report_number_2_node                                = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::report_number_2));
        report_number_2_node.set_reported<firmware_update_md_get_report_number_2_t>(report_number_2);

        return SL_STATUS_OK;
    }

    // FIRMWARE_UPDATE_MD_REQUEST_REPORT: status
    sl_status_t command_class_firmware_update_md_attribute_store::on_firmware_update_md_request_report_received_store(attribute_store::attribute endpoint_node, command_class_firmware_update_md_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_report_group_attributes_t::FIRMWARE_UPDATE_MD_REQUEST_REPORT_GROUP));

        firmware_update_md_request_report_status_t status = 0;
        status                                            = get_value_or_default(attribute_map, "status", status);
        auto status_node                                  = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_report_group_attributes_t::status));
        status_node.set_reported<firmware_update_md_request_report_status_t>(status);

        return SL_STATUS_OK;
    }

    // FIRMWARE_UPDATE_MD_STATUS_REPORT: status, waittime
    sl_status_t command_class_firmware_update_md_attribute_store::on_firmware_update_md_status_report_received_store(attribute_store::attribute endpoint_node, command_class_firmware_update_md_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_status_report_group_attributes_t::FIRMWARE_UPDATE_MD_STATUS_REPORT_GROUP));

        firmware_update_md_status_report_status_t status = 0;
        status                                           = get_value_or_default(attribute_map, "status", status);
        auto status_node                                 = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_status_report_group_attributes_t::status));
        status_node.set_reported<firmware_update_md_status_report_status_t>(status);

        firmware_update_md_status_report_waittime_t waittime = 0;
        waittime                                             = get_value_or_default(attribute_map, "waittime", waittime);
        auto waittime_node                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_status_report_group_attributes_t::waittime));
        waittime_node.set_reported<firmware_update_md_status_report_waittime_t>(waittime);

        return SL_STATUS_OK;
    }

    // FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT: manufacturer_id, firmware_id, checksum,
    //                                           firmware_target, firmware_update_status,
    //                                           hardware_version
    sl_status_t command_class_firmware_update_md_attribute_store::on_firmware_update_activation_status_report_received_store(attribute_store::attribute endpoint_node, command_class_firmware_update_md_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_status_report_group_attributes_t::FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_GROUP));

        firmware_update_activation_status_report_manufacturer_id_t manufacturer_id = 0;
        manufacturer_id                                                            = get_value_or_default(attribute_map, "manufacturer_id", manufacturer_id);
        auto manufacturer_id_node                                                  = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_status_report_group_attributes_t::manufacturer_id));
        manufacturer_id_node.set_reported<firmware_update_activation_status_report_manufacturer_id_t>(manufacturer_id);

        firmware_update_activation_status_report_firmware_id_t firmware_id = 0;
        firmware_id                                                        = get_value_or_default(attribute_map, "firmware_id", firmware_id);
        auto firmware_id_node                                              = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_status_report_group_attributes_t::firmware_id));
        firmware_id_node.set_reported<firmware_update_activation_status_report_firmware_id_t>(firmware_id);

        firmware_update_activation_status_report_checksum_t checksum = 0;
        checksum                                                     = get_value_or_default(attribute_map, "checksum", checksum);
        auto checksum_node                                           = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_status_report_group_attributes_t::checksum));
        checksum_node.set_reported<firmware_update_activation_status_report_checksum_t>(checksum);

        firmware_update_activation_status_report_firmware_target_t firmware_target = 0;
        firmware_target                                                            = get_value_or_default(attribute_map, "firmware_target", firmware_target);
        auto firmware_target_node                                                  = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_status_report_group_attributes_t::firmware_target));
        firmware_target_node.set_reported<firmware_update_activation_status_report_firmware_target_t>(firmware_target);

        firmware_update_activation_status_report_firmware_update_status_t firmware_update_status = 0;
        firmware_update_status                                                                   = get_value_or_default(attribute_map, "firmware_update_status", firmware_update_status);
        auto firmware_update_status_node                                                         = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_status_report_group_attributes_t::firmware_update_status));
        firmware_update_status_node.set_reported<firmware_update_activation_status_report_firmware_update_status_t>(firmware_update_status);

        firmware_update_activation_status_report_hardware_version_t hardware_version = 0;
        hardware_version                                                             = get_value_or_default(attribute_map, "hardware_version", hardware_version);
        auto hardware_version_node                                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_status_report_group_attributes_t::hardware_version));
        hardware_version_node.set_reported<firmware_update_activation_status_report_hardware_version_t>(hardware_version);

        return SL_STATUS_OK;
    }

    // FIRMWARE_UPDATE_MD_PREPARE_REPORT: status, firmware_checksum
    sl_status_t command_class_firmware_update_md_attribute_store::on_firmware_update_md_prepare_report_received_store(attribute_store::attribute endpoint_node, command_class_firmware_update_md_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_report_group_attributes_t::FIRMWARE_UPDATE_MD_PREPARE_REPORT_GROUP));

        firmware_update_md_prepare_report_status_t status = 0;
        status                                            = get_value_or_default(attribute_map, "status", status);
        auto status_node                                  = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_report_group_attributes_t::status));
        status_node.set_reported<firmware_update_md_prepare_report_status_t>(status);

        firmware_update_md_prepare_report_firmware_checksum_t firmware_checksum = 0;
        firmware_checksum                                                       = get_value_or_default(attribute_map, "firmware_checksum", firmware_checksum);
        auto firmware_checksum_node                                             = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_report_group_attributes_t::firmware_checksum));
        firmware_checksum_node.set_reported<firmware_update_md_prepare_report_firmware_checksum_t>(firmware_checksum);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
