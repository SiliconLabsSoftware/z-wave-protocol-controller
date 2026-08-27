/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "network_management_mqtt_api.hpp"
#include "log.h"
#include "sl_status.h"
#include "zwave_network_management.h"
#include "zwave_controller.h"
#include "nlohmann/json.hpp"
#include "fmt/format.h"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "attribute_store.h"
#include "attribute_store_helper.h"
#include "attribute_store_defined_attribute_types.h"
#include "attribute.hpp"
#include "device_interviewer_types.hpp"
#include "device_interviewer_events.hpp"
#include "command_class_version_types.hpp"
#include "command_class_version_events.hpp"
#include "zwave_node_id_definitions.h"
#include "zwave_network_management_remove_failed_report.h"
#include <any>
#include <functional>
#include <cassert>
#include "zwave_utils.h"
#include "zwapi_protocol_controller.h"
#include "utils.hpp"
#include "network_monitor_attribute_store.hpp"
#include "network_monitor_network_status.h"

extern uint8_t node_reported_dsk[16];

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "network_management_mqtt_api";

    template<typename T> static nlohmann::json json_optional_value(const std::optional<T> &value)
    {
        return value.has_value() ? nlohmann::json(value.value()) : nlohmann::json(nullptr);
    }

    static nlohmann::json json_optional_command_class_list(const std::optional<std::vector<uint8_t>> &value)
    {
        return value.has_value() ? nlohmann::json(value.value()) : nlohmann::json::array();
    }

    static std::string_view nm_activity_to_string(zwave_network_management_activity_t activity)
    {
        switch (activity) {
            case ZWAVE_NETWORK_MANAGEMENT_ACTIVITY_INCLUSION_ONGOING:
                return "inclusion";
            case ZWAVE_NETWORK_MANAGEMENT_ACTIVITY_EXCLUSION_ONGOING:
                return "exclusion";
            case ZWAVE_NETWORK_MANAGEMENT_ACTIVITY_LEARNING_ONGOING:
                return "learning";
            case ZWAVE_NETWORK_MANAGEMENT_ACTIVITY_INTERNAL_ONGOING:
                return "internal";
            case ZWAVE_NETWORK_MANAGEMENT_ACTIVITY_IDLE:
                return "idle";
            default:
                return "unknown";
        }
    }

    void NetworkManagementMqttApi::setup_mqtt_api()
    {
        // Connect to component_connector events
        component_connector connector;
        connector.connect_typed<component_connector_common_events_t, component_connector_node_added_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_ADDED,
                                                                                                               [](const component_connector_node_added_payload_t &payload) -> sl_status_t { return zwave_command_class::NetworkManagementMqttApi::on_node_added(payload); });

        connector.connect_typed<component_connector_common_events_t, component_connector_node_deleted_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_DELETED,
                                                                                                                 [](const component_connector_node_deleted_payload_t &payload) -> sl_status_t { return zwave_command_class::NetworkManagementMqttApi::on_node_deleted(payload); });

        connector.connect_typed<component_connector_common_events_t, component_connector_node_remove_failed_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_FAILED_NODE_DELETED,
                                                                                                                       [](const component_connector_node_remove_failed_payload_t &payload) -> sl_status_t { return zwave_command_class::NetworkManagementMqttApi::on_failed_node_deleted(payload); });

        connector.connect_typed<component_connector_common_events_t, component_connector_factory_reset_complete_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_FACTORY_RESET_COMPLETE, [](const component_connector_factory_reset_complete_payload_t &payload) -> sl_status_t {
            return zwave_command_class::NetworkManagementMqttApi::on_factory_reset_complete(payload);
        });

        // Network related commands
        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_ADD_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_node_add(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_ADD_ABORT_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_management_abort(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_REMOVE_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_node_remove(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_REMOVE_ABORT_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_management_abort(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_DSK_ACCEPT_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_dsk_accept(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_GRANT_KEYS_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_grant_keys(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_LIST_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_node_list(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_PROPERTIES_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_node_properties(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_FACTORY_RESET_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_factory_reset(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NLS_ENABLE_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_nls_enable(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NLS_STATE_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_nls_state(topic, message); });

        subscribe_topic(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_REMOVE_FAILED_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::NetworkManagementMqttApi::on_network_node_remove_failed(topic, message); });
    }

    void NetworkManagementMqttApi::publish_requested_keys(bool csa, zwave_keyset_t keys)
    {
        nlohmann::json payload;
        payload["Keys"] = fmt::format("0x{:x}", keys);
        payload["CSA"]  = csa;
        publish_report(MQTT_API_NETWORK_REQUESTED_KEYS_REPORT_TOPIC, payload.dump(), false);
    }

    void NetworkManagementMqttApi::publish_requested_dsk(uint8_t input_length, const std::string &dsk_str)
    {
        nlohmann::json payload;
        payload["DSK"] = dsk_str;
        publish_report(MQTT_API_NETWORK_REQUESTED_DSK_REPORT_TOPIC, payload.dump(), false);
    }

    sl_status_t NetworkManagementMqttApi::on_factory_reset_complete(const component_connector_factory_reset_complete_payload_t &payload)
    {
        nlohmann::json report;
        report["status"]  = "ready";
        report["home_id"] = fmt::format("{:08X}", payload.home_id);
        publish_report(MQTT_API_NETWORK_FACTORY_RESET_REPORT_TOPIC, report.dump(), false, false);
        return SL_STATUS_OK;
    }

    sl_status_t NetworkManagementMqttApi::on_failed_node_deleted(const component_connector_node_remove_failed_payload_t &payload)
    {
        nlohmann::json report;
        report["node_id"] = payload.node_id;
        report["status"]  = (payload.reason == REMOVE_FAILED_STATUS_OPERATION_SUCCESSFUL) ? "ok" : "fail";
        report["reason"]  = payload.reason;
        publish_report(MQTT_API_NETWORK_NODE_REMOVE_FAILED_REPORT_TOPIC, report.dump(), false);
        return SL_STATUS_OK;
    }

    sl_status_t NetworkManagementMqttApi::on_node_deleted(const component_connector_node_deleted_payload_t &payload)
    {
        nlohmann::json report;
        report["node_id"] = payload.node_id;

        char dsk_str[DSK_STR_LEN] = {};
        if (Utils::convert_dsk_to_dsk_str(payload.dsk, dsk_str, sizeof(dsk_str)) == SL_STATUS_OK) {
            report["dsk"] = dsk_str;
        }

        report["status"] = MQTT_STATUS_SUCCESS;
        publish_report(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_REMOVE_REPORT_TOPIC, report.dump(), false);
        return SL_STATUS_OK;
    }

    sl_status_t NetworkManagementMqttApi::on_node_added(const component_connector_node_added_payload_t &payload)
    {
        nlohmann::json report;
        report["node_id"] = payload.node_id;

        char dsk_str[DSK_STR_LEN] = {};
        if (Utils::convert_dsk_to_dsk_str(payload.dsk, dsk_str, sizeof(dsk_str)) == SL_STATUS_OK) {
            report["dsk"] = dsk_str;
        }

        const bool security_failed = (payload.status != SL_STATUS_OK) || (payload.kex_fail_type != ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE);
        if (security_failed) {
            report["status"] = MQTT_STATUS_FAIL;
            report["reason"] = MQTT_REASON_NODE_ADD_SECURITY_FAIL;
        } else {
            report["status"] = MQTT_STATUS_SUCCESS;
        }
        publish_report(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_ADD_REPORT_TOPIC, report.dump(), false);
        return SL_STATUS_OK;
    }

    void NetworkManagementMqttApi::on_network_node_add(const std::string &topic, const std::string &message)
    {
        (void)topic;
        (void)message;
        nlohmann::json report;
        if (zwave_controller_is_reset_ongoing()) {
            // `zwave_controller_is_reset_ongoing` is used in favor of
            // `zwave_network_management_get_state() == NM_IDLE` because it is possible that
            // on_network_node_add is called after factory reset has actually started but
            // the network management state is not yet updated.
            sl_log_warning(LOG_TAG.data(), "Reset ongoing, rejecting request");
            report["reason"] = static_cast<sl_status_t>(zpc_status_t::FACTORY_RESET_ONGOING);
            report["status"] = MQTT_STATUS_FAIL;
            publish_report(MQTT_API_NETWORK_NODE_ADD_REPORT_TOPIC, report.dump(), false);
        } else {
            const zwave_network_management_state_t nm_state = zwave_network_management_get_state();
            if (nm_state != NM_IDLE) {
                sl_log_warning(LOG_TAG.data(), "Network management busy, rejecting request");
                report["reason"]   = static_cast<sl_status_t>(zpc_status_t::NETWORK_MANAGEMENT_BUSY);
                report["status"]   = MQTT_STATUS_FAIL;
                report["activity"] = nm_activity_to_string(zwave_network_management_get_activity(nm_state));  // Temporary field: will be replaced by another API in the future.
                publish_report(MQTT_API_NETWORK_NODE_ADD_REPORT_TOPIC, report.dump(), false);
            } else {
                sl_status_t status = zwave_network_management_add_node();
                if (status != SL_STATUS_OK) {
                    sl_log_error(LOG_TAG.data(), "Failed to add node: %d", status);
                }
            }
        }
    }

    void NetworkManagementMqttApi::on_network_management_abort(const std::string &topic, const std::string &message)
    {
        (void)message;
        zwave_network_management_abort();
    }

    void NetworkManagementMqttApi::on_network_node_remove(const std::string &topic, const std::string &message)
    {
        (void)topic;
        (void)message;
        nlohmann::json report;
        if (zwave_controller_is_reset_ongoing()) {
            // `zwave_controller_is_reset_ongoing` is used in favor of
            // `zwave_network_management_get_state() == NM_IDLE` because it is possible that
            // on_network_node_remove is called after factory reset has actually started but
            // the network management state is not yet updated.
            sl_log_warning(LOG_TAG.data(), "Reset ongoing, rejecting request");
            report["reason"] = static_cast<sl_status_t>(zpc_status_t::FACTORY_RESET_ONGOING);
            report["status"] = MQTT_STATUS_FAIL;
            publish_report(MQTT_API_NETWORK_NODE_REMOVE_REPORT_TOPIC, report.dump(), false);
        } else {
            const zwave_network_management_state_t nm_state = zwave_network_management_get_state();
            if (nm_state != NM_IDLE) {
                sl_log_warning(LOG_TAG.data(), "Network management busy, rejecting request");
                report["reason"]   = static_cast<sl_status_t>(zpc_status_t::NETWORK_MANAGEMENT_BUSY);
                report["status"]   = MQTT_STATUS_FAIL;
                report["activity"] = nm_activity_to_string(zwave_network_management_get_activity(nm_state));  // Temporary field: will be replaced by another API in the future.
                publish_report(MQTT_API_NETWORK_NODE_REMOVE_REPORT_TOPIC, report.dump(), false);
            } else {
                sl_status_t status = zwave_network_management_remove_node();
                if (status != SL_STATUS_OK) {
                    sl_log_error(LOG_TAG.data(), "Failed to remove node: %d", status);
                }
            }
        }
    }

    void NetworkManagementMqttApi::on_network_dsk_accept(const std::string &topic, const std::string &message)
    {
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        nlohmann::json json_data;
        if (!message.empty()) {
            json_data = nlohmann::json::parse(message);
        } else {
            sl_log_error(LOG_TAG.data(), "Failed to parse message: %s", message.c_str());
            return;
        }

        auto it = json_data.find("dsk");
        if (it == json_data.end()) {
            sl_log_debug(LOG_TAG.data(), "dsk not found");
            return;
        }
        /// DSK can start with 0 which can't be a JSON number
        std::string number = it.value();
        int value          = std::atoi(number.c_str());
        if (value < 0 || value > 0xffff) {
            sl_log_error(LOG_TAG.data(), "DSK value out of range: %d", value);
            return;
        }
        uint16_t dsk         = static_cast<uint16_t>(value);
        node_reported_dsk[1] = (dsk) & 0xff;
        node_reported_dsk[0] = (dsk >> 8) & 0xff;

        sl_status_t status = zwave_network_management_dsk_set(node_reported_dsk);
        if (status != SL_STATUS_OK) {
            sl_log_error(LOG_TAG.data(), "Failed to set DSK: %d", status);
        }
    }

    void NetworkManagementMqttApi::on_network_grant_keys(const std::string &topic, const std::string &message)
    {
        (void)topic;
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        nlohmann::json json_data;
        if (!message.empty()) {
            try {
                json_data = nlohmann::json::parse(message);
            } catch (const nlohmann::json::exception &e) {
                sl_log_error(LOG_TAG.data(), "Failed to parse GrantKeys payload: %s", e.what());
                return;
            }
        } else {
            sl_log_error(LOG_TAG.data(), "GrantKeys command: empty payload");
            return;
        }

        auto accept_it = json_data.find("Accept");
        if (accept_it == json_data.end()) {
            sl_log_error(LOG_TAG.data(), "GrantKeys command: Accept field missing");
            return;
        }
        bool accept = accept_it->is_boolean() ? accept_it->get<bool>() : (accept_it->get<int>() != 0);

        auto keys_it = json_data.find("Keys");
        if (keys_it == json_data.end()) {
            sl_log_error(LOG_TAG.data(), "GrantKeys command: Keys field missing");
            return;
        }
        uint8_t keys;
        if (keys_it->is_string()) {
            std::string keys_str = keys_it->get<std::string>();
            keys                 = static_cast<uint8_t>(std::stoul(keys_str, nullptr, 0));
        } else {
            keys = static_cast<uint8_t>(keys_it->get<int>());
        }

        bool csa    = false;
        auto csa_it = json_data.find("CSA");
        if (csa_it != json_data.end()) {
            csa = csa_it->is_boolean() ? csa_it->get<bool>() : (csa_it->get<int>() != 0);
        }

        sl_status_t status = zwave_network_management_keys_set(accept, csa, keys);
        if (status != SL_STATUS_OK) {
            sl_log_error(LOG_TAG.data(), "Failed to set grant keys: %d", status);
        }
    }

    void NetworkManagementMqttApi::on_network_node_list(const std::string &topic, const std::string &message)
    {
        (void)topic;
        (void)message;
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        nlohmann::json node_information_list = nlohmann::json::array();

        // Step 1: Get the root node
        attribute_store_node_t root = attribute_store_get_root();
        if (root == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_error(LOG_TAG.data(), "Attribute store root is invalid");
            return;
        }

        // Step 2: Get the HOME_ID node for the current network
        zwave_home_id_t current_home_id     = zwave_network_management_get_home_id();
        attribute_store_node_t home_id_node = attribute_store_get_node_child_by_value(root, ATTRIBUTE_HOME_ID, REPORTED_ATTRIBUTE, (uint8_t *)&current_home_id, sizeof(current_home_id), 0);

        if (home_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_error(LOG_TAG.data(), "HOME_ID node not found for home_id: 0x%08X", current_home_id);
            return;
        }

        // Step 3: Iterate through all NODE_ID children
        uint32_t node_id_index              = 0;
        attribute_store_node_t node_id_node = attribute_store_get_node_child_by_type(home_id_node, ATTRIBUTE_NODE_ID, node_id_index);

        // Move to the next NodeID, because the first one is us
        node_id_index++;
        node_id_node = attribute_store_get_node_child_by_type(home_id_node, ATTRIBUTE_NODE_ID, node_id_index);

        while (node_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
            // Read the NodeID value
            zwave_node_id_t node_id = 0;
            sl_status_t status      = attribute_store_read_value(node_id_node, REPORTED_ATTRIBUTE, &node_id, sizeof(zwave_node_id_t));

            if (status == SL_STATUS_OK) {
                sl_log_debug(LOG_TAG.data(), "Found NodeID: %d", node_id);
                nlohmann::json node_information = nlohmann::json::object();
                nlohmann::json version_report   = nlohmann::json::object();

                device_interviewer_get_node_information_payload_t payload_struct;
                payload_struct.device_node = attribute_store::attribute(node_id_node);

                // NODE_INFORMATION_GROUP
                component_connector connector;
                auto future                            = connector.fire_event_async<device_interviewer_get_node_information_payload_t, device_interviewer_get_node_information_payload_t>(static_cast<uint32_t>(device_interviewer_events_t::DEVICE_INTERVIEWER_GET_NODE_INFORMATION), payload_struct);
                auto [status, node_information_result] = future.get();

                if (status != SL_STATUS_OK) {
                    sl_log_warning(LOG_TAG.data(), "Failed to get node information for NodeID: %d", node_id);
                }

                node_information["node_id"]               = node_id;
                node_information["listening_protocol"]    = json_optional_value(node_information_result.listening_protocol);
                node_information["optional_protocol"]     = json_optional_value(node_information_result.optional_protocol);
                node_information["basic_device_class"]    = json_optional_value(node_information_result.basic_device_class);
                node_information["generic_device_class"]  = json_optional_value(node_information_result.generic_device_class);
                node_information["specific_device_class"] = json_optional_value(node_information_result.specific_device_class);
                node_information["command_class_list"]    = json_optional_command_class_list(node_information_result.command_class_list);
                node_information["s2_command_class_list"] = json_optional_command_class_list(node_information_result.s2_command_class_list);
                node_information["s0_command_class_list"] = json_optional_command_class_list(node_information_result.s0_command_class_list);
                node_information["inclusion_protocol"]    = json_optional_value(node_information_result.inclusion_protocol);
                node_information["granted_keys"]          = json_optional_value(node_information_result.granted_keys);

                // VERSION_REPORT_GROUP
                command_class_version_types::command_class_get_version_report_payload_t payload_struct_version_report;
                payload_struct_version_report.device_node = attribute_store::attribute(node_id_node);

                auto future_version = connector.fire_event_async<command_class_version_types::command_class_get_version_report_payload_t, command_class_version_types::command_class_get_version_report_payload_t>(static_cast<uint32_t>(command_class_version_events_t::COMMAND_CLASS_GET_VERSION_REPORT),
                                                                                                                                                                                                                   payload_struct_version_report);
                auto [status_version, version_report_result] = future_version.get();
                if (status_version != SL_STATUS_OK) {
                    sl_log_warning(LOG_TAG.data(), "Failed to get version report for NodeID: %d", node_id);
                }
                version_report["z_wave_library_type"]         = json_optional_value(version_report_result.z_wave_library_type);
                version_report["z_wave_protocol_version"]     = json_optional_value(version_report_result.z_wave_protocol_version);
                version_report["z_wave_protocol_sub_version"] = json_optional_value(version_report_result.z_wave_protocol_sub_version);
                version_report["firmware_0_version"]          = json_optional_value(version_report_result.firmware_0_version);
                version_report["firmware_0_sub_version"]      = json_optional_value(version_report_result.firmware_0_sub_version);
                version_report["hardware_version"]            = json_optional_value(version_report_result.hardware_version);
                version_report["number_of_firmware_targets"]  = json_optional_value(version_report_result.number_of_firmware_targets);

                node_information_list.push_back({{"node_information", std::move(node_information)}, {"version_report", std::move(version_report)}});
            } else {
                sl_log_warning(LOG_TAG.data(), "Failed to read NodeID value for node at index %d", node_id_index);
            }

            // Move to the next NodeID
            node_id_index++;
            node_id_node = attribute_store_get_node_child_by_type(home_id_node, ATTRIBUTE_NODE_ID, node_id_index);
        }

        auto json_str = node_information_list.dump();

        publish_report(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_LIST_REPORT_TOPIC, json_str, false);
    }

    void NetworkManagementMqttApi::on_network_node_properties(const std::string &topic, const std::string &message)
    {
        (void)topic;
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        nlohmann::json json_data;
        if (!message.empty()) {
            try {
                json_data = nlohmann::json::parse(message);
            } catch (const nlohmann::json::exception &) {
                sl_log_error(LOG_TAG.data(), "Node properties request: invalid request payload");
                return;
            }
        } else {
            sl_log_error(LOG_TAG.data(), "Node properties request: empty payload");
            return;
        }

        auto it = json_data.find("node_id");
        if (it == json_data.end()) {
            sl_log_error(LOG_TAG.data(), "Node properties request: node_id not found in payload");
            return;
        }
        zwave_node_id_t node_id = static_cast<zwave_node_id_t>(it->get<int>());
        if (node_id == 0) {
            sl_log_error(LOG_TAG.data(), "Node properties request: node_id must be non-zero");
            return;
        }

        zwave_home_id_t home_id     = zwave_network_management_get_home_id();
        attribute_store_node_t root = attribute_store_get_root();
        if (root == ATTRIBUTE_STORE_INVALID_NODE) {
            return;
        }
        attribute_store_node_t home_id_node = attribute_store_get_node_child_by_value(root, ATTRIBUTE_HOME_ID, REPORTED_ATTRIBUTE, reinterpret_cast<const uint8_t *>(&home_id), sizeof(home_id), 0);
        if (home_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
            return;
        }
        attribute_store_node_t node_id_node = attribute_store_get_node_child_by_value(home_id_node, ATTRIBUTE_NODE_ID, REPORTED_ATTRIBUTE, reinterpret_cast<const uint8_t *>(&node_id), sizeof(node_id), 0);
        if (node_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_debug(LOG_TAG.data(), "Node properties: node_id %d not in attribute store", node_id);
            return;
        }

        nlohmann::json report = {{"node_id", node_id},
                                 {"network_status", nullptr},
                                 {"inclusion_protocol", nullptr},
                                 {"granted_keys", nullptr},
                                 {"last_rx_tx_timestamp", nullptr},
                                 {"last_rx_rssi", nullptr},
                                 {"last_routing_path", nullptr},
                                 {"last_tx_ticks", nullptr},
                                 {"last_number_of_repeaters", nullptr},
                                 {"last_tx_power", nullptr}};

        attribute_store_node_t child;
        uint32_t u32_val;
        uint64_t u64_val;
        uint16_t u16_val;
        uint8_t u8_val;
        int8_t i8_val;
        uint8_t path_buf[4] = {0, 0, 0, 0};
        uint8_t path_size;

        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_ZWAVE_INCLUSION_PROTOCOL, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &u32_val, sizeof(u32_val))) {
            report["inclusion_protocol"] = u32_val;
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_GRANTED_SECURITY_KEYS, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &u8_val, sizeof(u8_val))) {
            report["granted_keys"] = u8_val;
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_LAST_RECEIVED_FRAME_TIMESTAMP, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE) {
            uint8_t timestamp_size = attribute_store_get_node_value_size(child, REPORTED_ATTRIBUTE);
            if (timestamp_size == sizeof(uint32_t) && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &u32_val, sizeof(u32_val))) {
                report["last_rx_tx_timestamp"] = u32_val;
            } else if (timestamp_size == sizeof(uint64_t) && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &u64_val, sizeof(u64_val))) {
                report["last_rx_tx_timestamp"] = u64_val;
            }
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_LAST_RX_RSSI, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &i8_val, sizeof(i8_val))) {
            report["last_rx_rssi"] = i8_val;
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_LAST_ROUTING_PATH, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE) {
            path_size = attribute_store_get_node_value_size(child, REPORTED_ATTRIBUTE);
            if (path_size > 0 && path_size <= sizeof(path_buf) && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, path_buf, path_size)) {
                report["last_routing_path"] = nlohmann::json::array({path_buf[0], path_buf[1], path_buf[2], path_buf[3]});
            }
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_LAST_TX_TICKS, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &u16_val, sizeof(u16_val))) {
            report["last_tx_ticks"] = u16_val;
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_LAST_NUMBER_OF_REPEATERS, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &u8_val, sizeof(u8_val))) {
            report["last_number_of_repeaters"] = u8_val;
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_LAST_TX_POWER, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE && SL_STATUS_OK == attribute_store_read_value(child, REPORTED_ATTRIBUTE, &i8_val, sizeof(i8_val))) {
            report["last_tx_power"] = i8_val;
        }
        child = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_NODE_IS_S2_CAPABLE, 0);
        if (child != ATTRIBUTE_STORE_INVALID_NODE) {
            report["s2_capability"] = true;
        } else {
            report["s2_capability"] = false;
        }

        child                                      = attribute_store_get_node_child_by_type(node_id_node, static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::NETWORK_MONITOR_GROUP), 0);
        child                                      = attribute_store_get_node_child_by_type(child, static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::network_status), 0);
        NetworkMonitorNetworkStatus network_status = NETWORK_MONITOR_NETWORK_STATUS_UNAVAILABLE;
        attribute_store_get_reported(child, &network_status, sizeof(network_status));
        switch (network_status) {
            case NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL:
                report["network_status"] = "online";
                break;
            case NETWORK_MONITOR_NETWORK_STATUS_OFFLINE:
                report["network_status"] = "offline";
                break;
            default:
                report["network_status"] = "unknown";
                break;
        }

        std::string json_str = report.dump();

        publish_report(NetworkManagementMqttApi::MQTT_API_NETWORK_NODE_PROPERTIES_REPORT_TOPIC, json_str, false);
    }

    void NetworkManagementMqttApi::on_network_factory_reset(const std::string &topic, const std::string &message)
    {
        (void)topic;
        (void)message;
        // zwave_controller_reset() silently ignores the request when a reset
        // is already in progress. Detect that case explicitly up front so the
        // post-call state check does not misinterpret a reset chain that
        // completed synchronously (reset_ongoing is cleared before
        // zwave_controller_reset() returns when all steps resolve inline) as
        // a failed initiation.
        if (zwave_controller_is_reset_ongoing()) {
            sl_log_warning(LOG_TAG.data(), "Factory reset already in progress, ignoring request");
            return;
        }
        // The reset chain runs steps in priority order: cleaning up associations,
        // sending Device Reset Locally Notifications to lifeline nodes, updating
        // the SmartStart list, flushing the Tx queue, and finally resetting the
        // network at zwave_network_management_set_default (priority 5). That
        // final step refuses to run when network management is already busy
        // (e.g. inclusion/exclusion in progress). Fail fast here so we do not
        // emit Device Reset Locally Notifications for a reset that the network
        // management layer will then abort.
        if (zwave_network_management_is_busy()) {
            sl_log_warning(LOG_TAG.data(), "Network management is busy, ignoring factory reset request");
            return;
        }
        zwave_controller_reset();
    }

    void NetworkManagementMqttApi::on_network_nls_enable(const std::string &topic, const std::string &message)
    {
        (void)topic;
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        nlohmann::json json_data;
        if (!message.empty()) {
            try {
                json_data = nlohmann::json::parse(message);
            } catch (const nlohmann::json::exception &) {
                sl_log_error(LOG_TAG.data(), "NLS Enable: invalid request payload");
                return;
            }
        } else {
            sl_log_error(LOG_TAG.data(), "NLS Enable: empty payload");
            return;
        }

        auto it = json_data.find("node_id");
        if (it == json_data.end()) {
            sl_log_error(LOG_TAG.data(), "NLS Enable: node_id not found in payload");
            return;
        }

        zwave_node_id_t node_id = static_cast<zwave_node_id_t>(it->get<int>());
        if (node_id == 0) {
            sl_log_error(LOG_TAG.data(), "NLS Enable: node_id must be non-zero");
            return;
        }

        sl_status_t status = zwave_store_nls_state(node_id, true, DESIRED_ATTRIBUTE);

        nlohmann::json report;
        report["node_id"] = node_id;
        report["status"]  = (status == SL_STATUS_OK) ? "ok" : "fail";
        publish_report(MQTT_API_NETWORK_NLS_ENABLE_REPORT_TOPIC, report.dump(), false);
    }

    void NetworkManagementMqttApi::on_network_nls_state(const std::string &topic, const std::string &message)
    {
        (void)topic;
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        nlohmann::json json_data;
        if (!message.empty()) {
            try {
                json_data = nlohmann::json::parse(message);
            } catch (const nlohmann::json::exception &) {
                sl_log_error(LOG_TAG.data(), "NLS State: invalid request payload");
                return;
            }
        } else {
            sl_log_error(LOG_TAG.data(), "NLS State: empty payload");
            return;
        }

        auto it = json_data.find("node_id");
        if (it == json_data.end()) {
            sl_log_error(LOG_TAG.data(), "NLS State: node_id not found in payload");
            return;
        }

        zwave_node_id_t node_id = static_cast<zwave_node_id_t>(it->get<int>());
        if (node_id == 0) {
            sl_log_error(LOG_TAG.data(), "NLS State: node_id must be non-zero");
            return;
        }

        uint8_t nls_state   = 0;
        uint8_t nls_support = 0;
        sl_status_t status  = zwapi_get_node_nls(node_id, &nls_state, &nls_support);

        nlohmann::json report;
        report["node_id"] = node_id;

        if (status == SL_STATUS_OK) {
            zwave_store_nls_state(node_id, nls_state != 0U, REPORTED_ATTRIBUTE);
            zwave_store_nls_support(node_id, nls_support != 0U, REPORTED_ATTRIBUTE);

            report["nls_support"] = static_cast<bool>(nls_support);
            report["nls_state"]   = static_cast<bool>(nls_state);
            report["status"]      = "ok";
        } else {
            report["status"] = "fail";
            sl_log_error(LOG_TAG.data(), "Unable to read NLS state for NodeID %d", node_id);
        }

        publish_report(MQTT_API_NETWORK_NLS_STATE_REPORT_TOPIC, report.dump(), false);
    }

    void NetworkManagementMqttApi::on_network_node_remove_failed(const std::string &topic, const std::string &message)
    {
        (void)topic;
        sl_log_debug(LOG_TAG.data(), "%s: %s %s", __func__, topic.c_str(), message.c_str());

        if (message.empty()) {
            sl_log_error(LOG_TAG.data(), "Remove Failed Node: empty payload");
            return;
        }

        nlohmann::json json_data;
        try {
            json_data = nlohmann::json::parse(message);
        } catch (const nlohmann::json::exception &) {
            sl_log_error(LOG_TAG.data(), "Remove Failed Node: invalid request payload");
            return;
        }

        auto it = json_data.find("node_id");
        if (it == json_data.end()) {
            sl_log_error(LOG_TAG.data(), "Remove Failed Node: node_id not found in payload");
            return;
        }

        zwave_node_id_t node_id = static_cast<zwave_node_id_t>(it->get<int>());
        if (node_id == 0) {
            sl_log_error(LOG_TAG.data(), "Remove Failed Node: node_id must be non-zero");
            return;
        }

        sl_status_t status = zwave_network_management_remove_failed(node_id);
        if (status != SL_STATUS_OK) {
            sl_log_error(LOG_TAG.data(), "Failed to start remove failed node: %d", status);
            nlohmann::json report;
            report["node_id"] = node_id;
            report["status"]  = "fail";
            report["reason"]  = "not_ready";
            publish_report(MQTT_API_NETWORK_NODE_REMOVE_FAILED_REPORT_TOPIC, report.dump(), false);
            return;
        }

        sl_log_debug(LOG_TAG.data(), "Remove Failed Node operation started for NodeID: %d", node_id);
    }
}  // namespace zwave_command_class
