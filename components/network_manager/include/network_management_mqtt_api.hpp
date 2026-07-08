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

#ifndef NETWORK_MANAGEMENT_MQTT_API_H
#define NETWORK_MANAGEMENT_MQTT_API_H

#include "mqtt_api_base.hpp"
#include "sl_status.h"
#include "component_connector_types.hpp"
#include "zwave_keyset_definitions.h"
#include <any>
#include <string>
#include <string_view>

namespace zwave_command_class
{
    /**
     * @brief MQTT API for network management operations
     *
     * Handles MQTT commands for network management:
     * - Node Add/Remove
     * - DSK Accept
     * - Grant Keys
     * - Node List
     * - Node Properties
     *
     * Also listens to component_connector events for node added/deleted
     * and publishes MQTT reports.
     */
    class NetworkManagementMqttApi : public MqttApiBase
    {
        public:
            NetworkManagementMqttApi()  = default;
            ~NetworkManagementMqttApi() = default;

            void setup_mqtt_api() override;

            /**
             * @brief Publish requested keys to MQTT (Network/RequestedKeys) during S2 inclusion.
             * Called from network_management_on_keys_report when the node requests security keys.
             */
            static void publish_requested_keys(bool csa, zwave_keyset_t keys);

            /**
             * @brief Publish requested DSK to MQTT (Network/RequestedDSK) during S2 inclusion.
             * Called from network_management_on_dsk_report when the node reports its DSK and user verification is needed.
             */
            static void publish_requested_dsk(uint8_t input_length, const std::string &dsk_str);

        private:
            inline static std::string MQTT_API_NETWORK_NODE_ADD_TOPIC               = "Network/Node/Add";
            inline static std::string MQTT_API_NETWORK_NODE_ADD_REPORT_TOPIC        = MQTT_API_NETWORK_NODE_ADD_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_NODE_ADD_ABORT_TOPIC         = MQTT_API_NETWORK_NODE_ADD_TOPIC + "/Abort";
            inline static std::string MQTT_API_NETWORK_NODE_REMOVE_TOPIC            = "Network/Node/Remove";
            inline static std::string MQTT_API_NETWORK_NODE_REMOVE_REPORT_TOPIC     = MQTT_API_NETWORK_NODE_REMOVE_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_NODE_REMOVE_ABORT_TOPIC      = MQTT_API_NETWORK_NODE_REMOVE_TOPIC + "/Abort";
            inline static std::string MQTT_API_NETWORK_DSK_ACCEPT_TOPIC             = "Network/DSK/Accept";
            inline static std::string MQTT_API_NETWORK_GRANT_KEYS_TOPIC             = "Network/GrantKeys";
            inline static std::string MQTT_API_NETWORK_REQUESTED_KEYS_TOPIC         = "Network/RequestedKeys";
            inline static std::string MQTT_API_NETWORK_REQUESTED_KEYS_REPORT_TOPIC  = MQTT_API_NETWORK_REQUESTED_KEYS_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_REQUESTED_DSK_TOPIC          = "Network/RequestedDSK";
            inline static std::string MQTT_API_NETWORK_REQUESTED_DSK_REPORT_TOPIC   = MQTT_API_NETWORK_REQUESTED_DSK_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_NODE_LIST_TOPIC              = "Network/Node/List";
            inline static std::string MQTT_API_NETWORK_NODE_LIST_REPORT_TOPIC       = MQTT_API_NETWORK_NODE_LIST_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_NODE_PROPERTIES_TOPIC        = "Network/Node/Properties";
            inline static std::string MQTT_API_NETWORK_NODE_PROPERTIES_REPORT_TOPIC = MQTT_API_NETWORK_NODE_PROPERTIES_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_FACTORY_RESET_TOPIC          = "Network/FactoryReset";
            // Published globally (no home-id prefix) like zpc/Discovery/Report, so
            // CTT/clients can subscribe to a stable topic without having to learn
            // the new home id that the reset assigns.
            inline static std::string MQTT_API_NETWORK_FACTORY_RESET_REPORT_TOPIC      = "zpc/Network/FactoryReset/Report";
            inline static std::string MQTT_API_NETWORK_NLS_ENABLE_TOPIC                = "Network/NLS/Enable";
            inline static std::string MQTT_API_NETWORK_NLS_ENABLE_REPORT_TOPIC         = MQTT_API_NETWORK_NLS_ENABLE_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_NLS_STATE_TOPIC                 = "Network/NLS/State";
            inline static std::string MQTT_API_NETWORK_NLS_STATE_REPORT_TOPIC          = MQTT_API_NETWORK_NLS_STATE_TOPIC + "/Report";
            inline static std::string MQTT_API_NETWORK_NODE_REMOVE_FAILED_TOPIC        = "Network/Node/RemoveFailed";
            inline static std::string MQTT_API_NETWORK_NODE_REMOVE_FAILED_REPORT_TOPIC = MQTT_API_NETWORK_NODE_REMOVE_FAILED_TOPIC + "/Report";

            /// MQTT Add/Report reason: security bootstrapping failed (S2 timeout / kex fail).
            static constexpr int MQTT_REASON_NODE_ADD_SECURITY_FAIL = 6404;

            // Event handlers
            static sl_status_t on_node_added(const component_connector_node_added_payload_t &payload);
            static sl_status_t on_node_deleted(const component_connector_node_deleted_payload_t &payload);
            static sl_status_t on_failed_node_deleted(const component_connector_node_remove_failed_payload_t &payload);
            static sl_status_t on_factory_reset_complete(const component_connector_factory_reset_complete_payload_t &payload);

            // Topic handlers
            static void on_network_node_add(const std::string &topic, const std::string &message);
            static void on_network_management_abort(const std::string &topic, const std::string &message);
            static void on_network_node_remove(const std::string &topic, const std::string &message);
            static void on_network_node_remove_failed(const std::string &topic, const std::string &message);
            static void on_network_dsk_accept(const std::string &topic, const std::string &message);
            static void on_network_grant_keys(const std::string &topic, const std::string &message);
            static void on_network_node_list(const std::string &topic, const std::string &message);
            static void on_network_node_properties(const std::string &topic, const std::string &message);
            static void on_network_factory_reset(const std::string &topic, const std::string &message);
            static void on_network_nls_enable(const std::string &topic, const std::string &message);
            static void on_network_nls_state(const std::string &topic, const std::string &message);
    };
}  // namespace zwave_command_class

#endif  // NETWORK_MANAGEMENT_MQTT_API_H
