
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
#include "command_class_firmware_update_md.hpp"

// Component connector for firing events to consumers
#include "component_connector.hpp"
#include "command_class_firmware_update_md_events.hpp"
#include "command_class_firmware_update_md_types.hpp"
#include "command_class_firmware_update_md_generated_types.hpp"
#include "command_class_firmware_update_md_core.hpp"

// Z-Wave definitions
#include "ZW_classcmd.h"

// Z-Wave TX (for sending firmware report frames directly)
#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"
#include "zwave_crc16.h"
#include "zpc_attribute_store_network_helper.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_firmware_update_md";

    using command_class_firmware_update_md_types::component_connector_firmware_update_md_report_payload_t;

    command_class_firmware_update_md::command_class_firmware_update_md()
    {
        component_connector connector;
        connector.connect_typed<command_class_firmware_update_md_events_t, attribute_store::attribute>(command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_FIRMWARE_MD_GET,
                                                                                                       [](const attribute_store::attribute &endpoint_node) -> sl_status_t { return zwave_command_class::command_class_firmware_update_md::on_firmware_md_get_requested(endpoint_node); });

        using command_class_firmware_update_md_types::command_class_firmware_update_md_request_get_payload_t;
        connector.connect_typed<command_class_firmware_update_md_events_t, command_class_firmware_update_md_request_get_payload_t>(
          command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_REQUEST_GET,
          [](const command_class_firmware_update_md_request_get_payload_t &payload) -> sl_status_t { return zwave_command_class::command_class_firmware_update_md::on_firmware_update_md_request_get_requested(payload); });

        using command_class_firmware_update_md_types::command_class_firmware_update_md_report_payload_t;
        connector.connect_typed<command_class_firmware_update_md_events_t, command_class_firmware_update_md_report_payload_t>(command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_REPORT, [](const command_class_firmware_update_md_report_payload_t &payload) -> sl_status_t {
            return zwave_command_class::command_class_firmware_update_md::on_firmware_update_md_report_requested(payload);
        });

        using command_class_firmware_update_md_types::command_class_firmware_update_md_activation_set_payload_t;
        connector.connect_typed<command_class_firmware_update_md_events_t, command_class_firmware_update_md_activation_set_payload_t>(
          command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_ACTIVATION_SET,
          [](const command_class_firmware_update_md_activation_set_payload_t &payload) -> sl_status_t { return zwave_command_class::command_class_firmware_update_md::on_firmware_update_activation_set_requested(payload); });
    }

    void command_class_firmware_update_md::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version)
    {
        (void)supported_version;
        // Automatically request firmware metadata during device interview
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_FIRMWARE_MD_GET), endpoint_node);
    }

    sl_status_t command_class_firmware_update_md::on_firmware_md_get_requested(attribute_store::attribute endpoint_node)
    {
        // Create the FIRMWARE_MD_GET_GROUP and trigger resolution
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(firmware_md_get_group_attributes_t::FIRMWARE_MD_GET_GROUP));
        command_class_firmware_update_md_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_firmware_update_md::on_firmware_update_md_request_get_requested(const command_class_firmware_update_md_types::command_class_firmware_update_md_request_get_payload_t &payload)
    {
        // Create the FIRMWARE_UPDATE_MD_REQUEST_GET_GROUP and set desired values
        attribute_store::attribute endpoint(payload.endpoint_node);
        auto group_node = endpoint.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::FIRMWARE_UPDATE_MD_REQUEST_GET_GROUP));

        // Manufacturer ID (16 bits)
        auto manufacturer_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::manufacturer_id));
        manufacturer_id_node.set_desired<uint16_t>(payload.manufacturer_id);

        // Firmware ID (16 bits)
        auto firmware_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::firmware_id));
        firmware_id_node.set_desired<uint16_t>(payload.firmware_id);

        // Checksum (16 bits)
        auto checksum_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::checksum));
        checksum_node.set_desired<uint16_t>(payload.checksum);

        // Firmware Target (8 bits)
        auto firmware_target_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::firmware_target));
        firmware_target_node.set_desired<uint8_t>(payload.firmware_target);

        // Fragment Size (16 bits)
        auto fragment_size_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::fragment_size));
        fragment_size_node.set_desired<uint16_t>(payload.fragment_size);

        // Activation bit (1 bit via properties1)
        auto activation_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::activation));
        activation_node.set_desired<uint8_t>(payload.activation ? 1 : 0);

        // Hardware Version (8 bits)
        auto hardware_version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::hardware_version));
        hardware_version_node.set_desired<uint8_t>(payload.hardware_version);

        // Trigger the attribute resolver to send the Request Get frame
        command_class_firmware_update_md_core::start_group_resolution(group_node);

        return SL_STATUS_OK;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Firmware Update Meta Data Report Command (0x06) — outgoing
    // FIXME: Attribute store is unable to handle the report because only get and set commands are available.
    //
    // Frame layout (all versions):
    //   [CC 0x7A] [CMD 0x06]
    //   [Properties1: Last(bit7) | Report Number 1 (bits 6-0)]
    //   [Report Number 2]
    //   [Data (fragment_size bytes, or less for the last fragment)]
    //   [CRC MSB] [CRC LSB]   (always appended; device validates over full frame)
    ///////////////////////////////////////////////////////////////////////////
    sl_status_t command_class_firmware_update_md::on_firmware_update_md_report_requested(const command_class_firmware_update_md_types::command_class_firmware_update_md_report_payload_t &payload)
    {
        constexpr uint8_t CC_FIRMWARE_UPDATE_MD         = 0x7A;
        constexpr uint8_t CMD_FIRMWARE_UPDATE_MD_REPORT = 0x06;

        // Build frame bytes
        std::vector<uint8_t> frame;
        frame.reserve(2 + 1 + 1 + payload.data.size() + 2);

        frame.push_back(CC_FIRMWARE_UPDATE_MD);
        frame.push_back(CMD_FIRMWARE_UPDATE_MD_REPORT);

        // Properties1: Last (bit 7) | Report Number 1 high byte (bits 6-0)
        const uint8_t rn_high    = static_cast<uint8_t>((payload.report_number >> 8) & 0x7F);
        const uint8_t properties = (payload.is_last ? 0x80U : 0x00U) | rn_high;
        frame.push_back(properties);

        // Report Number 2 (low byte)
        frame.push_back(static_cast<uint8_t>(payload.report_number & 0xFF));

        // Firmware data
        frame.insert(frame.end(), payload.data.begin(), payload.data.end());

        // CRC16-CCITT over the full frame (excluding the CRC bytes themselves)
        uint16_t crc = zwave_crc16(CRC16_INIT_VALUE, frame.data(), frame.size());
        frame.push_back(static_cast<uint8_t>(crc >> 8));
        frame.push_back(static_cast<uint8_t>(crc & 0xFF));

        // Resolve node_id from the endpoint attribute
        zwave_node_id_t node_id = 0;
        if (attribute_store_network_helper_get_node_id_from_node(payload.endpoint_node, &node_id) != SL_STATUS_OK || node_id == 0) {
            sl_log_error(LOG_TAG.data(), "FirmwareReport: Failed to resolve node_id from endpoint node");
            return SL_STATUS_FAIL;
        }

        zwave_controller_connection_info_t connection_info = {};
        zwave_tx_scheme_get_node_connection_info(node_id, 0, &connection_info);

        zwave_tx_options_t tx_options                  = {};
        constexpr uint8_t number_of_expected_responses = 0;
        constexpr uint32_t discard_timeout_ms          = 10000;
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_RECOMMENDED_TIMING_CRITICAL_PRIORITY + payload.qos_offset, number_of_expected_responses, discard_timeout_ms, &tx_options);
        // Optimization: Skip verify delivery for firmware report frames as they are not expected to be responded to.
        // Without skipping, an additional delay would be introduced (~280ms) for each frame.
        // That means that the socure OTA would be delayed by ~280ms * number of frames, which is significant (30 mins).
        tx_options.skip_s2_verify_delivery = true;

        sl_status_t status = zwave_tx_send_data(&connection_info, static_cast<uint16_t>(frame.size()), frame.data(), &tx_options, nullptr, nullptr, nullptr);

        if (status != SL_STATUS_OK) {
            sl_log_error(LOG_TAG.data(), "FirmwareReport: Failed to send MD Report #%u to node %d (status=0x%04X)", payload.report_number, node_id, status);
        }

        return status;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Firmware Update Meta Data Get Command (0x05)
    //
    // Spec (v1+): Number of Reports (8) + Properties1 [Res(1)|Report Number 1(7)] + Report Number 2 (8)
    // Frame size: 5 bytes across all versions
    ///////////////////////////////////////////////////////////////////////////
    sl_status_t command_class_firmware_update_md::on_firmware_update_md_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        // Number of Reports (8 bits)
        auto number_of_reports_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::number_of_reports));
        frame_generator->add_value(number_of_reports_node, DESIRED_ATTRIBUTE);

        // Properties1: Res (1 bit, bit 7) | Report Number 1 (7 bits, bits 6-0)
        auto report_number_1_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::report_number_1));
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = report_number_1_node, .node_value_state = DESIRED_ATTRIBUTE},
          {.left_shift = 7, .raw_value = 0},  // Res bit, MUST be set to 0
        });

        // Report Number 2 (8 bits)
        auto report_number_2_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_get_group_attributes_t::report_number_2));
        frame_generator->add_value(report_number_2_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Firmware Update Meta Data Request Get Command (0x03)
    //
    // v1: Manufacturer ID (16) + Firmware ID (16) + Checksum (16) = 8 bytes
    // v3: + Firmware Target (8) + Fragment Size (16) = 11 bytes
    // v4: + Properties1 [Reserved(7)|Activation(1)] = 12 bytes
    // v5: + Hardware Version (8) = 13 bytes
    ///////////////////////////////////////////////////////////////////////////
    sl_status_t command_class_firmware_update_md::on_firmware_update_md_request_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        // v1+: Manufacturer ID (16 bits)
        auto manufacturer_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::manufacturer_id));
        frame_generator->add_value(manufacturer_id_node, DESIRED_ATTRIBUTE);

        // v1+: Firmware ID (16 bits)
        auto firmware_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::firmware_id));
        frame_generator->add_value(firmware_id_node, DESIRED_ATTRIBUTE);

        // v1+: Checksum (16 bits)
        auto checksum_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::checksum));
        frame_generator->add_value(checksum_node, DESIRED_ATTRIBUTE);

        // v3+: Firmware Target (8 bits)
        auto firmware_target_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::firmware_target));
        frame_generator->add_value(firmware_target_node, DESIRED_ATTRIBUTE);

        // v3+: Fragment Size (16 bits)
        auto fragment_size_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::fragment_size));
        frame_generator->add_value(fragment_size_node, DESIRED_ATTRIBUTE);

        // v4+: Properties1 [Reserved(7 bits)|Activation(1 bit)]
        auto activation_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::activation));
        frame_generator->add_shifted_values({
          {.left_shift = 0, .node = activation_node, .node_value_state = DESIRED_ATTRIBUTE},
        });

        // v5+: Hardware Version (8 bits)
        auto hardware_version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_request_get_group_attributes_t::hardware_version));
        frame_generator->add_value(hardware_version_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    sl_status_t command_class_firmware_update_md::on_firmware_update_activation_set_requested(const command_class_firmware_update_md_types::command_class_firmware_update_md_activation_set_payload_t &payload)
    {
        attribute_store::attribute endpoint(payload.endpoint_node);
        auto group_node = endpoint.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::FIRMWARE_UPDATE_ACTIVATION_SET_GROUP));

        auto manufacturer_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::manufacturer_id));
        manufacturer_id_node.set_desired<uint16_t>(payload.manufacturer_id);

        auto firmware_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::firmware_id));
        firmware_id_node.set_desired<uint16_t>(payload.firmware_id);

        auto checksum_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::checksum));
        checksum_node.set_desired<uint16_t>(payload.checksum);

        auto firmware_target_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::firmware_target));
        firmware_target_node.set_desired<uint8_t>(payload.firmware_target);

        auto hardware_version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::hardware_version));
        hardware_version_node.set_desired<uint8_t>(payload.hardware_version);

        // Skip supervision for the activation set command as the device will reboot and cannot complete the supervision handshake.
        command_class_firmware_update_md_core::start_group_resolution(group_node, {.skip_supervision = true});

        return SL_STATUS_OK;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Firmware Update Activation Set Command (0x08)
    //
    // Introduced in v4.
    // v4: Manufacturer ID (16) + Firmware ID (16) + Checksum (16) + Firmware Target (8) = 9 bytes
    // v5: + Hardware Version (8) = 10 bytes
    ///////////////////////////////////////////////////////////////////////////
    sl_status_t command_class_firmware_update_md::on_firmware_update_activation_set_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;

        // v4+: Manufacturer ID (16 bits)
        auto manufacturer_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::manufacturer_id));
        frame_generator->add_value(manufacturer_id_node, DESIRED_ATTRIBUTE);

        // v4+: Firmware ID (16 bits)
        auto firmware_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::firmware_id));
        frame_generator->add_value(firmware_id_node, DESIRED_ATTRIBUTE);

        // v4+: Checksum (16 bits)
        auto checksum_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::checksum));
        frame_generator->add_value(checksum_node, DESIRED_ATTRIBUTE);

        // v4+: Firmware Target (8 bits)
        auto firmware_target_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::firmware_target));
        frame_generator->add_value(firmware_target_node, DESIRED_ATTRIBUTE);

        // v5+: Hardware Version (8 bits)
        auto hardware_version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_activation_set_group_attributes_t::hardware_version));
        frame_generator->add_value(hardware_version_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Firmware Update Meta Data Prepare Get Command (0x0A)
    //
    // Introduced in v5.
    // v5: Manufacturer ID (16) + Firmware ID (16) + Firmware Target (8) + Fragment Size (16) + Hardware Version (8) = 10 bytes
    ///////////////////////////////////////////////////////////////////////////
    sl_status_t command_class_firmware_update_md::on_firmware_update_md_prepare_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.get_frame_generator;

        // v5: Manufacturer ID (16 bits)
        auto manufacturer_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_get_group_attributes_t::manufacturer_id));
        frame_generator->add_value(manufacturer_id_node, DESIRED_ATTRIBUTE);

        // v5: Firmware ID (16 bits)
        auto firmware_id_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_get_group_attributes_t::firmware_id));
        frame_generator->add_value(firmware_id_node, DESIRED_ATTRIBUTE);

        // v5: Firmware Target (8 bits)
        auto firmware_target_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_get_group_attributes_t::firmware_target));
        frame_generator->add_value(firmware_target_node, DESIRED_ATTRIBUTE);

        // v5: Fragment Size (16 bits)
        auto fragment_size_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_get_group_attributes_t::fragment_size));
        frame_generator->add_value(fragment_size_node, DESIRED_ATTRIBUTE);

        // v5: Hardware Version (8 bits)
        auto hardware_version_node = group_node.emplace_node(static_cast<attribute_store_type_t>(firmware_update_md_prepare_get_group_attributes_t::hardware_version));
        frame_generator->add_value(hardware_version_node, DESIRED_ATTRIBUTE);

        return frame_generator->generate_frame();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parsed report overrides — fire component_connector events
    ///////////////////////////////////////////////////////////////////////////

    /// Helper to build and fire a component_connector event
    static void fire_fw_update_event(command_class_firmware_update_md_events_t event_id, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t &payload)
    {
        component_connector_firmware_update_md_report_payload_t cc_payload;
        cc_payload.endpoint_node = endpoint;
        cc_payload.attribute_map = payload;

        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(event_id), cc_payload);
    }

    sl_status_t command_class_firmware_update_md::on_firmware_md_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload)
    {
        (void)connection_info;
        fire_fw_update_event(command_class_firmware_update_md_events_t::FIRMWARE_MD_REPORT_PARSED, endpoint, payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_firmware_update_md::on_firmware_update_md_request_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload)
    {
        (void)connection_info;
        fire_fw_update_event(command_class_firmware_update_md_events_t::FIRMWARE_UPDATE_MD_REQUEST_REPORT_PARSED, endpoint, payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_firmware_update_md::on_firmware_update_md_get_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload)
    {
        (void)connection_info;
        fire_fw_update_event(command_class_firmware_update_md_events_t::FIRMWARE_UPDATE_MD_GET_PARSED, endpoint, payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_firmware_update_md::on_firmware_update_md_status_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload)
    {
        (void)connection_info;
        fire_fw_update_event(command_class_firmware_update_md_events_t::FIRMWARE_UPDATE_MD_STATUS_REPORT_PARSED, endpoint, payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_firmware_update_md::on_firmware_update_activation_status_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload)
    {
        (void)connection_info;
        fire_fw_update_event(command_class_firmware_update_md_events_t::FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_PARSED, endpoint, payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_firmware_update_md::on_firmware_update_md_prepare_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_firmware_update_md_attribute_map_t payload)
    {
        (void)connection_info;
        fire_fw_update_event(command_class_firmware_update_md_events_t::FIRMWARE_UPDATE_MD_PREPARE_REPORT_PARSED, endpoint, payload);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class