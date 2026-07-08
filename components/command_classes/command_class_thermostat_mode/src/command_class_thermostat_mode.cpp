
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
#include "command_class_thermostat_mode.hpp"

// Component connector
#include "component_connector.hpp"
#include "command_class_thermostat_mode_events.hpp"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_mode";

    command_class_thermostat_mode::command_class_thermostat_mode() {}

    void command_class_thermostat_mode::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        auto supported_get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_supported_get_group_attributes_t::THERMOSTAT_MODE_SUPPORTED_GET_GROUP));
        start_group_resolution(supported_get_node);

        auto get_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_get_group_attributes_t::THERMOSTAT_MODE_GET_GROUP));
        start_group_resolution(get_node);
    }

    sl_status_t command_class_thermostat_mode::on_thermostat_mode_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_mode_attribute_map_t payload)
    {
        uint8_t mode = 0;
        mode         = get_value_or_default(payload, "mode", mode);
        sl_log_debug(LOG_TAG.data(), "Thermostat mode received: %u", mode);

        // Per Z-Wave spec: Thermostat Mode Get only requires Thermostat Mode Report in response. Do not trigger
        // setpoint/capability chain on every report; only when the mode actually changed (e.g. after Set or unsolicited).
        const bool mode_changed                  = !m_thermostat_mode_reported_before_update.has_value() || *m_thermostat_mode_reported_before_update != mode;
        m_thermostat_mode_reported_before_update = std::nullopt;

        if (mode_changed) {
            component_connector connector;
            command_class_thermostat_mode_types::thermostat_mode_changed_payload_t event_payload {endpoint, mode};
            connector.fire_event(static_cast<uint32_t>(command_class_thermostat_mode_events_t::THERMOSTAT_MODE_CHANGED), event_payload);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_mode::on_thermostat_mode_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto mode_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::mode));
        auto no_of_mfr_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::no_of_manufacturer_data_fields));
        auto mfr_data_node  = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_mode_set_group_attributes_t::manufacturer_data));

        if (!mode_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = mode_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 5, .node = no_of_mfr_node.desired_exists() ? static_cast<attribute_store_node_t>(no_of_mfr_node) : ATTRIBUTE_STORE_INVALID_NODE, .node_value_state = DESIRED_ATTRIBUTE, .raw_value = 0},
        });

        if (mfr_data_node.desired_exists()) {
            frame_generator->add_value(mfr_data_node, DESIRED_ATTRIBUTE);
        }

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class