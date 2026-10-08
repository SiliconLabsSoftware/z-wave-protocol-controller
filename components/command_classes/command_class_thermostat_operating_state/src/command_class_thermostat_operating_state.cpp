
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

#include "command_class_thermostat_operating_state.hpp"
#include "command_class_thermostat_operating_state_constants.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_thermostat_operating_state";

    command_class_thermostat_operating_state::command_class_thermostat_operating_state() {}

    void command_class_thermostat_operating_state::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_REPORT_GROUP));

        auto state_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_REPORT_GROUP));
        cc_interview_require_attribute(state_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_report_group_attributes_t::operating_state)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_GET_GROUP)));

        if (supported_version < thermostat_operating_state_logging_supported_get_min_version) {
            return;
        }

        invalidate_report_groups(endpoint_node, static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT_GROUP));

        auto supported_report = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT_GROUP));
        cc_interview_require_attribute(supported_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_logging_supported_report_group_attributes_t::bit_mask)));
        start_group_resolution(endpoint_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_supported_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_SUPPORTED_GET_GROUP)));
    }

    sl_status_t command_class_thermostat_operating_state::on_thermostat_operating_logging_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_thermostat_operating_state_attribute_map_t payload)
    {
        (void)connection_info;

        if (endpoint_supported_version(endpoint) < thermostat_operating_state_logging_get_min_version) {
            return SL_STATUS_OK;
        }

        auto bit_mask = get_value_or_default(payload, "bit_mask", thermostat_operating_logging_supported_report_bit_mask_t {});
        if (!command_class_thermostat_operating_state_constants::logging_supported_bit_mask_has_logs(bit_mask)) {
            return SL_STATUS_OK;
        }

        auto log_report = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_REPORT_GROUP));
        cc_interview_require_attribute(log_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::reports_to_follow)));
        cc_interview_require_attribute(log_report.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_report_group_attributes_t::vg1)));

        auto get_group     = endpoint.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::THERMOSTAT_OPERATING_STATE_LOGGING_GET_GROUP));
        auto bit_mask_node = get_group.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::bit_mask));
        bit_mask_node.set_desired<std::vector<uint8_t>>(bit_mask);
        start_group_resolution(get_group);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_thermostat_operating_state::on_thermostat_operating_state_logging_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto bit_mask_node = group_node.emplace_node(static_cast<attribute_store_type_t>(thermostat_operating_state_logging_get_group_attributes_t::bit_mask));
        if (!bit_mask_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        const auto bit_mask = bit_mask_node.desired<std::vector<uint8_t>>();
        for (uint8_t byte: bit_mask) {
            frame_generator->add_raw_byte(byte);
        }

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
