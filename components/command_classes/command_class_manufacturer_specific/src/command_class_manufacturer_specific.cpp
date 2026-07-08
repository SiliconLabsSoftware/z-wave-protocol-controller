
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

#include <charconv>
#include <limits>
#include <string_view>

// Base class
#include "command_class_manufacturer_specific.hpp"

#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "zpc_attribute_store.h"
#include "zpc_attribute_store_network_helper.h"
#include "zpc_config.h"

#include "command_class_manufacturer_specific_constants.hpp"
#include "command_class_manufacturer_specific_events.hpp"
#include "command_class_manufacturer_specific_types.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_manufacturer_specific";

    namespace
    {
        std::vector<uint8_t> decode_hex_device_id(const char *hex_string)
        {
            if (hex_string == nullptr) {
                return {};
            }

            constexpr std::size_t hex_chars_per_byte = 2;
            constexpr int hex_base                   = 16;

            const std::string_view source(hex_string);
            const std::size_t byte_count = source.size() / hex_chars_per_byte;
            if ((byte_count * hex_chars_per_byte) != source.size()) {
                return {};
            }

            std::vector<uint8_t> bytes;
            bytes.reserve(byte_count);
            for (std::size_t source_offset = 0; source_offset < source.size(); source_offset += hex_chars_per_byte) {
                unsigned int parsed_value = 0;
                const char *parse_begin   = source.data() + source_offset;
                const char *parse_end     = parse_begin + hex_chars_per_byte;
                const auto [next, error]  = std::from_chars(parse_begin, parse_end, parsed_value, hex_base);
                if (error != std::errc {} || next != parse_end || parsed_value > std::numeric_limits<uint8_t>::max()) {
                    return {};
                }
                bytes.push_back(static_cast<uint8_t>(parsed_value));
            }

            return bytes;
        }

        uint8_t normalize_device_id_type(uint8_t requested_device_id_type)
        {
            switch (requested_device_id_type) {
                case DEVICE_SPECIFIC_GET_DEVICE_ID_TYPE_SERIAL_NUMBER_V2:
                case command_class_manufacturer_specific_constants::DEVICE_ID_TYPE_PSEUDO_RANDOM:
                    return requested_device_id_type;
                case DEVICE_SPECIFIC_GET_DEVICE_ID_TYPE_RESERVED_V2:
                default:
                    return DEVICE_SPECIFIC_REPORT_DEVICE_ID_TYPE_SERIAL_NUMBER_V2;
            }
        }

        std::vector<uint8_t> get_device_id_data(const zpc_config_t &config)
        {
            auto device_id_data = decode_hex_device_id(config.device_id);
            if (device_id_data.empty()) {
                device_id_data = {
                  static_cast<uint8_t>((config.manufacturer_id >> 8) & 0xFF),
                  static_cast<uint8_t>(config.manufacturer_id & 0xFF),
                  static_cast<uint8_t>((config.product_type >> 8) & 0xFF),
                  static_cast<uint8_t>(config.product_type & 0xFF),
                  static_cast<uint8_t>((config.product_id >> 8) & 0xFF),
                  static_cast<uint8_t>(config.product_id & 0xFF),
                };
            }

            if (device_id_data.size() > command_class_manufacturer_specific_constants::DEVICE_ID_DATA_MAX_LENGTH) {
                device_id_data.resize(command_class_manufacturer_specific_constants::DEVICE_ID_DATA_MAX_LENGTH);
            }

            return device_id_data;
        }
    }  // namespace

    command_class_manufacturer_specific::command_class_manufacturer_specific() {}

    void command_class_manufacturer_specific::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        auto group_node_manufacturer_specific = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(manufacturer_specific_get_group_attributes_t::MANUFACTURER_SPECIFIC_GET_GROUP));
        command_class_manufacturer_specific_core::start_group_resolution(group_node_manufacturer_specific);

        auto group_node_device_specific = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(device_specific_get_group_attributes_t::DEVICE_SPECIFIC_GET_GROUP));
        command_class_manufacturer_specific_core::start_group_resolution(group_node_device_specific);
    }

    sl_status_t command_class_manufacturer_specific::on_manufacturer_specific_get_support_requested_assemble_frame(const zwave_controller_connection_info_t * /*connection_info*/,
                                                                                                                   command_class_manufacturer_specific_attribute_map_t /*attribute_map*/,
                                                                                                                   zwave_frame_generator_standalone &report_frame,
                                                                                                                   std::vector<uint8_t> &frame)
    {
        const auto *config = zpc_get_config();
        if (config == nullptr) {
            return SL_STATUS_FAIL;
        }

        report_frame.add_raw_byte(static_cast<uint8_t>((config->manufacturer_id >> 8) & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>(config->manufacturer_id & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>((config->product_type >> 8) & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>(config->product_type & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>((config->product_id >> 8) & 0xFF));
        report_frame.add_raw_byte(static_cast<uint8_t>(config->product_id & 0xFF));

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_manufacturer_specific::on_device_specific_get_support_requested_assemble_frame(const zwave_controller_connection_info_t * /*connection_info*/,
                                                                                                             command_class_manufacturer_specific_attribute_map_t attribute_map,
                                                                                                             zwave_frame_generator_standalone &report_frame,
                                                                                                             std::vector<uint8_t> &frame)
    {
        const auto *config = zpc_get_config();
        if (config == nullptr) {
            return SL_STATUS_FAIL;
        }

        const uint8_t requested_device_id_type = get_value_or_default<uint8_t>(attribute_map, "device_id_type", DEVICE_SPECIFIC_GET_DEVICE_ID_TYPE_RESERVED_V2);
        const uint8_t device_id_type           = normalize_device_id_type(requested_device_id_type);

        const auto device_id_data = get_device_id_data(*config);
        if (device_id_data.empty()) {
            return SL_STATUS_FAIL;
        }

        const uint8_t properties1 = static_cast<uint8_t>(device_id_type & DEVICE_SPECIFIC_REPORT_PROPERTIES1_DEVICE_ID_TYPE_MASK_V2);
        uint8_t properties2       = static_cast<uint8_t>((DEVICE_SPECIFIC_REPORT_DEVICE_ID_DATA_FORMAT_BINARY_V2 << DEVICE_SPECIFIC_REPORT_PROPERTIES2_DEVICE_ID_DATA_FORMAT_SHIFT_V2) & DEVICE_SPECIFIC_REPORT_PROPERTIES2_DEVICE_ID_DATA_FORMAT_MASK_V2);
        properties2 |= static_cast<uint8_t>(device_id_data.size() & DEVICE_SPECIFIC_REPORT_PROPERTIES2_DEVICE_ID_DATA_LENGTH_INDICATOR_MASK_V2);

        report_frame.add_raw_byte(properties1);
        report_frame.add_raw_byte(properties2);
        for (const uint8_t b: device_id_data) {
            report_frame.add_raw_byte(b);
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
