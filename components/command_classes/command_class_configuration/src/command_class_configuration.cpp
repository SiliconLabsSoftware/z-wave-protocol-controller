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

#include "command_class_configuration.hpp"
#include "command_class_configuration_constants.hpp"
#include "command_class_configuration_types.hpp"

#include "ZW_classcmd.h"
#include "attribute_resolver.h"
#include "log.h"
#include "zwave_command_class_indices.h"
#include "zwave_command_class_utils.hpp"
#include "zwave_frame_parser.hpp"
#include "zwave_tx.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_configuration";

    using namespace command_class_configuration_types;
    using namespace command_class_configuration_constants;

    command_class_configuration::command_class_configuration()
    {
        attribute_resolver_set_resolution_give_up_listener(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::CONFIGURATION_GET_GROUP), &command_class_configuration::on_configuration_get_resolution_give_up);
        register_default_reset_set_rule();
    }

    void command_class_configuration::register_default_reset_set_rule()
    {
        attribute_resolver::register_rules(
          static_cast<attribute_store_type_t>(configuration_default_reset_group_attributes_t::CONFIGURATION_DEFAULT_RESET_GROUP),
          [this](attribute_store_node_t node, uint8_t *data, uint16_t *length) {
              try {
                  auto command              = static_cast<uint8_t>(command_class_configuration_commands_t::COMMAND_CLASS_CONFIGURATION_CONFIGURATION_DEFAULT_RESET);
                  const auto version_status = validate_command_version(attribute_store::attribute(node), command, configuration_default_reset_min_version);
                  if (version_status != SL_STATUS_OK) {
                      return version_status;
                  }
                  m_frame_generator.initialize_frame(command, data, length);
                  auto endpoint_node = attribute_store::attribute(node).parent();
                  clear_reported_values_and_refresh(endpoint_node);
                  return m_frame_generator.generate_frame();
              } catch (const std::exception &e) {
                  sl_log_error(LOG_TAG.data(), "Error while generating CONFIGURATION_DEFAULT_RESET_GROUP : %s", e.what());
                  return SL_STATUS_FAIL;
              }
          },
          nullptr);
    }

    void command_class_configuration::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)endpoint_node;
        (void)supported_version;
        // The specification requires no Configuration command during interview.
    }

    uint8_t command_class_configuration::configuration_version(attribute_store::attribute endpoint_node)
    {
        auto version_node = endpoint_node.child_by_type(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION));
        if (version_node.is_valid() && version_node.reported_exists()) {
            return version_node.reported<uint8_t>();
        }
        return 0;
    }

    void command_class_configuration::request_properties_get(attribute_store::attribute endpoint_node, uint16_t parameter_number)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::CONFIGURATION_PROPERTIES_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number)).set_desired(parameter_number);
        command_class_configuration_core::start_group_resolution(group_node);
    }

    void command_class_configuration::request_configuration_get(attribute_store::attribute endpoint_node, uint8_t parameter_number)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::CONFIGURATION_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number)).set_desired(parameter_number);
        command_class_configuration_core::start_group_resolution(group_node);
    }

    void command_class_configuration::request_configuration_set(attribute_store::attribute endpoint_node, uint8_t parameter_number, uint8_t size, int64_t value, bool use_default)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::CONFIGURATION_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(parameter_number);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(size);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired(static_cast<uint8_t>(use_default ? 1 : 0));
        auto value_bytes = encode_configuration_value(value, size);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value)).set_desired(value_bytes);
        command_class_configuration_core::start_group_resolution(group_node);
    }

    void command_class_configuration::start_properties_walk(attribute_store::attribute endpoint_node)
    {
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::properties_walk_active)).set_reported<uint8_t>(1);
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_complete)).clear_reported();
        request_properties_get(endpoint_node, PARAMETER_NUMBER_PROBE_START);
    }

    void command_class_configuration::start_parameter_scan(attribute_store::attribute endpoint_node)
    {
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active)).set_reported<uint8_t>(1);
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_number)).set_reported<uint8_t>(PARAMETER_NUMBER_V1_MIN);
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_size_index)).set_reported<uint8_t>(0);
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::SIZE_PROBE));

        request_configuration_set(endpoint_node, PARAMETER_NUMBER_V1_MIN, PARAMETER_SIZES[0], SCAN_PROBE_VALUE, false);
        request_configuration_get(endpoint_node, PARAMETER_NUMBER_V1_MIN);
    }

    void command_class_configuration::start_default_bit_probe(attribute_store::attribute endpoint_node)
    {
        if (configuration_version(endpoint_node) >= 4) {
            return;
        }

        const auto parameters = endpoint_node.children(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID));
        if (parameters.size() < 2) {
            return;
        }

        const uint16_t first_id  = parameters[0].reported<uint16_t>();
        const uint16_t second_id = parameters[1].reported<uint16_t>();
        if (first_id > PARAMETER_NUMBER_V1_MAX || second_id > PARAMETER_NUMBER_V1_MAX) {
            return;
        }

        auto first_size_node  = parameters[0].child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size));
        auto second_size_node = parameters[1].child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size));
        if (!first_size_node.is_valid() || !first_size_node.reported_exists() || !second_size_node.is_valid() || !second_size_node.reported_exists()) {
            return;
        }

        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active)).set_reported<uint8_t>(1);
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining)).set_reported<uint8_t>(2);
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first)).set_reported(static_cast<uint8_t>(first_id));
        endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second)).set_reported(static_cast<uint8_t>(second_id));

        request_configuration_set(endpoint_node, static_cast<uint8_t>(first_id), first_size_node.reported<uint8_t>(), 0, true);
        request_configuration_set(endpoint_node, static_cast<uint8_t>(second_id), second_size_node.reported<uint8_t>(), 0, true);
        request_configuration_get(endpoint_node, static_cast<uint8_t>(first_id));
        request_configuration_get(endpoint_node, static_cast<uint8_t>(second_id));
    }

    void command_class_configuration::finish_size_probe_and_maybe_start_default_probe(attribute_store::attribute endpoint_node)
    {
        auto active_node = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active));
        if (active_node.is_valid()) {
            active_node.set_reported<uint8_t>(0);
        }
        start_default_bit_probe(endpoint_node);
    }

    void command_class_configuration::advance_parameter_scan(attribute_store::attribute endpoint_node, bool parameter_found)
    {
        auto active_node = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active));
        if (!active_node.is_valid() || !active_node.reported_exists() || active_node.reported<uint8_t>() == 0) {
            return;
        }

        auto phase_node  = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase));
        const auto phase = (phase_node.is_valid() && phase_node.reported_exists()) ? static_cast<parameter_scan_phase_t>(phase_node.reported<uint8_t>()) : parameter_scan_phase_t::SIZE_PROBE;
        if (phase != parameter_scan_phase_t::SIZE_PROBE) {
            return;
        }

        auto number_node     = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_number));
        auto size_index_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_size_index));
        uint8_t number       = number_node.reported_exists() ? number_node.reported<uint8_t>() : PARAMETER_NUMBER_V1_MIN;
        uint8_t size_index   = size_index_node.reported_exists() ? size_index_node.reported<uint8_t>() : 0;

        if (parameter_found) {
            if (number >= PARAMETER_NUMBER_V1_MAX) {
                finish_size_probe_and_maybe_start_default_probe(endpoint_node);
                return;
            }
            number_node.set_reported<uint8_t>(static_cast<uint8_t>(number + 1));
            size_index_node.set_reported<uint8_t>(0);
            request_configuration_set(endpoint_node, static_cast<uint8_t>(number + 1), PARAMETER_SIZES[0], SCAN_PROBE_VALUE, false);
            request_configuration_get(endpoint_node, static_cast<uint8_t>(number + 1));
            return;
        }

        if (static_cast<size_t>(size_index) + 1U < PARAMETER_SIZES.size()) {
            const uint8_t next_index = static_cast<uint8_t>(size_index + 1);
            size_index_node.set_reported(next_index);
            request_configuration_set(endpoint_node, number, PARAMETER_SIZES[next_index], SCAN_PROBE_VALUE, false);
            request_configuration_get(endpoint_node, number);
            return;
        }

        if (number >= PARAMETER_NUMBER_V1_MAX) {
            finish_size_probe_and_maybe_start_default_probe(endpoint_node);
            return;
        }

        number_node.set_reported<uint8_t>(static_cast<uint8_t>(number + 1));
        size_index_node.set_reported<uint8_t>(0);
        request_configuration_set(endpoint_node, static_cast<uint8_t>(number + 1), PARAMETER_SIZES[0], SCAN_PROBE_VALUE, false);
        request_configuration_get(endpoint_node, static_cast<uint8_t>(number + 1));
    }

    void command_class_configuration::continue_parameter_scan_after_give_up(attribute_store::attribute endpoint_node)
    {
        advance_parameter_scan(endpoint_node, false);
    }

    void command_class_configuration::on_configuration_get_resolution_give_up(attribute_store_node_t group_node_id)
    {
        attribute_store::attribute group_node(group_node_id);
        auto endpoint_node = group_node.parent();
        if (!endpoint_node.is_valid()) {
            return;
        }
        continue_parameter_scan_after_give_up(endpoint_node);
    }

    void command_class_configuration::clear_reported_values_and_refresh(attribute_store::attribute endpoint_node)
    {
        for (auto parameter_node: endpoint_node.children(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID))) {
            if (!parameter_node.reported_exists()) {
                continue;
            }
            const uint16_t parameter_number = parameter_node.reported<uint16_t>();
            auto value_node                 = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value));
            if (value_node.is_valid()) {
                value_node.clear_reported();
            }
            if (parameter_number >= PARAMETER_NUMBER_V1_MIN && parameter_number <= PARAMETER_NUMBER_V1_MAX) {
                request_configuration_get(endpoint_node, static_cast<uint8_t>(parameter_number));
            }
        }
    }

    void command_class_configuration::handle_default_probe_report(attribute_store::attribute endpoint, uint8_t parameter_number, int64_t value)
    {
        auto phase_node = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase));
        if (!phase_node.is_valid() || !phase_node.reported_exists()) {
            return;
        }

        const auto phase    = static_cast<parameter_scan_phase_t>(phase_node.reported<uint8_t>());
        auto first_node     = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first));
        auto second_node    = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second));
        auto remaining_node = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining));
        if (!first_node.is_valid() || !first_node.reported_exists() || !second_node.is_valid() || !second_node.reported_exists() || !remaining_node.is_valid() || !remaining_node.reported_exists()) {
            return;
        }

        const uint8_t first_id  = first_node.reported<uint8_t>();
        const uint8_t second_id = second_node.reported<uint8_t>();
        if (parameter_number != first_id && parameter_number != second_id) {
            return;
        }

        auto parameter_node = endpoint.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(parameter_number));
        if (!parameter_node.is_valid()) {
            return;
        }

        if (phase == parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS) {
            parameter_node.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported(value);

            uint8_t remaining = remaining_node.reported<uint8_t>();
            if (remaining > 0) {
                remaining_node.set_reported<uint8_t>(static_cast<uint8_t>(remaining - 1));
            }
            if (remaining_node.reported<uint8_t>() > 0) {
                return;
            }

            auto first_param    = endpoint.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(first_id));
            auto second_param   = endpoint.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(second_id));
            auto first_size     = first_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size));
            auto second_size    = second_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size));
            auto first_default  = first_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value));
            auto second_default = second_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value));
            if (!first_size.is_valid() || !first_size.reported_exists() || !second_size.is_valid() || !second_size.reported_exists() || !first_default.is_valid() || !first_default.reported_exists() || !second_default.is_valid() || !second_default.reported_exists()) {
                endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active)).set_reported<uint8_t>(0);
                return;
            }

            request_configuration_set(endpoint, first_id, first_size.reported<uint8_t>(), first_default.reported<int64_t>() + 1, false);
            request_configuration_set(endpoint, second_id, second_size.reported<uint8_t>(), second_default.reported<int64_t>() + 1, false);

            phase_node.set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
            remaining_node.set_reported<uint8_t>(2);
            request_configuration_set(endpoint, first_id, first_size.reported<uint8_t>(), 0, true);
            request_configuration_get(endpoint, first_id);
            request_configuration_get(endpoint, second_id);
            return;
        }

        if (phase == parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET) {
            uint8_t remaining = remaining_node.reported<uint8_t>();
            if (remaining > 0) {
                remaining_node.set_reported<uint8_t>(static_cast<uint8_t>(remaining - 1));
            }
            if (remaining_node.reported<uint8_t>() > 0) {
                return;
            }

            auto first_param    = endpoint.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(first_id));
            auto second_param   = endpoint.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(second_id));
            auto first_value    = first_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value));
            auto second_value   = second_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value));
            auto first_default  = first_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value));
            auto second_default = second_param.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value));

            uint8_t resets_every = 0;
            if (first_value.is_valid() && first_value.reported_exists() && second_value.is_valid() && second_value.reported_exists() && first_default.is_valid() && first_default.reported_exists() && second_default.is_valid() && second_default.reported_exists()
                && first_value.reported<int64_t>() == first_default.reported<int64_t>() && second_value.reported<int64_t>() == second_default.reported<int64_t>()) {
                resets_every = 1;
            }
            endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter)).set_reported(resets_every);
            endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active)).set_reported<uint8_t>(0);
        }
    }

    bool command_class_configuration::bulk_set_allowed(attribute_store::attribute endpoint_node, uint16_t parameter_offset, uint8_t number_of_parameters)
    {
        for (uint8_t i = 0; i < number_of_parameters; ++i) {
            const uint32_t parameter_number = static_cast<uint32_t>(parameter_offset) + i;
            if (parameter_number > 0xFFFF) {
                sl_log_warning(LOG_TAG.data(), "Configuration Bulk Set range overflows 16-bit parameter numbers");
                return false;
            }
            auto parameter_node = endpoint_node.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(parameter_number));
            if (!parameter_node.is_valid()) {
                continue;
            }
            auto read_only_node = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only));
            if (read_only_node.is_valid() && read_only_node.reported_exists() && read_only_node.reported<uint8_t>() != 0) {
                sl_log_debug(LOG_TAG.data(), "Skipping Bulk Set: parameter %u is read-only", static_cast<unsigned>(parameter_number));
                return false;
            }
            auto no_bulk_node = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support));
            if (no_bulk_node.is_valid() && no_bulk_node.reported_exists() && no_bulk_node.reported<uint8_t>() != 0) {
                sl_log_debug(LOG_TAG.data(), "Skipping Bulk Set: parameter %u advertises no_bulk_support", static_cast<unsigned>(parameter_number));
                return false;
            }
        }
        return true;
    }

    sl_status_t command_class_configuration::control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        if (frame_length >= 2 && frame_data[COMMAND_INDEX] == static_cast<uint8_t>(command_class_configuration_commands_t::COMMAND_CLASS_CONFIGURATION_CONFIGURATION_BULK_REPORT)) {
            // Generated parser reads VARIANT size incorrectly inside the vg; handle Bulk Report here.
            return handle_configuration_bulk_report(connection_info, frame_data, frame_length);
        }

        if (frame_length >= 4 && frame_data[COMMAND_INDEX] == static_cast<uint8_t>(command_class_configuration_commands_t::COMMAND_CLASS_CONFIGURATION_CONFIGURATION_PROPERTIES_REPORT)) {
            pending_properties_frame_.assign(frame_data, frame_data + frame_length);
        } else {
            pending_properties_frame_.clear();
        }
        return command_class_configuration_core::control_handler(connection_info, frame_data, frame_length);
    }

    sl_status_t command_class_configuration::handle_configuration_bulk_report(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length)
    {
        attribute_store::attribute endpoint_node {command_class_utils::get_endpoint_node(connection_info)};
        zwave_frame_parser frame_parser(frame_data, frame_length);
        command_class_configuration_attribute_map_t attribute_map;

        try {
            frame_parser.read_sequential<uint8_t>(1);  // command class
            frame_parser.read_sequential<uint8_t>(1);  // command

            const auto parameter_offset     = frame_parser.read_sequential<uint16_t>(sizeof(uint16_t));
            const auto number_of_parameters = frame_parser.read_sequential<uint8_t>(sizeof(uint8_t));
            const auto reports_to_follow    = frame_parser.read_sequential<uint8_t>(sizeof(uint8_t));
            const auto properties1_value    = frame_parser.read_sequential<uint8_t>(sizeof(uint8_t));
            const uint8_t size              = properties1_value & static_cast<uint8_t>(configuration_bulk_report_properties1_attribute_masks_t::size_mask);
            const uint8_t handshake         = (properties1_value & static_cast<uint8_t>(configuration_bulk_report_properties1_attribute_masks_t::handshake_mask)) != 0 ? 1 : 0;
            const uint8_t default_flag      = (properties1_value & static_cast<uint8_t>(configuration_bulk_report_properties1_attribute_masks_t::default_flag_mask)) != 0 ? 1 : 0;

            configuration_bulk_report_vg_t vg;
            if (is_valid_size(size)) {
                for (uint8_t i = 0; i < number_of_parameters; ++i) {
                    configuration_bulk_report_vg_t_item_t item;
                    item.parameter = frame_parser.read_sequential<std::vector<uint8_t>>(size);
                    vg.push_back(std::move(item));
                }
            }

            attribute_map.insert({"parameter_offset", parameter_offset});
            attribute_map.insert({"number_of_parameters", number_of_parameters});
            attribute_map.insert({"reports_to_follow", reports_to_follow});
            attribute_map.insert({"size", size});
            attribute_map.insert({"handshake", handshake});
            attribute_map.insert({"default_flag", default_flag});
            attribute_map.insert({"vg", vg});
        } catch (const std::exception &e) {
            sl_log_error(LOG_TAG.data(), "Error while parsing CONFIGURATION_BULK_REPORT frame: %s", e.what());
            return SL_STATUS_FAIL;
        }

        const sl_status_t parse_status = on_configuration_bulk_report_received_store(endpoint_node, attribute_map);
        if (parse_status != SL_STATUS_OK) {
            return parse_status;
        }

        auto get_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::CONFIGURATION_BULK_GET_GROUP));
        if (get_group.is_valid()) {
            stop_group_resolution(get_group);
        }
        auto set_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::CONFIGURATION_BULK_SET_GROUP));
        if (set_group.is_valid()) {
            stop_group_resolution(set_group);
        }

        mqtt_publish_report(endpoint_node, command_class_configuration_commands_t::COMMAND_CLASS_CONFIGURATION_CONFIGURATION_BULK_REPORT, attribute_map);
        on_configuration_bulk_report_parsed(connection_info, endpoint_node, attribute_map);
        return SL_STATUS_OK;
    }

    uint16_t command_class_configuration::apply_parameter_0_next_quirk(uint16_t parameter_number, uint16_t next_parameter_number)
    {
        // CC:0070.03.00.22.001 / CC:0070.03.00.22.002: when requesting parameter 0 and next
        // looks like 0x0000, take the next number from the last two bytes of the frame.
        if (parameter_number != PARAMETER_NUMBER_PROBE_START || next_parameter_number != NEXT_PARAMETER_TERMINATOR) {
            return next_parameter_number;
        }
        if (pending_properties_frame_.size() < 4) {
            return next_parameter_number;
        }
        const size_t length = pending_properties_frame_.size();
        return static_cast<uint16_t>((pending_properties_frame_[length - 2] << 8) | pending_properties_frame_[length - 1]);
    }

    sl_status_t command_class_configuration::on_configuration_properties_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload)
    {
        (void)connection_info;

        auto walk_active = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::properties_walk_active));
        if (!walk_active.is_valid() || !walk_active.reported_exists() || walk_active.reported<uint8_t>() == 0) {
            pending_properties_frame_.clear();
            return SL_STATUS_OK;
        }

        configuration_properties_report_next_parameter_number_t next_parameter_number = NEXT_PARAMETER_TERMINATOR;
        next_parameter_number                                                         = get_value_or_default(payload, "next_parameter_number", next_parameter_number);
        configuration_properties_report_parameter_number_t parameter_number           = 0;
        parameter_number                                                              = get_value_or_default(payload, "parameter_number", parameter_number);

        next_parameter_number = apply_parameter_0_next_quirk(parameter_number, next_parameter_number);
        pending_properties_frame_.clear();

        if (next_parameter_number == NEXT_PARAMETER_TERMINATOR) {
            walk_active.set_reported<uint8_t>(0);
            endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_complete)).set_reported<uint8_t>(1);
            return SL_STATUS_OK;
        }

        request_properties_get(endpoint, next_parameter_number);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration::on_configuration_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload)
    {
        (void)connection_info;

        auto scan_active = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active));
        if (!scan_active.is_valid() || !scan_active.reported_exists() || scan_active.reported<uint8_t>() == 0) {
            return SL_STATUS_OK;
        }

        auto phase_node  = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase));
        const auto phase = (phase_node.is_valid() && phase_node.reported_exists()) ? static_cast<parameter_scan_phase_t>(phase_node.reported<uint8_t>()) : parameter_scan_phase_t::SIZE_PROBE;

        configuration_report_parameter_number_t parameter_number = 0;
        parameter_number                                         = get_value_or_default(payload, "parameter_number", parameter_number);
        uint8_t size                                             = 0;
        size                                                     = get_value_or_default(payload, "size", size);

        if (phase == parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS || phase == parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET) {
            auto parameter_node = endpoint.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(parameter_number));
            int64_t value       = 0;
            if (parameter_node.is_valid()) {
                auto value_node = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value));
                if (value_node.is_valid() && value_node.reported_exists()) {
                    value = value_node.reported<int64_t>();
                }
            }
            handle_default_probe_report(endpoint, parameter_number, value);
            return SL_STATUS_OK;
        }

        auto number_node     = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_number));
        auto size_index_node = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_size_index));
        if (!number_node.is_valid() || !number_node.reported_exists() || !size_index_node.is_valid() || !size_index_node.reported_exists()) {
            return SL_STATUS_OK;
        }

        const uint8_t expected_number = number_node.reported<uint8_t>();
        const uint8_t size_index      = size_index_node.reported<uint8_t>();
        if (parameter_number != expected_number || static_cast<size_t>(size_index) >= PARAMETER_SIZES.size()) {
            return SL_STATUS_OK;
        }

        if (size == PARAMETER_SIZES[size_index]) {
            advance_parameter_scan(endpoint, true);
        }

        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration::on_configuration_name_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload)
    {
        configuration_name_report_reports_to_follow_t reports_to_follow = 0;
        reports_to_follow                                               = get_value_or_default(payload, "reports_to_follow", reports_to_follow);
        if (reports_to_follow > 0 && connection_info != nullptr) {
            zwave_tx_set_expected_frames(connection_info->remote.node_id, reports_to_follow);
        }
        (void)endpoint;
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration::on_configuration_info_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload)
    {
        configuration_info_report_reports_to_follow_t reports_to_follow = 0;
        reports_to_follow                                               = get_value_or_default(payload, "reports_to_follow", reports_to_follow);
        if (reports_to_follow > 0 && connection_info != nullptr) {
            zwave_tx_set_expected_frames(connection_info->remote.node_id, reports_to_follow);
        }
        (void)endpoint;
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration::on_configuration_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto parameter_number_node = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number));
        if (!parameter_number_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(parameter_number_node, DESIRED_ATTRIBUTE);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_configuration::on_configuration_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;
        auto endpoint_node          = group_node.parent();

        auto parameter_number_node = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number));
        auto size_node             = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size));
        auto default_node          = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag));
        auto value_node            = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value));

        if (!parameter_number_node.desired_exists() || !size_node.desired_exists() || !default_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        const uint8_t parameter_number = parameter_number_node.desired<uint8_t>();
        const uint8_t size             = size_node.desired<uint8_t>();
        const uint8_t use_default      = default_node.desired<uint8_t>();

        if (!is_valid_size(size)) {
            return SL_STATUS_FAIL;
        }

        auto parameter_node = endpoint_node.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(parameter_number));
        if (parameter_node.is_valid()) {
            auto read_only_node = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only));
            if (read_only_node.is_valid() && read_only_node.reported_exists() && read_only_node.reported<uint8_t>() != 0) {
                sl_log_debug(LOG_TAG.data(), "Skipping Set for read-only parameter %u", parameter_number);
                return SL_STATUS_FAIL;
            }
        }

        frame_generator->add_value(parameter_number_node, DESIRED_ATTRIBUTE);

        configuration_set_level_t level;
        level.value                                = 0;
        level.flags.configuration_set_size         = size;
        level.flags.configuration_set_default_flag = use_default != 0 ? 1 : 0;
        frame_generator->add_raw_byte(level.value);

        if (use_default == 0) {
            if (!value_node.desired_exists()) {
                return SL_STATUS_NOT_READY;
            }
            auto value_bytes = value_node.desired<std::vector<uint8_t>>();
            if (value_bytes.size() != size) {
                return SL_STATUS_FAIL;
            }
            for (uint8_t byte: value_bytes) {
                frame_generator->add_raw_byte(byte);
            }
        } else {
            for (uint8_t i = 0; i < size; ++i) {
                frame_generator->add_raw_byte(0);
            }
        }

        request_configuration_get(endpoint_node, parameter_number);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_configuration::on_configuration_name_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto parameter_number_node = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::parameter_number));
        if (!parameter_number_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(parameter_number_node, DESIRED_ATTRIBUTE);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_configuration::on_configuration_info_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto parameter_number_node = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::parameter_number));
        if (!parameter_number_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(parameter_number_node, DESIRED_ATTRIBUTE);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_configuration::on_configuration_properties_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto parameter_number_node = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number));
        if (!parameter_number_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(parameter_number_node, DESIRED_ATTRIBUTE);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_configuration::on_configuration_bulk_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_configuration_attribute_map_t payload)
    {
        (void)endpoint;
        configuration_bulk_report_reports_to_follow_t reports_to_follow = 0;
        reports_to_follow                                               = get_value_or_default(payload, "reports_to_follow", reports_to_follow);
        if (reports_to_follow > 0 && connection_info != nullptr) {
            zwave_tx_set_expected_frames(connection_info->remote.node_id, reports_to_follow);
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration::on_configuration_bulk_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto parameter_offset_node     = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::parameter_offset));
        auto number_of_parameters_node = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::number_of_parameters));
        if (!parameter_offset_node.desired_exists() || !number_of_parameters_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        frame_generator->add_value(parameter_offset_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(number_of_parameters_node, DESIRED_ATTRIBUTE);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_configuration::on_configuration_bulk_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        (void)data;
        (void)length;
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto parameter_offset_node     = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset));
        auto number_of_parameters_node = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters));
        auto size_node                 = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size));
        auto handshake_node            = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake));
        auto default_node              = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag));
        auto vg_node                   = group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg));

        if (!parameter_offset_node.desired_exists() || !number_of_parameters_node.desired_exists() || !size_node.desired_exists() || !handshake_node.desired_exists() || !default_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }

        const uint8_t size        = size_node.desired<uint8_t>();
        const uint8_t use_default = default_node.desired<uint8_t>();
        const uint8_t handshake   = handshake_node.desired<uint8_t>();
        const uint8_t number      = number_of_parameters_node.desired<uint8_t>();
        if (!is_valid_size(size) || number == 0) {
            return SL_STATUS_FAIL;
        }

        frame_generator->add_value(parameter_offset_node, DESIRED_ATTRIBUTE);
        frame_generator->add_value(number_of_parameters_node, DESIRED_ATTRIBUTE);

        configuration_bulk_set_properties1_t properties1;
        properties1.value                                     = 0;
        properties1.flags.configuration_bulk_set_size         = size;
        properties1.flags.configuration_bulk_set_handshake    = handshake != 0 ? 1 : 0;
        properties1.flags.configuration_bulk_set_default_flag = use_default != 0 ? 1 : 0;
        frame_generator->add_raw_byte(properties1.value);

        if (use_default != 0) {
            for (uint16_t i = 0; i < static_cast<uint16_t>(number) * size; ++i) {
                frame_generator->add_raw_byte(0);
            }
        } else {
            if (!vg_node.desired_exists()) {
                return SL_STATUS_NOT_READY;
            }
            auto flat_values = vg_node.desired<std::vector<uint8_t>>();
            if (flat_values.size() != static_cast<size_t>(number) * size) {
                return SL_STATUS_FAIL;
            }
            for (uint8_t byte: flat_values) {
                frame_generator->add_raw_byte(byte);
            }
        }

        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
