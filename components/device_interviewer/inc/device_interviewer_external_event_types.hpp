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

#ifndef DEVICE_INTERVIEWER_EVENT_TYPES_H
#define DEVICE_INTERVIEWER_EVENT_TYPES_H

#include "attribute.hpp"
#include <any>
#include <optional>

namespace zwave_command_class
{
    // Event types for device interviewer (defined at namespace level for use by state machine)
    enum class device_interviewer_external_event_t {
        NODE_DELETED,
        FACTORY_RESET,
        S2_COMMANDS_SUPPORTED_REPORT,
        /// Commands Supported Get enqueue/air TX failed (retry or fail interview).
        S2_COMMANDS_SUPPORTED_GET_TX_FAILED,
        S0_COMMANDS_SUPPORTED_REPORT,
        NODE_INFORMATION_RECEIVED,
        VERSION_REPORT_RECEIVED,
        VERSION_CAPABILITIES_REPORT_RECEIVED,
        VERSION_ZWAVE_SOFTWARE_REPORT_RECEIVED,
        VERSION_CC_GET_REQUESTED,
        ZWAVEPLUS_INFO_REPORT_RECEIVED,
        WAKE_UP_CAPABILITIES_REPORT_RECEIVED,
        /// Wake Up Interval Set attribute resolution finished (interview path); Interval Get is queued next.
        WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED,
        WAKE_UP_INTERVAL_REPORT_RECEIVED,
        MULTI_CHANNEL_END_POINT_REPORT_RECEIVED,
        MULTI_CHANNEL_END_POINT_FIND_REPORT_RECEIVED,
        MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT_RECEIVED,
        MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED,
        ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED,
        ASSOCIATION_REPORT_RECEIVED,
        MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED,
        ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT_RECEIVED,
        ASSOCIATION_GRP_INFO_GROUP_INFO_REPORT_RECEIVED,
        ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT_RECEIVED,
    };

    struct device_interviewer_external_event_data {
            device_interviewer_external_event_t event;
            std::any payload;
            std::optional<attribute_store::attribute> device_endpoint_node;
    };

    /**
     * @brief Return a human-readable name for an external event (for logging).
     */
    inline const char *to_string(device_interviewer_external_event_t e)
    {
        switch (e) {
            case device_interviewer_external_event_t::NODE_DELETED:
                return "NODE_DELETED";
            case device_interviewer_external_event_t::FACTORY_RESET:
                return "FACTORY_RESET";
            case device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_REPORT:
                return "S2_COMMANDS_SUPPORTED_REPORT";
            case device_interviewer_external_event_t::S2_COMMANDS_SUPPORTED_GET_TX_FAILED:
                return "S2_COMMANDS_SUPPORTED_GET_TX_FAILED";
            case device_interviewer_external_event_t::S0_COMMANDS_SUPPORTED_REPORT:
                return "S0_COMMANDS_SUPPORTED_REPORT";
            case device_interviewer_external_event_t::NODE_INFORMATION_RECEIVED:
                return "NODE_INFORMATION_RECEIVED";
            case device_interviewer_external_event_t::VERSION_REPORT_RECEIVED:
                return "VERSION_REPORT_RECEIVED";
            case device_interviewer_external_event_t::VERSION_CAPABILITIES_REPORT_RECEIVED:
                return "VERSION_CAPABILITIES_REPORT_RECEIVED";
            case device_interviewer_external_event_t::VERSION_ZWAVE_SOFTWARE_REPORT_RECEIVED:
                return "VERSION_ZWAVE_SOFTWARE_REPORT_RECEIVED";
            case device_interviewer_external_event_t::VERSION_CC_GET_REQUESTED:
                return "VERSION_CC_GET_REQUESTED";
            case device_interviewer_external_event_t::ZWAVEPLUS_INFO_REPORT_RECEIVED:
                return "ZWAVEPLUS_INFO_REPORT_RECEIVED";
            case device_interviewer_external_event_t::WAKE_UP_CAPABILITIES_REPORT_RECEIVED:
                return "WAKE_UP_CAPABILITIES_REPORT_RECEIVED";
            case device_interviewer_external_event_t::WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED:
                return "WAKE_UP_INTERVAL_SET_RESOLUTION_COMPLETED";
            case device_interviewer_external_event_t::WAKE_UP_INTERVAL_REPORT_RECEIVED:
                return "WAKE_UP_INTERVAL_REPORT_RECEIVED";
            case device_interviewer_external_event_t::MULTI_CHANNEL_END_POINT_REPORT_RECEIVED:
                return "MULTI_CHANNEL_END_POINT_REPORT_RECEIVED";
            case device_interviewer_external_event_t::MULTI_CHANNEL_END_POINT_FIND_REPORT_RECEIVED:
                return "MULTI_CHANNEL_END_POINT_FIND_REPORT_RECEIVED";
            case device_interviewer_external_event_t::MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT_RECEIVED:
                return "MULTI_CHANNEL_COMMANDS_CAPABILITY_REPORT_RECEIVED";
            case device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED:
                return "MULTI_CHANNEL_ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED";
            case device_interviewer_external_event_t::ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED:
                return "ASSOCIATION_SUPPORTED_GROUPINGS_REPORT_RECEIVED";
            case device_interviewer_external_event_t::ASSOCIATION_REPORT_RECEIVED:
                return "ASSOCIATION_REPORT_RECEIVED";
            case device_interviewer_external_event_t::MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED:
                return "MULTI_CHANNEL_ASSOCIATION_REPORT_RECEIVED";
            case device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT_RECEIVED:
                return "ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT_RECEIVED";
            case device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_INFO_REPORT_RECEIVED:
                return "ASSOCIATION_GRP_INFO_GROUP_INFO_REPORT_RECEIVED";
            case device_interviewer_external_event_t::ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT_RECEIVED:
                return "ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT_RECEIVED";
            default:
                return "UNKNOWN_EVENT";
        }
    }

}  // namespace zwave_command_class

#endif  // DEVICE_INTERVIEWER_EVENT_TYPES_H
