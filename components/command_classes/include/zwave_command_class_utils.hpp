/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

/**
 * @file zwave_command_class_utils.hpp
 * @brief Stateless utilities shared by Z-Wave command class handlers.
 *
 * Helpers cover endpoint attribute resolution, outbound reports, checks against
 * the ZPC granted security class, Z-Wave duration
 * encoding, and splitting mixed 8-bit / extended command class ID lists.
 */

#ifndef ZWAVE_COMMAND_CLASS_UTILS_HPP
#define ZWAVE_COMMAND_CLASS_UTILS_HPP

#include <vector>

#include "attribute_store.h"
#include "clock_platform.h"
#include "sl_status.h"
#include "zwave_controller_connection_info.h"
#include "zwave_controller_types.h"

namespace zwave_command_class
{
    /**
     * @brief Static helpers used across command class implementations (no instance state).
     */
    class command_class_utils
    {
        public:
            /**
             * @brief Resolve the attribute store endpoint node for a connection.
             * @param connection_info Remote node and endpoint from the frame context.
             * @return Endpoint node, or ATTRIBUTE_STORE_INVALID_NODE if @p connection_info is null.
             */
            static attribute_store_node_t get_endpoint_node(const zwave_controller_connection_info_t *connection_info);

            /**
             * @brief Send a command class report to the peer using recommended answer QoS.
             * @param connection_info Destination derived from the inbound context.
             * @param report_size Length of @p report_data (must be non-zero; asserted in the implementation).
             * @param report_data Payload bytes.
             * @return Status from the Z-Wave TX layer.
             */
            static sl_status_t send_report(const zwave_controller_connection_info_t *connection_info, uint16_t report_size, const uint8_t *report_data);

            /**
             * @brief True if the frame uses the same encapsulation as the ZPC's highest granted scheme.
             * @param connection Inbound connection info (encapsulation and keys).
             * @return False if @p connection is null.
             */
            static bool is_using_zpc_highest_security_class(const zwave_controller_connection_info_t *connection);

            /**
             * @brief Map a Z-Wave duration byte to a Contiki-style clock interval.
             * @param zwave_duration 0x00–0x7F: seconds; 0x80–0xFD: minutes (per Z-Wave encoding); other values yield 0.
             */
            static clock_time_t zwave_duration_to_time(uint8_t zwave_duration);

            /**
             * @brief Extract 8-bit command class IDs from a list that may contain extended-ID markers.
             * @param command_classes Raw list (extended entries use the high marker and a following byte).
             */
            static std::vector<uint8_t> get_normal_command_classes(const std::vector<uint8_t> &command_classes);

            /**
             * @brief Extract 16-bit extended command class IDs from a mixed list.
             * @param command_classes Same encoding as NIF / supported CC lists with extended IDs.
             */
            static std::vector<uint16_t> get_extended_command_classes(const std::vector<uint8_t> &command_classes);

            /**
             * @brief Retrieve the S2 DSK for a node from the attribute store.
             * @param node_id Z-Wave node ID to look up.
             * @param[out] dsk Buffer filled with the DSK bytes, or zeroed if unavailable.
             * @return SL_STATUS_OK on success, SL_STATUS_NOT_FOUND if the node or DSK is absent.
             */
            static sl_status_t get_node_dsk(zwave_node_id_t node_id, zwave_dsk_t dsk);

            /**
             * @brief True if Version (0x86) appears in any of the normal (non-extended) CCs
             *        from S2 supported, S0 supported, and NIF lists (per device interviewer merge rule).
             */
            static bool is_version_command_class_in_s2_s0_nif_lists(const std::vector<uint8_t> &s2_supported_command_classes, const std::vector<uint8_t> &s0_supported_command_classes, const std::vector<uint8_t> &node_information_command_class_list);
    };
}  // namespace zwave_command_class

#endif  // ZWAVE_COMMAND_CLASS_UTILS_HPP
