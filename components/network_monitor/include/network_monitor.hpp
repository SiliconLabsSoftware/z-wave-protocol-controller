/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#ifndef NETWORK_MONITOR_HPP
#define NETWORK_MONITOR_HPP

#include <memory>
#include <variant>
#include <chrono>
#include <map>
#include <cstdint>
#include <string>
#include "threading.hpp"
#include "init_builder.hpp"
#include "safe_queue.hpp"
#include "zpc_attribute_store_type_registration.h"
#include "zwave_network_management_types.h"
#include "zwave_controller_types.h"
#include "zwave_keyset_definitions.h"
#include "zwave_controller_keyset.h"
#include "attribute_store.h"
#include "attribute.hpp"
#include "network_monitor_network_status.h"
#include "network_monitor_mqtt_api.hpp"
#include "sl_status.h"
#include "attribute_store_type_registration.h"
#include "network_monitor_attribute_store.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "command_class_wake_up_types.hpp"
#include "command_class_wake_up_events.hpp"

using namespace attribute_store;
using namespace zwave_command_class::command_class_wake_up_types;

// Forward declarations
struct network_data;
struct node_added_event_data;
struct node_id_assigned_event_data;

namespace zwave_component
{
    /**
     * @brief Network Monitor class that manages network state and node monitoring
     *
     * This class uses a thread-based approach with a thread-safe event queue.
     */
    class network_monitor_handler : public threading::threading, public Initializable
    {
        public:
            /**
             * @brief Event types for the Network Monitor
             */
            enum class EventType { NETWORK_READY, NODE_ID_ASSIGNED, NODE_ADDED, NODE_INTERVIEW_INITIATED, NODE_INTERVIEW_DONE, NODE_DELETED, NODE_FRAME_TRANSMISSION_FAILED, NODE_FRAME_TRANSMISSION_SUCCESS, NODE_FRAME_RX };

            /**
             * @brief Event structure for the event queue
             */
            struct Event {
                    EventType type;
                    std::variant<std::unique_ptr<network_data>, std::unique_ptr<node_added_event_data>, std::unique_ptr<node_id_assigned_event_data>, attribute_store_node_t, zwave_node_id_t> data;
            };

            network_monitor_handler();
            ~network_monitor_handler();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

            /**
             * @brief Post an event to the event queue
             */
            void post_event(EventType type, std::unique_ptr<network_data> data);
            void post_event(EventType type, std::unique_ptr<node_added_event_data> data);
            void post_event(EventType type, std::unique_ptr<node_id_assigned_event_data> data);
            void post_event(EventType type, attribute_store_node_t node);
            void post_event(EventType type, zwave_node_id_t node_id);

            /**
             * @brief Get the cached network node list
             */
            uint8_t *get_cached_current_node_list();

            /**
             * @brief Thread run method (called by base class)
             */
            void run() override;

        private:
            // Event queue
            ::threading::safe_queue<Event> event_queue_;

            // Cached ZPC network address (Home ID / Node ID)
            zwave_home_id_t zpc_home_id_;
            zwave_node_id_t zpc_node_id_;

            // Indicates if we initialized a network
            bool network_initialized_;

            // Cache of the list of nodes in our network
            zwave_nodemask_t current_node_list_;

            // List of NodeID and how many consecutive transmit failures they had
            std::map<zwave_node_id_t, uint8_t> failed_transmission_data_;

            // MQTT API for publishing network status
            network_monitor::NetworkMonitorMqttApi network_monitor_mqtt_api;

            // Event handlers
            void handle_event_network_ready(const network_data &event_data);
            static void handle_event_node_id_assigned(const node_id_assigned_event_data &event_data);
            void handle_event_node_added(const node_added_event_data &event_data);
            static void handle_event_node_interview_initiated(attribute_store_node_t node_id_node);
            static void handle_event_node_interview_done(attribute_store_node_t node_id_node);
            void handle_event_node_deleted(zwave_node_id_t node_id);
            void handle_event_failed_frame_transmission(zwave_node_id_t node_id);
            void handle_event_success_frame_transmission(zwave_node_id_t node_id);
            void handle_event_network_address_update(network_data *event_data);

        public:
            // Public methods needed by C callbacks / free-function connectors
            void on_network_address_update_internal(network_data *event_data);
            static void handle_event_node_interview_done(attribute_store_node_t node_id_node, sl_status_t interview_status);

        private:
            // Helper methods
            static void remove_attribute_store_home_id(zwave_home_id_t old_home_id);
            static void remove_attribute_store_node(zwave_node_id_t node_id);
            static attribute network_monitor_add_attribute_store_node(zwave_node_id_t node_id, NetworkMonitorNetworkStatus network_status);
            void create_attribute_store_network_nodes(zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type);
            static void activate_network_resolution(bool resolve_our_network);
            static void pause_nl_nodes_resolution(attribute_store_node_t current_network_node);
            static void mark_node_as_offline(attribute_store_node_t node_id_node);
            static void mark_node_as_online(attribute_store_node_t node_id_node);
            static void update_new_node_attribute_store(const node_added_event_data &node_added_data);
            static void update_last_received_frame_timestamp(attribute_store_node_t node_id_node);
            static void update_all_network_statuses(attribute_store_node_t home_id_node, NetworkMonitorNetworkStatus old_value, NetworkMonitorNetworkStatus new_value);

            // NL node offline monitoring via wake-up interval
            static void register_component_connector_handlers();
            static void on_wake_up_interval_report_received(const wake_up_interval_report_payload_t &payload);
            static void on_wake_up_notification_received(const wake_up_notification_payload_t &payload);
            static void on_wake_up_no_more_information_sent(const wake_up_no_more_information_sent_payload_t &payload);
            static void start_or_restart_nl_offline_timer(attribute_store_node_t node_id_node, uint32_t wake_up_interval_seconds);
            static void on_nl_node_offline_timeout(attribute_store_node_t node_id_node);

            const std::vector<attribute_schema_t> attributes = {
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::NETWORK_MONITOR_GROUP), "NETWORK_MONITOR_GROUP", ATTRIBUTE_NODE_ID, U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::ping_interval), "Ping Interval", static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::NETWORK_MONITOR_GROUP), U32_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::multicast_group_list), "Multicast Group List", static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::NETWORK_MONITOR_GROUP), EMPTY_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::multicast_group), "Multicast Group", static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::multicast_group_list), U8_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::s2_span_entry), "S2 Span Entry", static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::NETWORK_MONITOR_GROUP), BYTE_ARRAY_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::s2_mpan_table), "S2 Mpan Table", static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::NETWORK_MONITOR_GROUP), EMPTY_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::s2_mpan_entry), "S2 Mpan Entry", static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::s2_mpan_table), BYTE_ARRAY_STORAGE_TYPE},
              {static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::network_status), "Network Status", static_cast<attribute_store_type_t>(network_monitor::network_monitor_attributes_t::NETWORK_MONITOR_GROUP), U32_STORAGE_TYPE},
            };
    };
}  // namespace zwave_component

#endif  // NETWORK_MONITOR_HPP
