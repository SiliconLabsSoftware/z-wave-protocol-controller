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

#include "command_class_meter_attribute_store.hpp"
#include "command_class_meter_constants.hpp"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_meter_attribute_store";

    command_class_meter_attribute_store::command_class_meter_attribute_store() {}

    attribute_store::attribute command_class_meter_attribute_store::find_meter_report_group(attribute_store::attribute endpoint_node, uint8_t scale, uint8_t rate_type)
    {
        const auto report_type     = static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP);
        const auto scale_bit_2_t   = static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bit_2);
        const auto scale_bits_10_t = static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bits_10);
        const auto scale_2_t       = static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_2);
        const auto rate_type_t     = static_cast<attribute_store_type_t>(meter_report_group_attributes_t::rate_type);

        for (auto group_node: endpoint_node.children(report_type)) {
            auto scale_bit_2_node   = group_node.child_by_type(scale_bit_2_t);
            auto scale_bits_10_node = group_node.child_by_type(scale_bits_10_t);
            auto scale_2_node       = group_node.child_by_type(scale_2_t);
            auto rate_type_node     = group_node.child_by_type(rate_type_t);
            if (!scale_bit_2_node.is_valid() || !scale_bits_10_node.is_valid() || !scale_2_node.is_valid() || !rate_type_node.is_valid()) {
                continue;
            }
            if (!scale_bit_2_node.reported_exists() || !scale_bits_10_node.reported_exists() || !scale_2_node.reported_exists() || !rate_type_node.reported_exists()) {
                continue;
            }

            const uint8_t group_scale = command_class_meter_constants::meter_scale_from_report_fields(scale_bit_2_node.reported<uint8_t>(), scale_bits_10_node.reported<uint8_t>(), scale_2_node.reported<uint8_t>());
            const uint8_t group_rate  = command_class_meter_constants::rate_type_from_masked(rate_type_node.reported<uint8_t>());
            if (group_scale == scale && group_rate == rate_type) {
                return group_node;
            }
        }
        return attribute_store::attribute(ATTRIBUTE_STORE_INVALID_NODE);
    }

    attribute_store::attribute command_class_meter_attribute_store::find_or_create_meter_report_group(attribute_store::attribute endpoint_node, uint8_t scale, uint8_t rate_type)
    {
        auto report_group = find_meter_report_group(endpoint_node, scale, rate_type);
        if (report_group.is_valid()) {
            return report_group;
        }

        uint8_t scale_bit_2   = 0;
        uint8_t scale_bits_10 = 0;
        uint8_t scale_2       = 0;
        command_class_meter_constants::encode_scale_key_fields(scale, scale_bit_2, scale_bits_10, scale_2);

        report_group = endpoint_node.add_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bit_2)).set_reported<uint8_t>(scale_bit_2);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bits_10)).set_reported<uint8_t>(scale_bits_10);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_2)).set_reported<uint8_t>(scale_2);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::rate_type)).set_reported<uint8_t>(command_class_meter_constants::rate_type_to_masked(rate_type));
        return report_group;
    }

    bool command_class_meter_attribute_store::get_reported_meter_reset_supported(attribute_store::attribute endpoint_node)
    {
        auto supported = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
        if (!supported.is_valid()) {
            return false;
        }
        auto reset_node = supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_reset));
        if (!reset_node.is_valid() || !reset_node.reported_exists()) {
            return false;
        }
        return command_class_meter_constants::is_meter_reset_supported(reset_node.reported<uint8_t>());
    }

    bool command_class_meter_attribute_store::get_supported_scale_bitmask(attribute_store::attribute endpoint_node, uint8_t &scale_supported_0, uint8_t &m_s_t, std::vector<uint8_t> &scale_supported)
    {
        auto supported = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
        if (!supported.is_valid()) {
            return false;
        }
        auto scale_0_node = supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0));
        if (!scale_0_node.is_valid() || !scale_0_node.reported_exists()) {
            return false;
        }
        scale_supported_0 = scale_0_node.reported<uint8_t>();

        m_s_t           = 0;
        auto m_s_t_node = supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t));
        if (m_s_t_node.is_valid() && m_s_t_node.reported_exists()) {
            m_s_t = m_s_t_node.reported<uint8_t>();
        }

        scale_supported.clear();
        auto follow_node = supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported));
        if (follow_node.is_valid() && follow_node.reported_exists()) {
            scale_supported = follow_node.reported<std::vector<uint8_t>>();
        }
        return true;
    }

    bool command_class_meter_attribute_store::get_supported_rate_type_masked(attribute_store::attribute endpoint_node, uint8_t &rate_type_masked)
    {
        auto supported = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
        if (!supported.is_valid()) {
            return false;
        }
        auto rate_node = supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type));
        if (!rate_node.is_valid() || !rate_node.reported_exists()) {
            return false;
        }
        rate_type_masked = rate_node.reported<uint8_t>();
        return true;
    }

    sl_status_t command_class_meter_attribute_store::on_meter_supported_report_received_store(attribute_store::attribute endpoint_node, command_class_meter_attribute_map_t attribute_map)
    {
        uint8_t meter_type          = 0;
        uint8_t rate_type           = 0;
        uint8_t meter_reset         = 0;
        uint8_t scale_supported_0   = 0;
        uint8_t m_s_t               = 0;
        uint8_t follow_count        = 0;
        std::vector<uint8_t> scales = {};

        meter_type        = get_value_or_default(attribute_map, "meter_type", meter_type);
        rate_type         = get_value_or_default(attribute_map, "rate_type", rate_type);
        meter_reset       = get_value_or_default(attribute_map, "meter_reset", meter_reset);
        scale_supported_0 = get_value_or_default(attribute_map, "scale_supported_0", scale_supported_0);
        m_s_t             = get_value_or_default(attribute_map, "m_s_t", m_s_t);
        follow_count      = get_value_or_default(attribute_map, "number_of_scale_supported_bytes_to_follow", follow_count);
        scales            = get_value_or_default(attribute_map, "scale_supported", scales);

        auto report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
        report.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_type)).set_reported(meter_type);
        report.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type)).set_reported(rate_type);
        report.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_reset)).set_reported(meter_reset);
        report.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)).set_reported(scale_supported_0);
        report.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t)).set_reported(m_s_t);
        report.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::number_of_scale_supported_bytes_to_follow)).set_reported(follow_count);
        report.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).set_reported(scales);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_meter_attribute_store::on_meter_report_received_store(attribute_store::attribute endpoint_node, command_class_meter_attribute_map_t attribute_map)
    {
        uint8_t meter_type    = 0;
        uint8_t rate_type     = 0;
        uint8_t scale_bit_2   = 0;
        uint8_t size          = 0;
        uint8_t scale_bits_10 = 0;
        uint8_t precision     = 0;
        uint8_t scale_2       = 0;
        uint16_t delta_time   = 0;
        std::vector<uint8_t> meter_value;
        std::vector<uint8_t> previous_meter_value;

        meter_type           = get_value_or_default(attribute_map, "meter_type", meter_type);
        rate_type            = get_value_or_default(attribute_map, "rate_type", rate_type);
        scale_bit_2          = get_value_or_default(attribute_map, "scale_bit_2", scale_bit_2);
        size                 = get_value_or_default(attribute_map, "size", size);
        scale_bits_10        = get_value_or_default(attribute_map, "scale_bits_10", scale_bits_10);
        precision            = get_value_or_default(attribute_map, "precision", precision);
        scale_2              = get_value_or_default(attribute_map, "scale_2", scale_2);
        delta_time           = get_value_or_default(attribute_map, "delta_time", delta_time);
        meter_value          = get_value_or_default(attribute_map, "meter_value", meter_value);
        previous_meter_value = get_value_or_default(attribute_map, "previous_meter_value", previous_meter_value);

        const uint8_t logical_scale = command_class_meter_constants::meter_scale_from_report_fields(scale_bit_2, scale_bits_10, scale_2);
        const uint8_t logical_rate  = command_class_meter_constants::rate_type_from_masked(rate_type);

        auto report_group = find_or_create_meter_report_group(endpoint_node, logical_scale, logical_rate);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_type)).set_reported(meter_type);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::rate_type)).set_reported(rate_type);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bit_2)).set_reported(scale_bit_2);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::size)).set_reported(size);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bits_10)).set_reported(scale_bits_10);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::precision)).set_reported(precision);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).set_reported(meter_value);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::delta_time)).set_reported(delta_time);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::previous_meter_value)).set_reported(previous_meter_value);
        report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_2)).set_reported(scale_2);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
