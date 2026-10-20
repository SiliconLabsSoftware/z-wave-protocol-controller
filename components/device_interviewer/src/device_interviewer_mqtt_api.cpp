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
#include "zpc_attribute_store_network_helper.h"
#include "device_interviewer_types.hpp"
#include "device_interviewer_events.hpp"
#include "command_class_version_types.hpp"
#include "command_class_version_events.hpp"
#include "nlohmann/json.hpp"
#include <optional>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "device_interviewer_mqtt_api";

    template<typename T> static nlohmann::json json_optional_value(const std::optional<T> &value)
    {
        return value.has_value() ? nlohmann::json(value.value()) : nlohmann::json(nullptr);
    }

    static nlohmann::json json_optional_command_class_list(const std::optional<std::vector<uint8_t>> &value)
    {
        return value.has_value() ? nlohmann::json(value.value()) : nlohmann::json::array();
    }

    DeviceInterviewerMqttApi::DeviceInterviewerMqttApi() {}

    void DeviceInterviewerMqttApi::setup_mqtt_api() {}

    sl_status_t DeviceInterviewerMqttApi::publish_interview_terminated_report(const component_connector_interview_done_payload_t &payload)
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

        if (endpoint_id == 0) {
            const attribute_store::attribute endpoint(payload.endpoint_node);
            const attribute_store::attribute device_node = endpoint.parent();

            device_interviewer_get_node_information_payload_t request {};
            request.device_node = device_node;
            component_connector connector;
            auto future                            = connector.fire_event_async<device_interviewer_get_node_information_payload_t, device_interviewer_get_node_information_payload_t>(static_cast<uint32_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_GET_NODE_INFORMATION), request);
            auto [information_status, information] = future.get();
            if (information_status != SL_STATUS_OK) {
                sl_log_warning(LOG_TAG.data(), "Interview terminated: failed to get node information for node_id %d", node_id);
            }
            nlohmann::json node_information;
            node_information["node_id"]               = node_id;
            node_information["listening_protocol"]    = json_optional_value(information.listening_protocol);
            node_information["optional_protocol"]     = json_optional_value(information.optional_protocol);
            node_information["basic_device_class"]    = json_optional_value(information.basic_device_class);
            node_information["generic_device_class"]  = json_optional_value(information.generic_device_class);
            node_information["specific_device_class"] = json_optional_value(information.specific_device_class);
            node_information["command_class_list"]    = json_optional_command_class_list(information.command_class_list);
            node_information["s2_command_class_list"] = json_optional_command_class_list(information.s2_command_class_list);
            node_information["s0_command_class_list"] = json_optional_command_class_list(information.s0_command_class_list);
            node_information["inclusion_protocol"]    = json_optional_value(information.inclusion_protocol);
            node_information["granted_keys"]          = json_optional_value(information.granted_keys);
            j["node_information"]                     = std::move(node_information);

            command_class_version_types::command_class_get_version_report_payload_t version_request;
            version_request.device_node = device_node;
            auto version_future
              = connector.fire_event_async<command_class_version_types::command_class_get_version_report_payload_t, command_class_version_types::command_class_get_version_report_payload_t>(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_GET_VERSION_REPORT), version_request);
            auto [version_status, version] = version_future.get();
            if (version_status != SL_STATUS_OK) {
                sl_log_warning(LOG_TAG.data(), "Interview terminated: failed to get version report for node_id %d", node_id);
            }
            nlohmann::json version_report;
            version_report["z_wave_library_type"]         = json_optional_value(version.z_wave_library_type);
            version_report["z_wave_protocol_version"]     = json_optional_value(version.z_wave_protocol_version);
            version_report["z_wave_protocol_sub_version"] = json_optional_value(version.z_wave_protocol_sub_version);
            version_report["firmware_0_version"]          = json_optional_value(version.firmware_0_version);
            version_report["firmware_0_sub_version"]      = json_optional_value(version.firmware_0_sub_version);
            version_report["hardware_version"]            = json_optional_value(version.hardware_version);
            version_report["number_of_firmware_targets"]  = json_optional_value(version.number_of_firmware_targets);
            j["version_report"]                           = std::move(version_report);
        }

        publish_report(MQTT_API_INTERVIEW_TERMINATED_TOPIC, j.dump(), false);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
