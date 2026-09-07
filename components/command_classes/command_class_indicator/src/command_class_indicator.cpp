
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

#include <algorithm>
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_indicator.hpp"
#include "command_class_indicator_attributes.hpp"
#include "command_class_indicator_constants.hpp"

// Z-Wave definitions
#include "ZW_classcmd.h"
#include "sl_status.h"
#include "zwave_command_class_utils.hpp"
#include "log.h"

#include "component_connector.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_indicator";

    command_class_indicator::command_class_indicator()
    {
        register_attribute_types({
          {static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP), "SUPPORTED_INDICATORS_GROUP", ATTRIBUTE_ENDPOINT_ID, U8_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::supported_indicators), "supported_indicators", static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP), BYTE_ARRAY_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::finished_indicators), "finished_indicators", static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP), BYTE_ARRAY_STORAGE_TYPE},
          {static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::interview_finished), "interview_finished", static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP), U8_STORAGE_TYPE},
        });

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND), command_class_association_grp_info_types::component_connector_agi_lifeline_command_payload_t {COMMAND_CLASS_INDICATOR, INDICATOR_REPORT});
    }

    void command_class_indicator::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        // Initialize the supported indicator store attributes.
        auto supported_list_group = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP));
        auto supported_list_node  = supported_list_group.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::supported_indicators));
        supported_list_node.set_reported<std::vector<uint8_t>>({});
        auto finished_node = supported_list_group.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::finished_indicators));
        finished_node.set_reported<std::vector<uint8_t>>({});
        auto interview_finished_node = supported_list_group.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::interview_finished));
        interview_finished_node.set_reported<uint8_t>(0);

        // CL:0087.01.21.01.1
        if (supported_version >= 2) {
            auto group_node        = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_get_group_attributes_t::INDICATOR_SUPPORTED_GET_GROUP));
            auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_get_group_attributes_t::indicator_id));
            indicator_id_node.set_desired<uint8_t>(static_cast<uint8_t>(command_class_indicator_constants::indicator_id::NA));  // Set 0x00 to query first supported Indicator ID
            start_group_resolution(group_node);
        } else {
            auto group_node        = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::INDICATOR_GET_GROUP));
            auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::indicator_id));
            indicator_id_node.set_desired<uint8_t>(static_cast<uint8_t>(command_class_indicator_constants::indicator_id::NA));
            start_group_resolution(group_node, {.retry_count = 1});  // Decrease retry count to 1 for the first indicator get.
        }
    }

    sl_status_t command_class_indicator::on_indicator_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_indicator_attribute_map_t payload)
    {
        (void)connection_info;
        uint8_t next_indicator_id = get_value_or_default(payload, "next_indicator_id", static_cast<uint8_t>(command_class_indicator_constants::indicator_id::NA));
        uint8_t indicator_id      = get_value_or_default(payload, "indicator_id", static_cast<uint8_t>(command_class_indicator_constants::indicator_id::NA));

        auto supported_list_group    = endpoint.child_by_type(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP));
        auto supported_list_node     = supported_list_group.child_by_type(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::supported_indicators));
        auto interview_finished_node = supported_list_group.child_by_type(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::interview_finished));
        if (interview_finished_node.reported<uint8_t>() != 0) {
            // If interview is finished, do nothing with the parsed report.
            return SL_STATUS_OK;
        }

        const auto na_id         = static_cast<uint8_t>(command_class_indicator_constants::indicator_id::NA);
        std::vector<uint8_t> ids = supported_list_node.reported<std::vector<uint8_t>>();
        if (indicator_id != na_id && std::find(ids.begin(), ids.end(), indicator_id) == ids.end()) {
            ids.push_back(indicator_id);
            supported_list_node.set_reported<std::vector<uint8_t>>(ids);
        }

        if (next_indicator_id != static_cast<uint8_t>(command_class_indicator_constants::indicator_id::NA)) {
            // Continue with the next supported indicator if there is any.
            auto group_node        = endpoint.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_get_group_attributes_t::INDICATOR_SUPPORTED_GET_GROUP));
            auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_get_group_attributes_t::indicator_id));
            indicator_id_node.set_desired<uint8_t>(next_indicator_id);
            start_group_resolution(group_node);
        } else {
            // Supported discovery finished: interview each supported ID with Indicator Get, tracking progress in
            // finished_indicators until it matches supported_indicators.
            auto supported_list_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP));
            auto supported_list_node  = supported_list_group.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::supported_indicators));
            std::vector<uint8_t> ids  = supported_list_node.reported<std::vector<uint8_t>>();
            auto finished_node        = supported_list_group.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::finished_indicators));
            finished_node.set_reported<std::vector<uint8_t>>({});
            auto interview_node = supported_list_group.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::interview_finished));
            if (ids.empty()) {
                interview_node.set_reported<uint8_t>(1);
            } else {
                interview_node.set_reported<uint8_t>(0);
                auto group_node        = endpoint.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::INDICATOR_GET_GROUP));
                auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::indicator_id));
                indicator_id_node.set_desired<uint8_t>(ids.front());
                start_group_resolution(group_node);
            }
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator::on_indicator_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_indicator_attribute_map_t payload)
    {
        (void)connection_info;
        auto supported_list_group    = endpoint.child_by_type(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::SUPPORTED_INDICATORS_GROUP));
        auto interview_finished_node = supported_list_group.child_by_type(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::interview_finished));
        if (interview_finished_node.reported<uint8_t>() != 0) {
            // If interview is finished, do nothing with the parsed report.
            return SL_STATUS_OK;
        }

        auto supported_list_node           = supported_list_group.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::supported_indicators));
        std::vector<uint8_t> supported_ids = supported_list_node.reported<std::vector<uint8_t>>();
        auto finished_node                 = supported_list_group.child_by_type(static_cast<attribute_store_type_t>(indicator_supported_indicator_store_attributes_t::finished_indicators));
        std::vector<uint8_t> finished_ids  = finished_node.reported<std::vector<uint8_t>>();

        if (finished_ids.size() >= supported_ids.size()) {
            interview_finished_node.set_reported<uint8_t>(1);
            return SL_STATUS_OK;
        }

        indicator_report_vg1_t vg1;
        vg1 = get_value_or_default(payload, "vg1", vg1);
        if (vg1.empty()) {
            return SL_STATUS_EMPTY;
        }
        const uint8_t reported_indicator_id = vg1.front().indicator_id;
        const uint8_t expected_id           = supported_ids.at(finished_ids.size());
        if (reported_indicator_id != expected_id) {
            return SL_STATUS_OK;
        }

        finished_ids.push_back(reported_indicator_id);
        finished_node.set_reported<std::vector<uint8_t>>(finished_ids);

        if (finished_ids == supported_ids) {
            interview_finished_node.set_reported<uint8_t>(1);
            return SL_STATUS_OK;
        }

        // Start the next indicator get.
        auto group_node        = endpoint.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::INDICATOR_GET_GROUP));
        auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::indicator_id));
        indicator_id_node.set_desired<uint8_t>(supported_ids.at(finished_ids.size()));
        start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator::on_indicator_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_get_group_attributes_t::indicator_id));
        if (!indicator_id_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(indicator_id_node, DESIRED_ATTRIBUTE);
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_indicator::on_indicator_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        auto indicator_0_value_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_set_group_attributes_t::indicator_0_value));
        if (!indicator_0_value_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(indicator_0_value_node, DESIRED_ATTRIBUTE);

        auto indicator_object_count_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_set_group_attributes_t::indicator_object_count));
        if (!indicator_object_count_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(indicator_object_count_node, DESIRED_ATTRIBUTE);

        if (indicator_object_count_node.desired<uint8_t>() > 0) {
            auto vg1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_set_group_attributes_t::vg1));
            if (!vg1_node.desired_exists()) {
                return SL_STATUS_NOT_READY;
            }
            frame_generator->add_value(vg1_node, DESIRED_ATTRIBUTE);
        }

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_indicator::on_indicator_supported_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_supported_get_group_attributes_t::indicator_id));
        if (!indicator_id_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(indicator_id_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_indicator::on_indicator_description_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        auto indicator_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_description_get_group_attributes_t::indicator_id));
        if (!indicator_id_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_value(indicator_id_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_indicator::on_indicator_supported_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        using namespace command_class_indicator_constants;

        const auto indicator_id_na            = static_cast<uint8_t>(indicator_id::NA);
        const auto indicator_id_node_identify = static_cast<uint8_t>(indicator_id::NODE_IDENTIFY);

        uint8_t requested_id = get_value_or_default(attribute_map, "indicator_id", indicator_id_na);

        if (requested_id == indicator_id_na || requested_id == indicator_id_node_identify) {
            // Indicator ID 0x50 (Node Identify) — the only supported indicator
            report_frame.add_raw_byte(indicator_id_node_identify);
            // No more indicators after this one
            report_frame.add_raw_byte(indicator_id_na);
            // Properties1: Reserved (3 bits, 0) | Bit Mask Length (5 bits)
            report_frame.add_raw_byte(NODE_IDENTIFY_BITMASK_LENGTH);
            // Property bitmask: properties 0x03, 0x04, 0x05
            report_frame.add_raw_byte(NODE_IDENTIFY_SUPPORTED_PROPERTIES_MASK);
        } else {
            // Unsupported indicator: spec requires ALL fields set to 0x00
            report_frame.add_raw_byte(indicator_id_na);
            report_frame.add_raw_byte(indicator_id_na);
            report_frame.add_raw_byte(PROPERTY_SUPPORTED_BITMASK_LENGTH_NONE);
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator::on_indicator_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        using namespace command_class_indicator_constants;

        const auto indicator_id_na            = static_cast<uint8_t>(indicator_id::NA);
        const auto indicator_id_node_identify = static_cast<uint8_t>(indicator_id::NODE_IDENTIFY);

        uint8_t requested_id = get_value_or_default(attribute_map, "indicator_id", indicator_id_na);

        // Indicator 0 Value — legacy V1 backward compat, 0 when using V2+ objects
        report_frame.add_raw_byte(INDICATOR_0_VALUE_V2_PLUS);

        if (requested_id == indicator_id_na || requested_id == indicator_id_node_identify) {
            attribute_store::attribute endpoint_node(command_class_utils::get_zpc_endpoint_node(connection_info));
            if (!endpoint_node.is_valid()) {
                return SL_STATUS_FAIL;
            }
            auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::INDICATOR_REPORT_GROUP));
            auto vg1_node   = group_node.child_by_type(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::vg1));

            indicator_report_vg1_t stored_vg1;
            if (vg1_node != ATTRIBUTE_STORE_INVALID_NODE) {
                auto vg1_attr = attribute_store::attribute(vg1_node);
                if (vg1_attr.reported_exists()) {
                    stored_vg1 = vg1_attr.reported<indicator_report_vg1_t>();
                }
            }

            // Properties1: Reserved (3 bits, 0) | Indicator Object Count (5 bits)
            report_frame.add_raw_byte(static_cast<uint8_t>(NODE_IDENTIFY_PROPERTIES.size()));

            // All objects MUST carry the same Indicator ID per spec
            for (const auto property: NODE_IDENTIFY_PROPERTIES) {
                uint8_t prop_id = static_cast<uint8_t>(property);
                uint8_t value   = PROPERTY_VALUE_DEFAULT;
                for (const auto &item: stored_vg1) {
                    if (item.indicator_id == indicator_id_node_identify && item.property_id == prop_id) {
                        value = item.value;
                        break;
                    }
                }
                report_frame.add_raw_byte(indicator_id_node_identify);
                report_frame.add_raw_byte(prop_id);
                report_frame.add_raw_byte(value);
            }
        } else {
            // Unsupported indicator: MUST return one object with that ID,
            // Property ID = 0x00, Value = 0x00
            report_frame.add_raw_byte(UNSUPPORTED_INDICATOR_OBJECT_COUNT);
            report_frame.add_raw_byte(requested_id);
            report_frame.add_raw_byte(UNSUPPORTED_INDICATOR_PROPERTY_ID);
            report_frame.add_raw_byte(UNSUPPORTED_INDICATOR_VALUE);
        }

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator::on_indicator_description_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        using namespace command_class_indicator_constants;

        uint8_t requested_id = get_value_or_default(attribute_map, "indicator_id", static_cast<uint8_t>(indicator_id::NA));

        // Return the requested Indicator ID and DescriptionLength=0.
        // Descriptions are only required for manufacturer-defined indicators (0x80-0x9F).
        report_frame.add_raw_byte(requested_id);
        report_frame.add_raw_byte(INDICATOR_DESCRIPTION_LENGTH_NONE);

        frame = report_frame.generate_frame();
        return SL_STATUS_OK;
    }

    sl_status_t command_class_indicator::on_indicator_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_indicator_attribute_map_t attribute_map)
    {
        using namespace command_class_indicator_constants;

        const auto indicator_id_node_identify = static_cast<uint8_t>(indicator_id::NODE_IDENTIFY);

        indicator_set_vg1_t set_vg1;
        set_vg1 = get_value_or_default(attribute_map, "vg1", set_vg1);

        attribute_store::attribute endpoint_node(command_class_utils::get_zpc_endpoint_node(connection_info));
        if (!endpoint_node.is_valid()) {
            return SL_STATUS_FAIL;
        }
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::INDICATOR_REPORT_GROUP));

        // Read existing stored values
        auto vg1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(indicator_report_group_attributes_t::vg1));
        indicator_report_vg1_t report_vg1;
        if (vg1_node.reported_exists()) {
            report_vg1 = vg1_node.reported<indicator_report_vg1_t>();
        }

        // Apply incoming Set values
        for (const auto &set_item: set_vg1) {
            if (set_item.indicator_id != indicator_id_node_identify) {
                continue;
            }
            bool found = false;
            for (auto &existing: report_vg1) {
                if (existing.indicator_id == set_item.indicator_id && existing.property_id == set_item.property_id) {
                    existing.value = set_item.value;
                    found          = true;
                    break;
                }
            }
            if (!found) {
                report_vg1.push_back({set_item.indicator_id, set_item.property_id, set_item.value});
            }
        }

        // Per spec: non-specified Property IDs MUST be assumed as 0x00
        if (!set_vg1.empty() && set_vg1.front().indicator_id == indicator_id_node_identify) {
            for (const auto property: NODE_IDENTIFY_PROPERTIES) {
                uint8_t prop_id = static_cast<uint8_t>(property);
                bool exists     = false;
                for (const auto &item: report_vg1) {
                    if (item.indicator_id == indicator_id_node_identify && item.property_id == prop_id) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    report_vg1.push_back({indicator_id_node_identify, prop_id, PROPERTY_VALUE_DEFAULT});
                }
            }
        }

        vg1_node.set_reported<indicator_report_vg1_t>(report_vg1);

        publish_indicator_set_received(endpoint_node, get_value_or_default<uint8_t>(attribute_map, "indicator_0_value", uint8_t(0)), get_value_or_default<uint8_t>(attribute_map, "indicator_object_count", uint8_t(0)), set_vg1);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class