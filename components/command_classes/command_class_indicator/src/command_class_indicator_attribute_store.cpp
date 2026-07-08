
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

#include "command_class_indicator_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_indicator_attribute_store";

    command_class_indicator_attribute_store::command_class_indicator_attribute_store() {}

    sl_status_t command_class_indicator_attribute_store::on_indicator_report_received_store(attribute_store::attribute endpoint_node, command_class_indicator_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::INDICATOR_REPORT_GROUP));

        indicator_report_indicator_0_value_t indicator_0_value = 0;
        indicator_0_value                                      = get_value_or_default(attribute_map, "indicator_0_value", indicator_0_value);
        auto current_value_node                                = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::indicator_0_value));
        current_value_node.set_reported<indicator_report_indicator_0_value_t>(indicator_0_value);

        indicator_report_properties1_flags_t properties1    = {0, 0};
        properties1.indicator_report_indicator_object_count = get_value_or_default(attribute_map, "indicator_object_count", properties1.indicator_report_indicator_object_count);
        auto indicator_object_count_node                    = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::indicator_object_count));
        indicator_object_count_node.set_reported<indicator_report_indicator_0_value_t>(properties1.indicator_report_indicator_object_count);
        // Reserved is not used, default to 0

        indicator_report_vg1_t vg1 = {};
        vg1                        = get_value_or_default(attribute_map, "vg1", vg1);
        auto vg1_node              = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::vg1));
        vg1_node.set_reported<indicator_report_vg1_t>(vg1);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator_attribute_store::on_indicator_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_indicator_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_report_group_attributes_t::INDICATOR_SUPPORTED_REPORT_GROUP));

        indicator_supported_report_indicator_id_t indicator_id = 0;
        indicator_id                                           = get_value_or_default(attribute_map, "indicator_id", indicator_id);
        auto indicator_id_node                                 = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_report_group_attributes_t::indicator_id));
        indicator_id_node.set_reported<indicator_supported_report_indicator_id_t>(indicator_id);

        indicator_supported_report_next_indicator_id_t next_indicator_id = 0;
        next_indicator_id                                                = get_value_or_default(attribute_map, "next_indicator_id", next_indicator_id);
        auto next_indicator_id_node                                      = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_report_group_attributes_t::next_indicator_id));
        next_indicator_id_node.set_reported<indicator_supported_report_next_indicator_id_t>(next_indicator_id);

        indicator_supported_report_properties1_flags_t properties1                = {0, 0};
        properties1.indicator_supported_report_property_supported_bit_mask_length = get_value_or_default(attribute_map, "property_supported_bit_mask_length", properties1.indicator_supported_report_property_supported_bit_mask_length);
        auto property_supported_bit_mask_length_node                              = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_report_group_attributes_t::property_supported_bit_mask_length));
        property_supported_bit_mask_length_node.set_reported<indicator_supported_report_properties1_flags_t>(properties1);
        // Reserved is not used, default to 0

        indicator_supported_report_property_supported_bit_mask_t property_supported_bit_mask = {};
        property_supported_bit_mask                                                          = get_value_or_default(attribute_map, "property_supported_bit_mask", property_supported_bit_mask);
        auto property_supported_bit_mask_node                                                = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_report_group_attributes_t::property_supported_bit_mask));
        property_supported_bit_mask_node.set_reported<indicator_supported_report_property_supported_bit_mask_t>(property_supported_bit_mask);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator_attribute_store::on_indicator_description_report_received_store(attribute_store::attribute endpoint_node, command_class_indicator_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_description_report_group_attributes_t::INDICATOR_DESCRIPTION_REPORT_GROUP));

        indicator_description_report_indicator_id_t indicator_id = 0;
        indicator_id                                             = get_value_or_default(attribute_map, "indicator_id", indicator_id);
        auto indicator_id_node                                   = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_description_report_group_attributes_t::indicator_id));
        indicator_id_node.set_reported<indicator_description_report_indicator_id_t>(indicator_id);

        indicator_description_report_description_length_t description_length = 0;
        description_length                                                   = get_value_or_default(attribute_map, "description_length", description_length);
        auto description_length_node                                         = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_description_report_group_attributes_t::description_length));
        description_length_node.set_reported<indicator_description_report_description_length_t>(description_length);

        indicator_description_report_description_t description = {};
        description                                            = get_value_or_default(attribute_map, "description", description);
        auto description_node                                  = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_description_report_group_attributes_t::description));
        description_node.set_reported<indicator_description_report_description_t>(description);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class