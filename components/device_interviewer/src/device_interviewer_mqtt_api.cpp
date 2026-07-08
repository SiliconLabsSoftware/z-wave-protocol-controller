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

#include "device_interviewer_mqtt_api.hpp"
#include "log.h"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zpc_attribute_store_network_helper.h"
#include "nlohmann/json.hpp"

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "device_interviewer_mqtt_api";

    DeviceInterviewerMqttApi::DeviceInterviewerMqttApi() {}

    void DeviceInterviewerMqttApi::setup_mqtt_api()
    {
        component_connector connector;
        // Subscribe to FULLY_RESOLVED (not INTERVIEW_DONE) so the MQTT report is
        // published only after every command class on_interview transaction has
        // settled — i.e. when the device is truly ready for clients.
        connector.connect_typed<component_connector_common_events_t, component_connector_interview_done_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED,
                                                                                                                   [](const component_connector_interview_done_payload_t &payload) -> sl_status_t { return zwave_command_class::DeviceInterviewerMqttApi::on_interview_terminated(payload); });
    }

    sl_status_t DeviceInterviewerMqttApi::on_interview_terminated(const component_connector_interview_done_payload_t &payload)
    {
        zwave_node_id_t node_id         = 0;
        zwave_endpoint_id_t endpoint_id = 0;

        sl_status_t status = attribute_store_network_helper_get_zwave_ids_from_node(payload.endpoint_node, &node_id, &endpoint_id);
        if (status != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG.data(), "Interview terminated: failed to get node_id/endpoint_id from endpoint node");
            return status;
        }

        nlohmann::json j;
        j["node_id"]     = node_id;
        j["endpoint_id"] = endpoint_id;
        j["status"]      = payload.status;

        publish_report(MQTT_API_INTERVIEW_TERMINATED_TOPIC, j.dump(), false);

        return SL_STATUS_OK;
    }
}  // namespace zwave_command_class
