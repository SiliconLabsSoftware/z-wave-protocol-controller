
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
#include "command_class_thermostat_setpoint.hpp"
#include "command_class_thermostat_setpoint_constants.hpp"

// Component connector
#include "component_connector.hpp"
#include "command_class_thermostat_mode_events.hpp"
#include "command_class_thermostat_mode_types.hpp"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_setpoint";

    int32_t command_class_thermostat_setpoint::decode_signed_setpoint_value(const std::vector<uint8_t> &bytes, uint8_t size)
    {
        using command_class_thermostat_setpoint_constants::SetpointValueSize;

        // Z-Wave spec: Value field is big-endian signed two's complement (Table 2.12).
        // bytes[0] is the most significant byte.
        if (bytes.size() < size) {
            return 0;
        }

        switch (static_cast<SetpointValueSize>(size)) {
            case SetpointValueSize::OneByte: {
                // Single byte: interpret as int8_t, then widen to int32_t.
                return static_cast<int32_t>(static_cast<int8_t>(bytes[0]));
            }
            case SetpointValueSize::TwoBytes: {
                // Big-endian 16-bit: high byte first, then reinterpret as signed.
                const uint16_t raw = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
                return static_cast<int32_t>(static_cast<int16_t>(raw));
            }
            case SetpointValueSize::FourBytes: {
                // Big-endian 32-bit: MSB first, then reinterpret as signed.
                const uint32_t raw = (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) | (static_cast<uint32_t>(bytes[2]) << 8) | static_cast<uint32_t>(bytes[3]);
                return static_cast<int32_t>(raw);
            }
            default:
                return 0;
        }
    }

    command_class_thermostat_setpoint::command_class_thermostat_setpoint()
    {
        component_connector connector;
        connector.connect_typed<command_class_thermostat_mode_events_t, command_class_thermostat_mode_types::thermostat_mode_changed_payload_t>(command_class_thermostat_mode_events_t::THERMOSTAT_MODE_CHANGED,
                                                                                                                                                [](const command_class_thermostat_mode_types::thermostat_mode_changed_payload_t &p) { return on_thermostat_mode_changed(p); });
    }

    sl_status_t command_class_thermostat_setpoint::on_thermostat_mode_changed(const command_class_thermostat_mode_types::thermostat_mode_changed_payload_t &payload)
    {
        sl_log_debug(LOG_TAG.data(), "Thermostat mode changed to %u, refreshing setpoints", payload.mode);
        auto endpoint_node      = payload.endpoint_node;
        auto group_node         = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
        auto setpoint_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type));
        setpoint_type_node.set_desired<uint8_t>(1);
        start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    void command_class_thermostat_setpoint::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        auto supported_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_supported_get_group_attributes_t::THERMOSTAT_SETPOINT_SUPPORTED_GET_GROUP));
        start_group_resolution(supported_get_node);
    }

    sl_status_t command_class_thermostat_setpoint::on_thermostat_setpoint_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_setpoint_attribute_map_t payload)
    {
        const uint8_t supported_version = endpoint_supported_version(endpoint);
        if (supported_version >= 1 && supported_version <= 2) {
            auto get_group_node     = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
            auto setpoint_type_node = get_group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type));
            setpoint_type_node.set_desired<uint8_t>(1);
            start_group_resolution(get_group_node);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint::on_thermostat_setpoint_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_setpoint_attribute_map_t payload)
    {
        const uint8_t supported_version = endpoint_supported_version(endpoint);

        if (supported_version >= 1 && supported_version <= 2) {
            auto get_group_node     = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::THERMOSTAT_SETPOINT_GET_GROUP));
            auto setpoint_type_node = get_group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type));
            if (setpoint_type_node.desired_exists()) {
                const uint8_t asked_type = setpoint_type_node.desired<uint8_t>();
                if (asked_type >= 1 && asked_type <= 14) {
                    setpoint_type_node.set_desired<uint8_t>(asked_type + 1);
                    start_group_resolution(get_group_node);
                    return SL_STATUS_OK;
                }
            }
        }

        if (supported_version >= 3) {
            uint8_t setpoint_type = 0;
            setpoint_type         = get_value_or_default(payload, "setpoint_type", setpoint_type);

            auto cap_get_node  = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::THERMOSTAT_SETPOINT_CAPABILITIES_GET_GROUP));
            auto cap_type_node = cap_get_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type));
            cap_type_node.set_desired<uint8_t>(setpoint_type);
            start_group_resolution(cap_get_node);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_setpoint::on_thermostat_setpoint_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto setpoint_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_get_group_attributes_t::setpoint_type));
        if (!setpoint_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        thermostat_setpoint_get_level_t level;
        level.value                                       = 0;
        level.flags.thermostat_setpoint_get_setpoint_type = setpoint_type_node.desired<uint8_t>();
        frame_generator->add_raw_byte(level.value);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_thermostat_setpoint::on_thermostat_setpoint_capabilities_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto setpoint_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_capabilities_get_group_attributes_t::setpoint_type));
        if (!setpoint_type_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        thermostat_setpoint_capabilities_get_properties1_t properties1;
        properties1.value                                                    = 0;
        properties1.flags.thermostat_setpoint_capabilities_get_setpoint_type = setpoint_type_node.desired<uint8_t>();
        frame_generator->add_raw_byte(properties1.value);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_thermostat_setpoint::on_thermostat_setpoint_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto setpoint_type_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::setpoint_type));
        auto size_node          = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::size));
        auto scale_node         = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::scale));
        auto precision_node     = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::precision));
        auto value_node         = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_setpoint_set_group_attributes_t::value));
        if (!setpoint_type_node.desired_exists() || !size_node.desired_exists() || !scale_node.desired_exists() || !precision_node.desired_exists() || !value_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        const uint8_t setpoint_type                    = setpoint_type_node.desired<uint8_t>();
        uint8_t scale                                  = scale_node.desired<uint8_t>();
        const attribute_store::attribute endpoint_node = group_node.parent();
        if (endpoint_node.is_valid()) {
            uint8_t reported_scale = 0;
            if (get_reported_scale_for_setpoint_type(endpoint_node, setpoint_type, reported_scale)) {
                scale = reported_scale;
            }
        }

        thermostat_setpoint_set_level_t level;
        level.value                                       = 0;
        level.flags.thermostat_setpoint_set_setpoint_type = setpoint_type;
        frame_generator->add_raw_byte(level.value);

        const uint8_t size = size_node.desired<uint8_t>();
        if (!command_class_thermostat_setpoint_constants::is_valid_size(size)) {
            sl_log_warning(LOG_TAG.data(), "Invalid Size %u; must be 1, 2 or 4", size);
            return SL_STATUS_FAIL;
        }
        if (!command_class_thermostat_setpoint_constants::is_valid_scale(scale)) {
            sl_log_warning(LOG_TAG.data(), "Invalid Scale %u; must be 0 (Celsius) or 1 (Fahrenheit)", scale);
            return SL_STATUS_FAIL;
        }

        const uint8_t supported_version = endpoint_supported_version(endpoint_node);
        if (supported_version >= 3) {
            std::vector<uint8_t> min_val;
            std::vector<uint8_t> max_val;
            if (get_reported_capabilities_for_setpoint_type(endpoint_node, setpoint_type, min_val, max_val)) {
                auto value_bytes = value_node.desired<std::vector<uint8_t>>();
                if (value_bytes.size() == size && min_val.size() >= size && max_val.size() >= size) {
                    const int32_t value_signed = decode_signed_setpoint_value(value_bytes, size);
                    const int32_t min_signed   = decode_signed_setpoint_value(min_val, size);
                    const int32_t max_signed   = decode_signed_setpoint_value(max_val, size);
                    if (value_signed < min_signed || value_signed > max_signed) {
                        sl_log_warning(LOG_TAG.data(), "Set value %d outside capabilities range [%d, %d] for setpoint type %u", value_signed, min_signed, max_signed, setpoint_type);
                        return SL_STATUS_FAIL;
                    }
                }
            }
        }

        thermostat_setpoint_set_level2_t level2;
        level2.value                                   = 0;
        level2.flags.thermostat_setpoint_set_size      = size;
        level2.flags.thermostat_setpoint_set_scale     = scale;
        level2.flags.thermostat_setpoint_set_precision = precision_node.desired<uint8_t>();
        frame_generator->add_raw_byte(level2.value);

        auto value_bytes = value_node.desired<std::vector<uint8_t>>();
        for (auto byte: value_bytes) {
            frame_generator->add_raw_byte(byte);
        }

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class