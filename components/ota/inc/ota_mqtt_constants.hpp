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

#ifndef OTA_MQTT_CONSTANTS_HPP
#define OTA_MQTT_CONSTANTS_HPP

#include <string_view>

namespace ota::mqtt_constants
{

    namespace status
    {
        constexpr std::string_view OK                     = "ok";
        constexpr std::string_view ERROR                  = "error";
        constexpr std::string_view ACCEPTED               = "accepted";
        constexpr std::string_view REJECTED               = "rejected";
        constexpr std::string_view SUCCESS                = "success";
        constexpr std::string_view FAILED                 = "failed";
        constexpr std::string_view WAITING_FOR_ACTIVATION = "waiting_for_activation";
        constexpr std::string_view STORED_NO_RESTART      = "stored_no_restart";
        constexpr std::string_view ABORTED                = "aborted";
    }  // namespace status

    namespace reason
    {
        constexpr std::string_view INVALID_NODE               = "invalid_node";
        constexpr std::string_view UNSUPPORTED_FEATURE        = "unsupported_feature";
        constexpr std::string_view FIRMWARE_NOT_UPGRADABLE    = "firmware_not_upgradable";
        constexpr std::string_view IMAGE_NOT_FOUND            = "image_not_found";
        constexpr std::string_view INVALID_COMBINATION        = "invalid_combination";
        constexpr std::string_view REQUIRES_AUTHENTICATION    = "requires_authentication";
        constexpr std::string_view INVALID_FRAGMENT_SIZE      = "invalid_fragment_size";
        constexpr std::string_view NOT_DOWNLOADABLE           = "not_downloadable";
        constexpr std::string_view INVALID_HARDWARE_VERSION   = "invalid_hardware_version";
        constexpr std::string_view UNKNOWN                    = "unknown";
        constexpr std::string_view UPDATE_ALREADY_IN_PROGRESS = "update_already_in_progress";
    }  // namespace reason

    namespace key
    {
        constexpr std::string_view NODE_ID             = "node_id";
        constexpr std::string_view IMAGE_NAME          = "image_name";
        constexpr std::string_view STATUS              = "status";
        constexpr std::string_view REASON              = "reason";
        constexpr std::string_view DATA                = "data";
        constexpr std::string_view IMAGES              = "images";
        constexpr std::string_view WAIT_FOR_ACTIVATION = "wait_for_activation";
        constexpr std::string_view IMAGE_SIZE          = "image_size";
        constexpr std::string_view CURRENT_SENT        = "current_sent";
        constexpr std::string_view PERCENTAGE          = "percentage";
        constexpr std::string_view WAITTIME            = "waittime";
        constexpr std::string_view STATUS_CODE         = "status_code";
    }  // namespace key

}  // namespace ota::mqtt_constants

#endif  // OTA_MQTT_CONSTANTS_HPP
