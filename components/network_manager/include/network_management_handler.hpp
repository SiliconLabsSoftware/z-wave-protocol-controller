
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

#ifndef NETWORK_MANAGEMENT_HANDLER_H
#define NETWORK_MANAGEMENT_HANDLER_H

// ZPC includes
#include "mqtt_handler.hpp"
#include "log.h"
#include "sl_status.h"

#include <nlohmann/json.hpp>
#include <cstddef>
#include <map>
#include <sstream>
#include <deque>
#include <algorithm>
#include <any>

#include "threading.hpp"
#include "safe_queue.hpp"
#include "sl_status.h"
#include "zwave_controller_types.h"
#include "init_builder.hpp"
#include "network_management_mqtt_api.hpp"
#include <string>

namespace zwave_component
{
    typedef enum {
        /// A new node was added to the network
        NODE_ADDED_EVENT,
        /// Node interview done
        NODE_DELETED_EVENT,
        ///
        MQTT_CB_EVENT,
    } network_monitor_events_t;

    typedef enum {
        /// The network management state machine is idle and ready to carry operations
        NM_TOPIC_IDLE,
        /// Include a new node into the network
        NM_TOPIC_ADD_NODE,
        /// Remove any or a specific node from the network
        NM_TOPIC_REMOVE_NODE,
        /// Remove a non-responsive or non-reachable node from the network topology
        NM_TOPIC_REMOVE_FAILED_NODE,
        /// ZPC is joining a new network
        NM_TOPIC_JOIN_NETWORK,
        /// ZPC is leaving the current network
        NM_TOPIC_LEAVE_NETWORK,
        /// ZPC is interviewing a node
        NM_TOPIC_NODE_INTERVIEW,
        /// The Protocol Controller is carrying out some network repair functions,
        /// such as providing routes for nodes or
        /// remove bindings towards nodes that have left the network
        NM_TOPIC_NETWORK_REPAIR,
        /// The Protocol Controller is carrying out some network maintenance,
        /// which can for example be distributing
        /// the network topology or state to all other controller nodes in the network.
        NM_TOPIC_NETWORK_UPDATE,
        /// The Protocol Controller is resetting to default
        NM_TOPIC_RESET,
        /// The Protocol Controller goes offline and will not be reachable at all.
        /// It can be for example when it needs to restart after a firmware update
        NM_TOPIC_TEMPORARILY_OFFLINE,
        /// The Protocol Controller is looking for nodes to join the network,
        /// and will provide a list of candidates. No nodes are added yet
        NM_TOPIC_SCAN_MODE,
        /// This state shall always be last
        NM_TOPIC_LAST,
    } ucl_network_management_state_t;

    class NetworkManagementStateData
    {
        public:
            const ucl_network_management_state_t state;
            const std::map<std::string, std::string, std::less<>> state_parameters;
            const std::vector<std::string> requested_state_parameters;

            NetworkManagementStateData(ucl_network_management_state_t state, const std::map<std::string, std::string, std::less<>> &state_parameters, const std::vector<std::string> &requested_state_parameters) :
              state(state), state_parameters(state_parameters), requested_state_parameters(requested_state_parameters)
            {}

            NetworkManagementStateData(ucl_network_management_state_t state, const std::map<std::string, std::string, std::less<>> &state_parameters) : NetworkManagementStateData(state, state_parameters, std::vector<std::string>()) {}

            explicit NetworkManagementStateData(ucl_network_management_state_t state, const std::vector<std::string> &) : NetworkManagementStateData(state, std::map<std::string, std::string, std::less<>>(), requested_state_parameters) {}

            explicit NetworkManagementStateData(ucl_network_management_state_t state) : NetworkManagementStateData(state, std::map<std::string, std::string, std::less<>>(), std::vector<std::string>()) {}
    };

    class network_management_handler : public threading::threading, public Initializable
    {
        public:
            network_management_handler();
            ~network_management_handler();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        private:
            // Static instance pointer to access instance from static wrapper
            static network_management_handler *instance_ptr;
            static const zwave_controller_callbacks_t ucl_network_management_callbacks;
            static bool allow_multiple_inclusions;
            static zwave_home_id_t zpc_home_id;
            static zwave_node_id_t zpc_node_id;

            enum class network_management_event_t {
                MQTT_CB_EVENT,
            };

            struct network_management_event_data {
                    network_management_event_t event;
                    std::any data;
            };

            // Static wrapper functions for callbacks (can be used as function pointers)
            static void network_management_mqtt_callback_static(const std::string &topic, const std::string &message);
            static void network_management_on_network_address_update_static(zwave_home_id_t home_id, zwave_node_id_t node_id);
            static void network_management_on_node_added_static(sl_status_t status, const zwave_node_info_t *nif, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol);
            static void network_management_on_state_updated_static(zwave_network_management_state_t nm_state);
            static void network_management_on_keys_report_static(bool csa, zwave_keyset_t keys);
            static void network_management_on_dsk_report_static(uint8_t input_length, zwave_dsk_t dsk, zwave_keyset_t keys);
            static sl_status_t state_topic_update(const NetworkManagementStateData &state_data);

            static void network_management_init(void);
            static void network_management_exit(void);
            void network_management_mqtt_callback(const std::string &topic, const std::string &message);
            static void network_management_on_network_address_update(zwave_home_id_t home_id, zwave_node_id_t node_id);
            static void network_management_on_node_added(sl_status_t status, const zwave_node_info_t *nif, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol);
            static void network_management_on_state_updated(zwave_network_management_state_t nm_state);
            static void network_management_on_keys_report(bool csa, zwave_keyset_t keys);
            static void network_management_on_dsk_report(uint8_t input_length, zwave_dsk_t dsk, zwave_keyset_t keys);
            static sl_status_t write_topic_received(const std::string &message);
            static nlohmann::json get_case_insensitive_json(const nlohmann::json &jsn, const std::string &key);
            static nlohmann::json get_supported_states(ucl_network_management_state_t state);
            ::threading::safe_queue<network_management_event_data> event_queue;
            zwave_command_class::NetworkManagementMqttApi network_management_mqtt_api_instance;
            void run() override;
    };
}  // namespace zwave_component

#endif  // NETWORK_MANAGEMENT_HANDLER_H
