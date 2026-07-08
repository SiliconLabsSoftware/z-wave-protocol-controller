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

#ifndef OTA_EXTERNAL_EVENT_TYPES_HPP
#define OTA_EXTERNAL_EVENT_TYPES_HPP

#include <any>
#include <cstdint>

#include "zwave_node_id_definitions.h"

namespace ota
{
    /**
     * @brief External event kinds for the OTA state machine (parallel to
     *        device_interviewer_external_event_t).
     */
    enum class ota_external_event_t {
        // MQTT-originated events
        MQTT_START_UPLOAD,
        MQTT_ABORT,
        MQTT_ACTIVATE,
        MQTT_PROGRESS_REQUEST,
        // Z-Wave report events
        FIRMWARE_MD_REPORT_RECEIVED,
        FIRMWARE_UPDATE_MD_REQUEST_REPORT_RECEIVED,
        FIRMWARE_UPDATE_MD_GET_RECEIVED,
        FIRMWARE_UPDATE_MD_STATUS_REPORT_RECEIVED,
        FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT,
    };

    /**
     * @brief Event payload delivered to the OTA state machine (parallel to
     *        device_interviewer_external_event_data).
     */
    struct ota_external_event_data {
            ota_external_event_t event;
            zwave_node_id_t node_id = 0;
            std::any payload;
    };

    /**
     * @brief Human-readable name for logging (parallel to device interviewer).
     */
    inline const char *to_string(ota_external_event_t e)
    {
        switch (e) {
            case ota_external_event_t::MQTT_START_UPLOAD:
                return "MQTT_START_UPLOAD";
            case ota_external_event_t::MQTT_ABORT:
                return "MQTT_ABORT";
            case ota_external_event_t::MQTT_ACTIVATE:
                return "MQTT_ACTIVATE";
            case ota_external_event_t::MQTT_PROGRESS_REQUEST:
                return "MQTT_PROGRESS_REQUEST";
            case ota_external_event_t::FIRMWARE_MD_REPORT_RECEIVED:
                return "FIRMWARE_MD_REPORT_RECEIVED";
            case ota_external_event_t::FIRMWARE_UPDATE_MD_REQUEST_REPORT_RECEIVED:
                return "FIRMWARE_UPDATE_MD_REQUEST_REPORT_RECEIVED";
            case ota_external_event_t::FIRMWARE_UPDATE_MD_GET_RECEIVED:
                return "FIRMWARE_UPDATE_MD_GET_RECEIVED";
            case ota_external_event_t::FIRMWARE_UPDATE_MD_STATUS_REPORT_RECEIVED:
                return "FIRMWARE_UPDATE_MD_STATUS_REPORT_RECEIVED";
            case ota_external_event_t::FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT:
                return "FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT";
        }
        return "UNKNOWN";
    }

}  // namespace ota

#endif  // OTA_EXTERNAL_EVENT_TYPES_HPP
