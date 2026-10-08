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
#include <vector>

#include "command_class_meter.hpp"
#include "command_class_meter_constants.hpp"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_meter";

    command_class_meter::command_class_meter() = default;

    std::vector<uint8_t> command_class_meter::scales_from_supported_bitmask(uint8_t version, uint8_t scale_supported_0, uint8_t m_s_t, const std::vector<uint8_t> &scale_supported_follow)
    {
        using namespace command_class_meter_constants;

        std::vector<uint8_t> scales;
        const uint8_t byte1   = static_cast<uint8_t>(scale_supported_0 & SCALE_SUPPORTED_0_MASK);
        const uint8_t max_bit = (version <= 2) ? MAX_SCALE_V2 : MAX_SCALE_BYTE1;
        for (uint8_t bit = 0; bit <= max_bit; ++bit) {
            if ((byte1 & (1U << bit)) != 0U) {
                scales.push_back(bit);
            }
        }

        if (version >= 4 && has_more_scale_types(m_s_t)) {
            for (size_t byte_index = 0; byte_index < scale_supported_follow.size(); ++byte_index) {
                for (uint8_t bit = 0; bit < 8; ++bit) {
                    if ((scale_supported_follow[byte_index] & (1U << bit)) != 0U) {
                        scales.push_back(static_cast<uint8_t>(SCALE_BITS_EXTENDED + (byte_index * 8) + bit));
                    }
                }
            }
        }
        return scales;
    }

    std::vector<uint8_t> command_class_meter::rate_types_for_meter_get(uint8_t version, uint8_t supported_rate_type_masked)
    {
        using namespace command_class_meter_constants;

        if (version < 4) {
            return {RATE_TYPE_DEFAULT};
        }

        switch (rate_type_from_masked(supported_rate_type_masked)) {
            case SUPPORTED_RATE_TYPE_IMPORT_ONLY:
                return {RATE_TYPE_IMPORT};
            case SUPPORTED_RATE_TYPE_EXPORT_ONLY:
                return {RATE_TYPE_EXPORT};
            case SUPPORTED_RATE_TYPE_IMPORT_AND_EXPORT:
                return {RATE_TYPE_IMPORT, RATE_TYPE_EXPORT};
            default:
                return {};
        }
    }

    std::vector<std::pair<uint8_t, uint8_t>> command_class_meter::reading_requests_for_endpoint(attribute_store::attribute endpoint_node, uint8_t version)
    {
        std::vector<std::pair<uint8_t, uint8_t>> requests;
        if (version < 2) {
            requests.emplace_back(0, command_class_meter_constants::RATE_TYPE_DEFAULT);
            return requests;
        }

        uint8_t scale_supported_0 = 0;
        uint8_t m_s_t             = 0;
        std::vector<uint8_t> follow;
        if (!get_supported_scale_bitmask(endpoint_node, scale_supported_0, m_s_t, follow)) {
            return requests;
        }

        uint8_t rate_type_masked = 0;
        if (version >= 4) {
            if (!get_supported_rate_type_masked(endpoint_node, rate_type_masked)) {
                return requests;
            }
        }

        const auto scales     = scales_from_supported_bitmask(version, scale_supported_0, m_s_t, follow);
        const auto rate_types = rate_types_for_meter_get(version, rate_type_masked);
        for (uint8_t scale: scales) {
            for (uint8_t rate_type: rate_types) {
                requests.emplace_back(scale, rate_type);
            }
        }
        return requests;
    }

    bool command_class_meter::start_meter_get(attribute_store::attribute endpoint_node, uint8_t scale, uint8_t rate_type)
    {
        auto get_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
        get_group.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).set_desired<uint8_t>(scale);
        get_group.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).set_desired<uint8_t>(rate_type);
        start_group_resolution(get_group);
        return true;
    }

    bool command_class_meter::start_next_missing_meter_get(attribute_store::attribute endpoint_node, uint8_t version)
    {
        for (const auto &request: reading_requests_for_endpoint(endpoint_node, version)) {
            auto report_group = find_meter_report_group(endpoint_node, request.first, request.second);
            if (!report_group.is_valid()) {
                find_or_create_meter_report_group(endpoint_node, request.first, request.second);
                return start_meter_get(endpoint_node, request.first, request.second);
            }
            auto value_node = report_group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value));
            if (!value_node.is_valid() || !value_node.reported_exists()) {
                return start_meter_get(endpoint_node, request.first, request.second);
            }
        }
        return false;
    }

    void command_class_meter::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
        invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));

        if (supported_version < 2) {
            auto report = find_or_create_meter_report_group(endpoint_node, 0, command_class_meter_constants::RATE_TYPE_DEFAULT);
            cc_interview_require_attribute(report.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)));
            start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)));
            return;
        }

        auto supported = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
        cc_interview_require_attribute(supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_type)));
        cc_interview_require_attribute(supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_reset)));
        cc_interview_require_attribute(supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_supported_get_group_attributes_t::METER_SUPPORTED_GET_GROUP)));
    }

    sl_status_t command_class_meter::on_meter_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_meter_attribute_map_t payload)
    {
        (void)connection_info;
        (void)payload;

        const uint8_t version = endpoint_supported_version(endpoint);
        for (const auto &request: reading_requests_for_endpoint(endpoint, version)) {
            auto report_group = find_or_create_meter_report_group(endpoint, request.first, request.second);
            cc_interview_require_attribute(report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)));
        }

        start_next_missing_meter_get(endpoint, version);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_meter::on_meter_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_meter_attribute_map_t payload)
    {
        (void)connection_info;
        (void)payload;

        const uint8_t version = endpoint_supported_version(endpoint);
        if (version >= 2) {
            start_next_missing_meter_get(endpoint, version);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_meter::on_meter_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;

        auto group_node                          = args.node;
        const auto &frame_generator              = args.get_frame_generator;
        attribute_store::attribute endpoint_node = group_node.parent();
        const uint8_t version                    = endpoint_supported_version(endpoint_node);

        if (version < 2) {
            return frame_generator->generate_no_args_frame();
        }

        auto scale_node = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale));
        if (!scale_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        const uint8_t logical_scale = scale_node.desired<uint8_t>();

        uint8_t rate_type = command_class_meter_constants::RATE_TYPE_DEFAULT;
        if (version >= 4) {
            auto rate_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type));
            if (!rate_type_node.desired_exists()) {
                return SL_STATUS_NOT_READY;
            }
            rate_type = rate_type_node.desired<uint8_t>();
        }

        uint8_t scale_bits   = 0;
        uint8_t scale_2      = 0;
        bool include_scale_2 = false;
        command_class_meter_constants::write_meter_get_scale(logical_scale, version, scale_bits, scale_2, include_scale_2);

        if (version == 2 && scale_bits > command_class_meter_constants::MAX_SCALE_V2) {
            sl_log_warning(LOG_TAG.data(), "Meter Get v2 cannot request scale %u", logical_scale);
            return SL_STATUS_FAIL;
        }

        meter_get_properties1_t properties1;
        properties1.value                     = 0;
        properties1.flags.meter_get_scale     = scale_bits;
        properties1.flags.meter_get_rate_type = (version >= 4) ? rate_type : 0;
        frame_generator->add_raw_byte(properties1.value);

        if (include_scale_2) {
            frame_generator->add_raw_byte(scale_2);
        }

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_meter::on_meter_reset_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;

        auto group_node                          = args.node;
        const auto &frame_generator              = args.set_frame_generator;
        attribute_store::attribute endpoint_node = group_node.parent();
        const uint8_t version                    = endpoint_supported_version(endpoint_node);

        if (version < 2) {
            sl_log_warning(LOG_TAG.data(), "Meter Reset requires version 2 or newer");
            return SL_STATUS_FAIL;
        }

        if (version < 6) {
            return frame_generator->generate_no_args_frame();
        }

        auto meter_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::meter_type));
        auto rate_type_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::rate_type));
        auto size_node       = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::size));
        auto precision_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::precision));
        auto value_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::meter_value));
        auto scale_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::scale_bits_10));
        if (!meter_type_node.desired_exists() || !rate_type_node.desired_exists() || !size_node.desired_exists() || !precision_node.desired_exists() || !value_node.desired_exists() || !scale_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        const uint8_t logical_scale = scale_node.desired<uint8_t>();
        uint8_t scale_bits          = 0;
        uint8_t scale_2             = 0;
        bool include_scale_2        = false;
        command_class_meter_constants::write_meter_get_scale(logical_scale, version, scale_bits, scale_2, include_scale_2);

        meter_reset_properties1_t properties1;
        properties1.value                         = 0;
        properties1.flags.meter_reset_meter_type  = meter_type_node.desired<uint8_t>();
        properties1.flags.meter_reset_rate_type   = rate_type_node.desired<uint8_t>();
        properties1.flags.meter_reset_scale_bit_2 = static_cast<uint8_t>((scale_bits >> command_class_meter_constants::SCALE_HIGH_TO_SCALE_SHIFT) & command_class_meter_constants::SCALE_BIT_2_VALUE_MASK);
        frame_generator->add_raw_byte(properties1.value);

        meter_reset_properties2_t properties2;
        properties2.value                           = 0;
        properties2.flags.meter_reset_size          = size_node.desired<uint8_t>();
        properties2.flags.meter_reset_scale_bits_10 = static_cast<uint8_t>(scale_bits & command_class_meter_constants::SCALE_BITS_10_VALUE_MASK);
        properties2.flags.meter_reset_precision     = precision_node.desired<uint8_t>();
        frame_generator->add_raw_byte(properties2.value);

        const auto value_bytes = value_node.desired<std::vector<uint8_t>>();
        for (uint8_t byte: value_bytes) {
            frame_generator->add_raw_byte(byte);
        }
        frame_generator->add_raw_byte(scale_2);

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
