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

#include "zwave_command_class_utils.hpp"

#include "attribute_store_defined_attribute_types.h"
#include "zwave_command_class_indices.h"
#include "attribute_store_helper.h"
#include "log.h"
#include "zpc_attribute_store_network_helper.h"
#include "zwave_controller_utils.h"
#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"
#include "zwave_network_management.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#define LOG_TAG "zwave_command_class_utils"

namespace zwave_command_class
{
    attribute_store_node_t command_class_utils::get_endpoint_node(const zwave_controller_connection_info_t *connection_info)
    {
        if (nullptr == connection_info) {
            return ATTRIBUTE_STORE_INVALID_NODE;
        }
        zwave_home_id_t home_id = zwave_network_management_get_home_id();
        return attribute_store_network_helper_get_endpoint_node(home_id, connection_info->remote.node_id, connection_info->remote.endpoint_id);
    }

    sl_status_t command_class_utils::send_report(const zwave_controller_connection_info_t *connection_info, uint16_t report_size, const uint8_t *report_data)
    {
        assert(report_size > 0);

        zwave_tx_options_t tx_options {};
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_RECOMMENDED_GET_ANSWER_PRIORITY, 0, 0, &tx_options);

        return zwave_tx_send_data(connection_info, report_size, report_data, &tx_options, nullptr, nullptr, nullptr);
    }

    bool command_class_utils::is_using_zpc_highest_security_class(const zwave_controller_connection_info_t *connection)
    {
        if (connection == nullptr) {
            sl_log_warning(LOG_TAG, "NULL connection info object. Returning false");
            return false;
        }
        zwave_keyset_t zpc_keyset {};
        zwave_tx_scheme_get_zpc_granted_keys(&zpc_keyset);
        zwave_controller_encapsulation_scheme_t zpc_scheme = zwave_controller_get_highest_encapsulation(zpc_keyset);
        return connection->encapsulation == zpc_scheme;
    }

    clock_time_t command_class_utils::zwave_duration_to_time(uint8_t zwave_duration)
    {
        clock_time_t time = 0;
        if (zwave_duration <= 0x7F) {
            time = zwave_duration * CLOCK_SECOND;
        } else if (zwave_duration <= 0xFD) {
            time = (zwave_duration - 127) * CLOCK_SECOND * 60;
        }
        return time;
    }

    std::vector<uint8_t> command_class_utils::get_normal_command_classes(const std::vector<uint8_t> &command_classes)
    {
        std::vector<uint8_t> normal_command_classes;
        for (auto it = command_classes.begin(); it != command_classes.end(); ++it) {
            if (*it == COMMAND_CLASS_CONTROL_MARK) {
                break;
            }
            if (*it < EXTENDED_COMMAND_CLASS_IDENTIFIER_START) {
                normal_command_classes.push_back(*it);
            } else if (it + 1 != command_classes.end()) {
                ++it;
            }
        }

        return normal_command_classes;
    }

    std::vector<uint16_t> command_class_utils::get_extended_command_classes(const std::vector<uint8_t> &command_classes)
    {
        std::vector<uint16_t> extended_command_classes;
        for (auto it = command_classes.begin(); it != command_classes.end(); ++it) {
            if (*it == COMMAND_CLASS_CONTROL_MARK) {
                break;
            }
            if (*it >= EXTENDED_COMMAND_CLASS_IDENTIFIER_START && it + 1 != command_classes.end()) {
                const uint16_t extended_command_class = static_cast<uint16_t>((*it << 8) | *(it + 1));
                extended_command_classes.push_back(extended_command_class);
                ++it;
            }
        }

        return extended_command_classes;
    }

    sl_status_t command_class_utils::get_node_dsk(zwave_node_id_t node_id, zwave_dsk_t dsk)
    {
        std::memset(dsk, 0, sizeof(zwave_dsk_t));

        attribute_store_node_t node_id_node = attribute_store_network_helper_get_zwave_node_id_node(node_id);
        if (node_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
            return SL_STATUS_NOT_FOUND;
        }
        attribute_store_node_t dsk_node = attribute_store_get_first_child_by_type(node_id_node, ATTRIBUTE_S2_DSK);
        if (dsk_node == ATTRIBUTE_STORE_INVALID_NODE) {
            return SL_STATUS_NOT_FOUND;
        }
        return attribute_store_read_value(dsk_node, REPORTED_ATTRIBUTE, dsk, sizeof(zwave_dsk_t));
    }

    bool command_class_utils::is_command_class_in_s2_s0_nif_lists(uint8_t command_class, const std::vector<uint8_t> &s2_supported_command_classes, const std::vector<uint8_t> &s0_supported_command_classes, const std::vector<uint8_t> &node_information_command_class_list)
    {
        const std::vector<uint8_t> filtered_s2  = get_normal_command_classes(s2_supported_command_classes);
        const std::vector<uint8_t> filtered_s0  = get_normal_command_classes(s0_supported_command_classes);
        const std::vector<uint8_t> filtered_nif = get_normal_command_classes(node_information_command_class_list);

        return std::find(filtered_s2.begin(), filtered_s2.end(), command_class) != filtered_s2.end() || std::find(filtered_s0.begin(), filtered_s0.end(), command_class) != filtered_s0.end() || std::find(filtered_nif.begin(), filtered_nif.end(), command_class) != filtered_nif.end();
    }

    bool command_class_utils::is_version_command_class_in_s2_s0_nif_lists(const std::vector<uint8_t> &s2_supported_command_classes, const std::vector<uint8_t> &s0_supported_command_classes, const std::vector<uint8_t> &node_information_command_class_list)
    {
        return is_command_class_in_s2_s0_nif_lists(0x86, s2_supported_command_classes, s0_supported_command_classes, node_information_command_class_list);
    }
}  // namespace zwave_command_class
