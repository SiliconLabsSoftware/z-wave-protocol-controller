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

#ifndef COMMAND_CLASS_FIRMWARE_UPDATE_MD_TYPES_H
#define COMMAND_CLASS_FIRMWARE_UPDATE_MD_TYPES_H

#include "command_class_firmware_update_md_generated_types.hpp"
#include "attribute_store.h"
#include "attribute.hpp"

#include <cstdint>
#include <vector>

namespace zwave_command_class
{
    namespace command_class_firmware_update_md_types
    {
        /**
         * @brief Generic payload for component_connector events fired by
         * the Firmware Update MD Command Class after report parsing.
         *
         * Carries the endpoint node so the consumer can look up node_id,
         * and the parsed attribute_map with all report fields.
         */
        struct component_connector_firmware_update_md_report_payload_t {
                attribute_store::attribute endpoint_node;
                command_class_firmware_update_md_attribute_map_t attribute_map;
        };

        /**
         * @brief Payload for COMMAND_CLASS_FIRMWARE_UPDATE_MD_REQUEST_GET event.
         *
         * Fired by components to request a Firmware Update MD Request Get command.
         * The command class will handle all attribute store operations internally.
         */
        struct command_class_firmware_update_md_request_get_payload_t {
                attribute_store::attribute endpoint_node;
                uint16_t manufacturer_id;
                uint16_t firmware_id;
                uint16_t checksum;
                uint8_t firmware_target;
                uint16_t fragment_size;
                bool activation;
                uint8_t hardware_version;
        };

        /**
         * @brief Payload for COMMAND_CLASS_FIRMWARE_UPDATE_MD_REPORT event.
         *
         * Fired by components to send a single Firmware Update MD Report frame
         * (firmware data chunk) to the device. The command class builds the
         * Z-Wave frame and transmits it via zwave_tx_send_data.
         */
        /**
         * @brief Payload for COMMAND_CLASS_FIRMWARE_UPDATE_MD_ACTIVATION_SET event.
         *
         * Fired by components to trigger a Firmware Update Activation Set command.
         * The command class will set the DESIRED attributes and trigger resolution.
         */
        struct command_class_firmware_update_md_activation_set_payload_t {
                attribute_store::attribute endpoint_node;
                uint16_t manufacturer_id;
                uint16_t firmware_id;
                uint16_t checksum;
                uint8_t firmware_target;
                uint8_t hardware_version;
        };

        struct command_class_firmware_update_md_report_payload_t {
                attribute_store::attribute endpoint_node;
                uint16_t report_number;
                bool is_last;
                std::vector<uint8_t> data;
                uint32_t qos_offset;  // Higher = higher TX priority; used to order batch frames
        };
    }  // namespace command_class_firmware_update_md_types
}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_FIRMWARE_UPDATE_MD_TYPES_H
