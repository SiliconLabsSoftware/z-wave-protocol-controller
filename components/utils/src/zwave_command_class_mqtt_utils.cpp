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

#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{
    mqtt_payload_parser::mqtt_payload_parser(const std::string &payload, const char *log_tag) : m_log_tag(log_tag), m_status(SL_STATUS_OK)
    {
        try {
            m_payload = nlohmann::json::parse(payload);
        } catch (const std::exception &e) {
            sl_log_error(m_log_tag, "Failed to parse MQTT payload: %s", e.what());
            m_status = SL_STATUS_FAIL;
        }
    }
}  // namespace zwave_command_class
