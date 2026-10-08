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

#include <string>
#include <string_view>
#include <vector>

#include "command_class_configuration_attribute_store.hpp"
#include "command_class_configuration_constants.hpp"
#include "command_class_configuration_types.hpp"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_configuration_attribute_store";

    using namespace command_class_configuration_types;
    using namespace command_class_configuration_constants;

    command_class_configuration_attribute_store::command_class_configuration_attribute_store()
    {
        const auto parameter_id = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID);
        register_attribute_types({
          {parameter_id, "Configuration Parameter ID", ATTRIBUTE_ENDPOINT_ID, U16_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size), "size", parameter_id, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format), "format", parameter_id, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::min_value), "min_value", parameter_id, I64_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::max_value), "max_value", parameter_id, I64_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value), "default_value", parameter_id, I64_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value), "value", parameter_id, I64_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::name), "name", parameter_id, C_STRING_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::info), "info", parameter_id, C_STRING_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only), "read_only", parameter_id, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::altering_capabilities), "altering_capabilities", parameter_id, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::advanced), "advanced", parameter_id, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support), "no_bulk_support", parameter_id, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_complete), "discovery_complete", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter), "default_resets_every_parameter", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::properties_walk_active), "properties_walk_active", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active), "parameter_scan_active", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_number), "parameter_scan_number", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_size_index), "parameter_scan_size_index", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase), "parameter_scan_phase", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining), "parameter_scan_reports_remaining", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first), "parameter_scan_probe_first", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second), "parameter_scan_probe_second", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::name_reports_to_follow), "name_reports_to_follow", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::info_reports_to_follow), "info_reports_to_follow", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
        });
    }

    attribute_store::attribute command_class_configuration_attribute_store::emplace_parameter(attribute_store::attribute endpoint_node, uint16_t parameter_number)
    {
        return endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), parameter_number);
    }

    uint8_t command_class_configuration_attribute_store::normalize_format(uint8_t raw_format_field)
    {
        return static_cast<uint8_t>((raw_format_field >> 3) & 0x07);
    }

    uint8_t command_class_configuration_attribute_store::normalize_flag(uint8_t raw_flag_field)
    {
        return raw_flag_field != 0 ? 1 : 0;
    }

    std::vector<uint8_t> command_class_configuration_attribute_store::encode_configuration_value(int64_t value, uint8_t size)
    {
        std::vector<uint8_t> bytes(size);
        uint64_t raw = static_cast<uint64_t>(value);
        for (uint8_t i = 0; i < size; ++i) {
            bytes[size - 1 - i] = static_cast<uint8_t>(raw & 0xFF);
            raw >>= 8;
        }
        return bytes;
    }

    int64_t command_class_configuration_attribute_store::decode_configuration_value(const std::vector<uint8_t> &bytes, uint8_t format)
    {
        if (bytes.empty()) {
            return 0;
        }

        uint64_t raw = 0;
        for (uint8_t byte: bytes) {
            raw = (raw << 8) | byte;
        }

        using value_format         = command_class_configuration_constants::format;
        const bool treat_as_signed = format == static_cast<uint8_t>(value_format::SIGNED_INTEGER) || (format != static_cast<uint8_t>(value_format::UNSIGNED_INTEGER) && format != static_cast<uint8_t>(value_format::ENUMERATED) && format != static_cast<uint8_t>(value_format::BIT_FIELD));
        if (treat_as_signed && (bytes[0] & 0x80) != 0 && bytes.size() < sizeof(int64_t)) {
            const uint8_t shift = static_cast<uint8_t>((sizeof(int64_t) - bytes.size()) * 8);
            return static_cast<int64_t>(raw << shift) >> shift;
        }

        return static_cast<int64_t>(raw);
    }

    sl_status_t command_class_configuration_attribute_store::on_configuration_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map)
    {
        configuration_report_parameter_number_t parameter_number = 0;
        parameter_number                                         = get_value_or_default(attribute_map, "parameter_number", parameter_number);
        uint8_t size                                             = 0;
        size                                                     = get_value_or_default(attribute_map, "size", size);
        configuration_report_configuration_value_t configuration_value;
        configuration_value = get_value_or_default(attribute_map, "configuration_value", configuration_value);

        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_report_group_attributes_t::CONFIGURATION_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_report_group_attributes_t::parameter_number)).set_reported(parameter_number);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_report_group_attributes_t::size)).set_reported(size);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_report_group_attributes_t::configuration_value)).set_reported(configuration_value);

        if (!is_valid_size(size) || configuration_value.size() != size) {
            return SL_STATUS_OK;
        }

        auto parameter_node = emplace_parameter(endpoint_node, parameter_number);
        auto size_node      = parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size));
        size_node.set_reported(size);

        uint8_t format_value = static_cast<uint8_t>(command_class_configuration_constants::format::SIGNED_INTEGER);
        auto format_node     = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format));
        if (format_node.is_valid() && format_node.reported_exists()) {
            format_value = format_node.reported<uint8_t>();
        } else {
            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format)).set_reported(format_value);
        }

        const int64_t value = decode_configuration_value(configuration_value, format_value);
        parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value)).set_reported(value);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_attribute_store::on_configuration_name_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map)
    {
        configuration_name_report_parameter_number_t parameter_number   = 0;
        parameter_number                                                = get_value_or_default(attribute_map, "parameter_number", parameter_number);
        configuration_name_report_reports_to_follow_t reports_to_follow = 0;
        reports_to_follow                                               = get_value_or_default(attribute_map, "reports_to_follow", reports_to_follow);
        configuration_name_report_name_t name_bytes;
        name_bytes = get_value_or_default(attribute_map, "name", name_bytes);

        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_name_report_group_attributes_t::CONFIGURATION_NAME_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_name_report_group_attributes_t::parameter_number)).set_reported(parameter_number);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_name_report_group_attributes_t::reports_to_follow)).set_reported(reports_to_follow);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_name_report_group_attributes_t::name)).set_reported(name_bytes);

        auto parameter_node = emplace_parameter(endpoint_node, parameter_number);
        auto name_node      = parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::name));

        auto previous_follow_node      = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::name_reports_to_follow));
        const uint8_t previous_follow  = previous_follow_node.reported_exists() ? previous_follow_node.reported<uint8_t>() : 0;
        const bool continuing_sequence = previous_follow > 0 && reports_to_follow < previous_follow;

        std::string name = continuing_sequence && name_node.desired_exists() ? name_node.desired<std::string>() : std::string {};
        name.append(reinterpret_cast<const char *>(name_bytes.data()), name_bytes.size());

        previous_follow_node.set_reported(reports_to_follow);

        if (reports_to_follow > 0) {
            name_node.set_desired(name);
        } else {
            name_node.set_reported(name);
            name_node.clear_desired();
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_attribute_store::on_configuration_info_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map)
    {
        configuration_info_report_parameter_number_t parameter_number   = 0;
        parameter_number                                                = get_value_or_default(attribute_map, "parameter_number", parameter_number);
        configuration_info_report_reports_to_follow_t reports_to_follow = 0;
        reports_to_follow                                               = get_value_or_default(attribute_map, "reports_to_follow", reports_to_follow);
        configuration_info_report_info_t info_bytes;
        info_bytes = get_value_or_default(attribute_map, "info", info_bytes);

        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_info_report_group_attributes_t::CONFIGURATION_INFO_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_info_report_group_attributes_t::parameter_number)).set_reported(parameter_number);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_info_report_group_attributes_t::reports_to_follow)).set_reported(reports_to_follow);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_info_report_group_attributes_t::info)).set_reported(info_bytes);

        auto parameter_node = emplace_parameter(endpoint_node, parameter_number);
        auto info_node      = parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::info));

        auto previous_follow_node      = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::info_reports_to_follow));
        const uint8_t previous_follow  = previous_follow_node.reported_exists() ? previous_follow_node.reported<uint8_t>() : 0;
        const bool continuing_sequence = previous_follow > 0 && reports_to_follow < previous_follow;

        std::string info = continuing_sequence && info_node.desired_exists() ? info_node.desired<std::string>() : std::string {};
        info.append(reinterpret_cast<const char *>(info_bytes.data()), info_bytes.size());

        previous_follow_node.set_reported(reports_to_follow);

        if (reports_to_follow > 0) {
            info_node.set_desired(info);
        } else {
            info_node.set_reported(info);
            info_node.clear_desired();
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_attribute_store::on_configuration_properties_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map)
    {
        configuration_properties_report_parameter_number_t parameter_number = 0;
        parameter_number                                                    = get_value_or_default(attribute_map, "parameter_number", parameter_number);
        uint8_t size                                                        = 0;
        size                                                                = get_value_or_default(attribute_map, "size", size);
        uint8_t format_raw                                                  = 0;
        format_raw                                                          = get_value_or_default(attribute_map, "format", format_raw);
        configuration_properties_report_min_value_t min_value;
        min_value = get_value_or_default(attribute_map, "min_value", min_value);
        configuration_properties_report_max_value_t max_value;
        max_value = get_value_or_default(attribute_map, "max_value", max_value);
        configuration_properties_report_default_value_t default_value;
        default_value                                                                 = get_value_or_default(attribute_map, "default_value", default_value);
        configuration_properties_report_next_parameter_number_t next_parameter_number = 0;
        next_parameter_number                                                         = get_value_or_default(attribute_map, "next_parameter_number", next_parameter_number);

        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::CONFIGURATION_PROPERTIES_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::parameter_number)).set_reported(parameter_number);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::size)).set_reported(size);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::format)).set_reported(normalize_format(format_raw));
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::min_value)).set_reported(min_value);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::max_value)).set_reported(max_value);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::default_value)).set_reported(default_value);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::next_parameter_number)).set_reported(next_parameter_number);

        if (size == 0) {
            return SL_STATUS_OK;
        }

        const uint8_t format_value = normalize_format(format_raw);
        auto parameter_node        = emplace_parameter(endpoint_node, parameter_number);
        parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size)).set_reported(size);
        parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format)).set_reported(format_value);
        parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::min_value)).set_reported(decode_configuration_value(min_value, format_value));
        parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::max_value)).set_reported(decode_configuration_value(max_value, format_value));
        parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported(decode_configuration_value(default_value, format_value));

        if (attribute_map.contains("readonly")) {
            uint8_t readonly = 0;
            readonly         = get_value_or_default(attribute_map, "readonly", readonly);
            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only)).set_reported(normalize_flag(readonly));
            report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::readonly)).set_reported(normalize_flag(readonly));
        }
        if (attribute_map.contains("altering_capabilities")) {
            uint8_t altering = 0;
            altering         = get_value_or_default(attribute_map, "altering_capabilities", altering);
            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::altering_capabilities)).set_reported(normalize_flag(altering));
            report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::altering_capabilities)).set_reported(normalize_flag(altering));
        }
        if (attribute_map.contains("advanced")) {
            uint8_t advanced = 0;
            advanced         = get_value_or_default(attribute_map, "advanced", advanced);
            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::advanced)).set_reported(normalize_flag(advanced));
            report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::advanced)).set_reported(normalize_flag(advanced));
        }
        if (attribute_map.contains("no_bulk_support")) {
            uint8_t no_bulk = 0;
            no_bulk         = get_value_or_default(attribute_map, "no_bulk_support", no_bulk);
            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support)).set_reported(normalize_flag(no_bulk));
            report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::no_bulk_support)).set_reported(normalize_flag(no_bulk));
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_attribute_store::on_configuration_bulk_report_received_store(attribute_store::attribute endpoint_node, command_class_configuration_attribute_map_t attribute_map)
    {
        configuration_bulk_report_parameter_offset_t parameter_offset         = 0;
        parameter_offset                                                      = get_value_or_default(attribute_map, "parameter_offset", parameter_offset);
        configuration_bulk_report_number_of_parameters_t number_of_parameters = 0;
        number_of_parameters                                                  = get_value_or_default(attribute_map, "number_of_parameters", number_of_parameters);
        uint8_t size                                                          = 0;
        size                                                                  = get_value_or_default(attribute_map, "size", size);
        configuration_bulk_report_vg_t vg;
        vg = get_value_or_default(attribute_map, "vg", vg);

        auto report_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::CONFIGURATION_BULK_REPORT_GROUP));
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::parameter_offset)).set_reported(parameter_offset);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::number_of_parameters)).set_reported(number_of_parameters);
        report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::size)).set_reported(size);
        if (attribute_map.contains("reports_to_follow")) {
            configuration_bulk_report_reports_to_follow_t reports_to_follow = 0;
            reports_to_follow                                               = get_value_or_default(attribute_map, "reports_to_follow", reports_to_follow);
            report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::reports_to_follow)).set_reported(reports_to_follow);
        }
        if (attribute_map.contains("handshake")) {
            uint8_t handshake = 0;
            handshake         = get_value_or_default(attribute_map, "handshake", handshake);
            report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::handshake)).set_reported(normalize_flag(handshake));
        }
        if (attribute_map.contains("default_flag")) {
            uint8_t default_flag = 0;
            default_flag         = get_value_or_default(attribute_map, "default_flag", default_flag);
            report_group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::default_flag)).set_reported(normalize_flag(default_flag));
        }

        if (!is_valid_size(size) || vg.size() != number_of_parameters) {
            return SL_STATUS_OK;
        }

        for (uint8_t i = 0; i < number_of_parameters; ++i) {
            if (vg[i].parameter.size() != size) {
                continue;
            }
            const uint16_t parameter_number = static_cast<uint16_t>(parameter_offset + i);
            auto parameter_node             = emplace_parameter(endpoint_node, parameter_number);
            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size)).set_reported(size);

            uint8_t format_value = static_cast<uint8_t>(command_class_configuration_constants::format::SIGNED_INTEGER);
            auto format_node     = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format));
            if (format_node.is_valid() && format_node.reported_exists()) {
                format_value = format_node.reported<uint8_t>();
            } else {
                parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format)).set_reported(format_value);
            }

            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value)).set_reported(decode_configuration_value(vg[i].parameter, format_value));
        }

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
