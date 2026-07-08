
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
#include "command_class_manufacturer_specific.hpp"

#include "command_class_manufacturer_specific_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_manufacturer_specific_attribute_store";

    command_class_manufacturer_specific_attribute_store::command_class_manufacturer_specific_attribute_store() {}

    sl_status_t command_class_manufacturer_specific_attribute_store::on_manufacturer_specific_report_received_store(attribute_store::attribute endpoint_node, command_class_manufacturer_specific_attribute_map_t attribute_map)
    {
        manufacturer_specific_report_manufacturer_id_t manufacturer_id = 0;
        manufacturer_id                                                = get_value_or_default(attribute_map, "manufacturer_id", manufacturer_id);
        auto parent_node                                               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(manufacturer_specific_report_group_attributes_t::MANUFACTURER_SPECIFIC_REPORT_GROUP));
        auto manufacturer_id_node                                      = parent_node.emplace_node(static_cast<attribute_store_type_t>(manufacturer_specific_report_group_attributes_t::manufacturer_id));
        manufacturer_id_node.set_reported<manufacturer_specific_report_manufacturer_id_t>(manufacturer_id);

        manufacturer_specific_report_product_type_id_t product_type_id = 0;
        product_type_id                                                = get_value_or_default(attribute_map, "product_type_id", product_type_id);
        auto product_type_id_node                                      = parent_node.emplace_node(static_cast<attribute_store_type_t>(manufacturer_specific_report_group_attributes_t::product_type_id));
        product_type_id_node.set_reported<manufacturer_specific_report_product_type_id_t>(product_type_id);

        manufacturer_specific_report_product_id_t product_id = 0;
        product_id                                           = get_value_or_default(attribute_map, "product_id", product_id);
        auto product_id_node                                 = parent_node.emplace_node(static_cast<attribute_store_type_t>(manufacturer_specific_report_group_attributes_t::product_id));
        product_id_node.set_reported<manufacturer_specific_report_product_id_t>(product_id);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_manufacturer_specific_attribute_store::on_device_specific_report_received_store(attribute_store::attribute endpoint_node, command_class_manufacturer_specific_attribute_map_t attribute_map)
    {

        uint8_t device_id_type   = 0;
        device_id_type           = get_value_or_default(attribute_map, "device_id_type", device_id_type);
        auto parent_node         = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_report_group_attributes_t::DEVICE_SPECIFIC_REPORT_GROUP));
        auto device_id_type_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_report_group_attributes_t::device_id_type));
        device_id_type_node.set_reported<uint8_t>(device_id_type);

        uint8_t device_id_data_length_indicator   = 0;
        device_id_data_length_indicator           = get_value_or_default(attribute_map, "device_id_data_length_indicator", device_id_data_length_indicator);
        auto device_id_data_length_indicator_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_report_group_attributes_t::device_id_data_length_indicator));
        device_id_data_length_indicator_node.set_reported<uint8_t>(device_id_data_length_indicator);

        uint8_t device_id_data_format   = 0;
        device_id_data_format           = get_value_or_default(attribute_map, "device_id_data_format", device_id_data_format);
        auto device_id_data_format_node = parent_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_report_group_attributes_t::device_id_data_format));
        device_id_data_format_node.set_reported<uint8_t>(device_id_data_format);

        device_specific_report_device_id_data_t device_id_data = {};
        device_id_data                                         = get_value_or_default(attribute_map, "device_id_data", device_id_data);
        auto device_id_data_node                               = parent_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_report_group_attributes_t::device_id_data));
        device_id_data_node.set_reported<device_specific_report_device_id_data_t>(device_id_data);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class