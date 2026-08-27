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
// Generic includes
#include <cstddef>
#include <cstring>
#include <string>
#include <map>
#include <memory>
#include <chrono>

// Includes from this component
#include "network_monitor.hpp"
#include "network_monitor.h"
#include "network_monitor_span_persistence.h"
#include "keep_sleeping_nodes_alive.h"
#include "network_monitor_utils.h"
#include "failing_node_monitor.h"

// Interfaces

#include "attribute_store_defined_attribute_types.h"

// ZPC components
#include "log.h"
#include "attribute_store_helper.h"
#include "network_monitor_network_status.h"
#include "attribute.hpp"
#include "attribute_timeouts.h"
#include "attribute_resolver.h"

// Component connector
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"

// ZPC components
#include "zwave_controller_connection_info.h"
#include "zwave_utils.h"
#include "zwave_network_management.h"
#include "zwave_network_management_types.h"
#include "zwave_controller.h"
#include "zwave_controller_keyset.h"
#include "zwave_controller_utils.h"
#include "zwave_tx_scheme_selector.h"
#include "zpc_config.h"
#include "zwave_controller_storage.h"
#include "zwave_association_toolbox.h"

#include "zpc_attribute_store_network_helper.h"
#include "zpc_attribute_store.h"

// Setup the logging
#define LOG_TAG "network_monitor"

static NetworkMonitorNetworkStatus attribute_store_network_helper_get_network_status(attribute_store_node_t node);

// NodeID attributes that should be created under all NodeIDs.
constexpr attribute_store_type_t node_id_additional_attributes[] = {ATTRIBUTE_GRANTED_SECURITY_KEYS, ATTRIBUTE_ZWAVE_INCLUSION_PROTOCOL, ATTRIBUTE_ZWAVE_PROTOCOL_LISTENING, ATTRIBUTE_ZWAVE_OPTIONAL_PROTOCOL};

using namespace attribute_store;
using namespace network_monitor;

/**
 * @brief Struct used for sending event data with event NETWORK_READY_EVENT.
 */
struct network_data {
        zwave_home_id_t home_id;              ///< Home ID
        zwave_node_id_t node_id;              ///< Node ID
        zwave_keyset_t granted_keys;          ///< Granted Keys
        zwave_kex_fail_type_t kex_fail_type;  ///< Kex Fail type.
};

/**
 * @brief Struct used for sending event data with event NODE_ADDED_EVENT.
 */
struct node_added_event_data {
        zwave_node_info_t nif;                ///< NIF
        zwave_node_id_t node_id;              ///< Node ID
        zwave_dsk_t dsk;                      ///< DSK
        zwave_keyset_t granted_keys;          ///< Granted Keys
        zwave_kex_fail_type_t kex_fail_type;  ///< Kex Fail type
        zwave_protocol_t inclusion_protocol;
        sl_status_t status;  ///< Add/security result (OK only if bootstrapping succeeded)
};

/** Same success predicate as device_interviewer: interview only after a good add. */
static bool node_add_succeeded(const node_added_event_data &e)
{
    return (e.status == SL_STATUS_OK) && (e.kex_fail_type == ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE);
}

/**
 * @brief Struct used for sending event data with event NODE_ID_ASSIGNED_EVENT.
 */
struct node_id_assigned_event_data {
        zwave_node_id_t node_id;  ///< Node ID
        zwave_protocol_t inclusion_protocol;
};

// Global instance
static zwave_component::network_monitor_handler *g_network_monitor_instance = nullptr;

// Callback structs - defined here so they can be used in init()
static const zwave_controller_storage_callback_t zwave_controller_storage_callbacks = {
  .set_node_as_s2_capable              = zwave_security_validation_set_node_as_s2_capable,
  .is_node_S2_capable                  = zwave_security_validation_is_node_s2_capable,
  .get_node_granted_keys               = zwave_get_node_granted_keys,
  .get_inclusion_protocol              = zwave_get_inclusion_protocol,
  .zwave_controller_storage_cc_version = zwave_node_get_command_class_version,
};

// C callback wrappers (these need to remain as C functions for zwave_controller callbacks)
static void network_monitor_on_node_id_assigned(zwave_node_id_t node_id, bool included_by_us, zwave_protocol_t inclusion_protocol);
static void network_monitor_on_node_deleted(zwave_node_id_t node_id);
static void network_monitor_on_node_added(sl_status_t status, const zwave_node_info_t *nif, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol);
static void network_monitor_on_nif_updated(attribute_store_node_t updated_node, attribute_store_change_t change);
static void network_monitor_on_frame_transmission_failed(zwave_node_id_t node_id);
static void network_monitor_on_frame_transmission_success(zwave_node_id_t node_id);
static void network_monitor_on_frame_received(zwave_node_id_t node_id, const uint8_t *frame_data, uint16_t frame_length);
static void network_monitor_on_application_frame_received(const zwave_controller_connection_info_t *connection_info, const zwave_rx_receive_options_t *rx_options, const uint8_t *frame_data, uint16_t frame_length);
static void network_monitor_on_frame_transmission(bool transmission_successful, const zwapi_tx_report_t *tx_report, zwave_node_id_t node_id);
static void network_monitor_on_network_address_update(zwave_home_id_t home_id, zwave_node_id_t node_id);
static void network_monitor_on_network_ready(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type);

// Network monitor callbacks struct - defined after forward declarations
static const zwave_controller_callbacks_t network_monitor_callbacks = {.on_node_id_assigned           = &network_monitor_on_node_id_assigned,
                                                                       .on_node_deleted               = &network_monitor_on_node_deleted,
                                                                       .on_node_added                 = &network_monitor_on_node_added,
                                                                       .on_network_address_update     = &network_monitor_on_network_address_update,
                                                                       .on_new_network_entered        = &network_monitor_on_network_ready,
                                                                       .on_application_frame_received = &network_monitor_on_application_frame_received,
                                                                       .on_frame_transmission         = &network_monitor_on_frame_transmission,
                                                                       .on_rx_frame_received          = &network_monitor_on_frame_received};

// network_monitor_handler class implementation
zwave_component::network_monitor_handler::network_monitor_handler() : threading::threading("network_monitor"), zpc_home_id_(0), zpc_node_id_(0), network_initialized_(false)
{
    memset(current_node_list_, 0, sizeof(zwave_nodemask_t));
    g_network_monitor_instance = this;

    for (const auto &a: attributes) {
        sl_status_t status = attribute_store_register_type(a.type, a.name, a.parent_type, a.storage_type);
        if (status != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG, "Failed to register attribute type %s (%u), status=%u", a.name, a.type, status);
        }
    }
}

zwave_component::network_monitor_handler::~network_monitor_handler()
{
    g_network_monitor_instance = nullptr;
}

// Returns true when the loaded attribute store has no NodeID children
// for the supplied HomeID (or the HomeID node itself does not exist).
static bool is_attribute_store_empty_for_home_id(zwave_home_id_t home_id)
{
    attribute_store_node_t home_id_node = attribute_store_network_helper_get_home_id_node(home_id);
    if (home_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
        return true;
    }
    return attribute_store_get_node_child_count_by_type(home_id_node, ATTRIBUTE_NODE_ID) == 0;
}

// Counts nodes the radio currently reports in its node list, excluding ZPC's own NodeID.
static size_t count_radio_remote_nodes(zwave_node_id_t zpc_node_id)
{
    zwave_nodemask_t node_list;
    memset(node_list, 0, sizeof(node_list));
    zwave_network_management_get_network_node_list(node_list);

    size_t count = 0;
    for (zwave_node_id_t node_id = ZW_MIN_NODE_ID; node_id <= ZW_LR_MAX_NODE_ID; node_id++) {
        if (!ZW_IS_NODE_IN_MASK(node_id, node_list)) {
            continue;
        }
        if (node_id == zpc_node_id) {
            continue;
        }
        count++;
    }
    return count;
}

sl_status_t zwave_component::network_monitor_handler::initialize()
{
    initialize_keep_alive_for_sleeping_nodes();
    register_component_connector_handlers();

    // Z-Wave Controller callbacks.
    zwave_controller_storage_callback_register(&zwave_controller_storage_callbacks);
    zwave_controller_register_callbacks(&network_monitor_callbacks);

    // Listens to NIF creations, so we can detect node interviews
    attribute_store_register_callback_by_type(network_monitor_on_nif_updated, ATTRIBUTE_ZWAVE_NIF);

    // Prime network address cache for the attribute store
    network_data data = {};
    data.node_id      = zwave_network_management_get_node_id();
    data.home_id      = zwave_network_management_get_home_id();
    // At init, if our keys are not in the datastore, we do not want
    // to create a wrong granted_key data, so we ask zwave_network_management()
    data.granted_keys  = zwave_network_management_get_granted_keys();
    data.kex_fail_type = (zwave_kex_fail_type_t)0;

    // If the attribute store has no node entries for our current HomeID but the
    // radio still reports remote nodes from a previous session, the datastore was
    // erased while the radio kept its node list. Re-creating those nodes here
    // would leave bricked ghosts in the AS: the security keys are gone and no
    // interview is triggered for nodes that just appear from the radio mask.
    // Reset the radio so AS and radio end up consistent and empty.
    if (is_attribute_store_empty_for_home_id(data.home_id)) {
        size_t remote_node_count = count_radio_remote_nodes(data.node_id);
        if (remote_node_count > 0) {
            sl_log_warning(LOG_TAG,
                           "Empty datastore detected with %zu node(s) still on the radio. "
                           "Resetting the radio to recover a clean state.",
                           remote_node_count);
            zwave_controller_reset();
            failing_node_monitor_init();
            return SL_STATUS_OK;
        }
    }

    // Execute directly, do not post an event for this, other components
    // initializing right after are depending on us doing the job
    handle_event_network_address_update(&data);

    // Restore the SPAN/MPAN data to S2
    network_monitor_restore_span_table_data();
    network_monitor_restore_mpan_table_data();

    // Cycle network statuses to trigger attribute store callbacks after datastore load
    attribute_store_node_t home_id_node = get_zpc_network_node();
    update_all_network_statuses(home_id_node, NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL, NETWORK_MONITOR_NETWORK_STATUS_UNAVAILABLE);
    update_all_network_statuses(home_id_node, NETWORK_MONITOR_NETWORK_STATUS_UNAVAILABLE, NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL);

    // Activate network resolution
    activate_network_resolution(true);

    // Initialize supporting modules
    failing_node_monitor_init();

    return SL_STATUS_OK;
}

int zwave_component::network_monitor_handler::shutdown()
{
    // Store SPAN/MPAN data before stopping and before attribute store teardown
    // This must happen in shutdown() rather than destructor to ensure it runs
    // before the attribute store is torn down during shutdown sequence
    network_monitor_store_span_table_data();
    network_monitor_store_mpan_table_data();

    stop();
    return 0;
}

std::string zwave_component::network_monitor_handler::name() const
{
    return "Network Monitor Handler";
}

void zwave_component::network_monitor_handler::post_event(EventType type, std::unique_ptr<network_data> data)
{
    Event event;
    event.type = type;
    event.data = std::move(data);
    event_queue_.push(std::move(event));
}

void zwave_component::network_monitor_handler::post_event(EventType type, std::unique_ptr<node_added_event_data> data)
{
    Event event;
    event.type = type;
    event.data = std::move(data);
    event_queue_.push(std::move(event));
}

void zwave_component::network_monitor_handler::post_event(EventType type, std::unique_ptr<node_id_assigned_event_data> data)
{
    Event event;
    event.type = type;
    event.data = std::move(data);
    event_queue_.push(std::move(event));
}

void zwave_component::network_monitor_handler::post_event(EventType type, attribute_store_node_t node)
{
    Event event;
    event.type = type;
    event.data = node;
    event_queue_.push(std::move(event));
}

void zwave_component::network_monitor_handler::post_event(EventType type, zwave_node_id_t node_id)
{
    Event event;
    event.type = type;
    event.data = node_id;
    event_queue_.push(std::move(event));
}

uint8_t *zwave_component::network_monitor_handler::get_cached_current_node_list()
{
    return current_node_list_;
}

void zwave_component::network_monitor_handler::run()
{
    // Process events from the queue with a timeout
    auto event_opt = event_queue_.pop(100);  // 100ms timeout
    if (!event_opt.has_value()) {
        return;  // Timeout, check should_stop() next iteration
    }

    Event event = std::move(event_opt.value());

    switch (event.type) {
        case EventType::NETWORK_READY: {
            if (auto *data = std::get_if<std::unique_ptr<network_data>>(&event.data)) {
                if (*data) {
                    handle_event_network_ready(**data);
                }
            }
            break;
        }

        case EventType::NODE_ID_ASSIGNED: {
            if (auto *data = std::get_if<std::unique_ptr<node_id_assigned_event_data>>(&event.data)) {
                if (*data) {
                    handle_event_node_id_assigned(**data);
                }
            }
            break;
        }

        case EventType::NODE_ADDED: {
            if (auto *data = std::get_if<std::unique_ptr<node_added_event_data>>(&event.data)) {
                if (*data) {
                    handle_event_node_added(**data);
                }
            }
            break;
        }

        case EventType::NODE_INTERVIEW_INITIATED: {
            if (auto *node = std::get_if<attribute_store_node_t>(&event.data)) {
                handle_event_node_interview_initiated(*node);
            }
            break;
        }

        case EventType::NODE_INTERVIEW_DONE: {
            if (auto *node = std::get_if<attribute_store_node_t>(&event.data)) {
                handle_event_node_interview_done(*node);
            }
            break;
        }

        case EventType::NODE_DELETED: {
            if (auto *node_id = std::get_if<zwave_node_id_t>(&event.data)) {
                handle_event_node_deleted(*node_id);
            }
            break;
        }

        case EventType::NODE_FRAME_TRANSMISSION_FAILED: {
            if (auto *node_id = std::get_if<zwave_node_id_t>(&event.data)) {
                handle_event_failed_frame_transmission(*node_id);
            }
            break;
        }

        case EventType::NODE_FRAME_TRANSMISSION_SUCCESS:
        case EventType::NODE_FRAME_RX: {
            if (auto *node_id = std::get_if<zwave_node_id_t>(&event.data)) {
                handle_event_success_frame_transmission(*node_id);
            }
            break;
        }
    }
}

// C callback wrappers
static void network_monitor_on_frame_transmission(bool transmission_successful, const zwapi_tx_report_t *tx_report, zwave_node_id_t node_id)
{
    if (transmission_successful) {
        if (tx_report != nullptr) {
            attribute_store_node_t node_id_node = attribute_store_network_helper_get_zwave_node_id_node(node_id);
            if (node_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
                attribute_store_set_child_reported(node_id_node, ATTRIBUTE_LAST_ROUTING_PATH, tx_report->last_route_repeaters, MAX_REPEATERS);
                attribute_store_set_child_reported(node_id_node, ATTRIBUTE_LAST_TX_TICKS, &tx_report->transmit_ticks, sizeof(tx_report->transmit_ticks));
                attribute_store_set_child_reported(node_id_node, ATTRIBUTE_LAST_NUMBER_OF_REPEATERS, &tx_report->number_of_repeaters, sizeof(tx_report->number_of_repeaters));
                attribute_store_set_child_reported(node_id_node, ATTRIBUTE_LAST_TX_POWER, &tx_report->tx_power, sizeof(tx_report->tx_power));
            }
        }
        network_monitor_on_frame_transmission_success(node_id);
    } else {
        network_monitor_on_frame_transmission_failed(node_id);
    }
}

static void network_monitor_on_node_id_assigned(zwave_node_id_t node_id, bool included_by_us, zwave_protocol_t inclusion_protocol)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    auto event_data                = std::make_unique<node_id_assigned_event_data>();
    event_data->node_id            = node_id;
    event_data->inclusion_protocol = inclusion_protocol;
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_ID_ASSIGNED, std::move(event_data));
}

static void network_monitor_on_nif_updated(attribute_store_node_t updated_node, attribute_store_change_t change)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    if (change == ATTRIBUTE_DELETED) {
        return;
    }

    if (attribute_store_is_reported_defined(updated_node)) {
        return;
    }

    // Fresh NIF created/updated but still undefined, it means we are interviewing.
    attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(updated_node, ATTRIBUTE_NODE_ID);
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_INTERVIEW_INITIATED, node_id_node);
}

static void network_monitor_on_node_deleted(zwave_node_id_t node_id)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_DELETED, node_id);
}

static void network_monitor_on_node_added(sl_status_t status, const zwave_node_info_t *nif, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    auto event_data = std::make_unique<node_added_event_data>();
    // Copy all the data about this new added node.
    memcpy(event_data->dsk, dsk, sizeof(zwave_dsk_t));
    memcpy(&(event_data->nif), nif, sizeof(zwave_node_info_t));
    event_data->node_id            = node_id;
    event_data->granted_keys       = granted_keys;
    event_data->kex_fail_type      = kex_fail_type;
    event_data->inclusion_protocol = inclusion_protocol;
    event_data->status             = status;

    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_ADDED, std::move(event_data));
}

/**
 * @brief Generates an event indicating that the Network Addressing has
 * changed.
 *
 * @param home_id       Our current Z-Wave HomeID.
 * @param node_id       Our current Z-Wave NodeID.
 */
static void network_monitor_on_network_address_update(zwave_home_id_t home_id, zwave_node_id_t node_id)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    network_data data  = {};
    data.home_id       = home_id;
    data.node_id       = node_id;
    data.granted_keys  = (zwave_keyset_t)0;
    data.kex_fail_type = (zwave_kex_fail_type_t)0;
    g_network_monitor_instance->on_network_address_update_internal(&data);
}

/**
 * @brief Generates an event indicating that the Network is ready to be operated
 *
 * @param home_id           Our current Z-Wave HomeID.
 * @param node_id           Our current Z-Wave NodeID.
 * @param granted_keys      Our granted keys in this network.
 * @param kex_fail_type     Our KEX fail type in this network.
 */
static void network_monitor_on_network_ready(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    auto data           = std::make_unique<network_data>();
    data->node_id       = node_id;
    data->home_id       = home_id;
    data->granted_keys  = granted_keys;
    data->kex_fail_type = kex_fail_type;
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NETWORK_READY, std::move(data));
}

static void network_monitor_on_frame_transmission_failed(zwave_node_id_t node_id)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_FRAME_TRANSMISSION_FAILED, node_id);
}

static void network_monitor_on_frame_transmission_success(zwave_node_id_t node_id)
{
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_FRAME_TRANSMISSION_SUCCESS, node_id);
}

// On frame received callback handler
static void network_monitor_on_frame_received(zwave_node_id_t node_id, const uint8_t *frame_data, uint16_t frame_length)
{
    (void)frame_data;
    (void)frame_length;
    if (g_network_monitor_instance == nullptr) {
        return;
    }
    // ZPC received a frame from a given node. If this node is in the failing list,
    // we remove the node from failing list
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_FRAME_RX, node_id);
}

// On application frame received: store last RX RSSI for the source node
static void network_monitor_on_application_frame_received(const zwave_controller_connection_info_t *connection_info, const zwave_rx_receive_options_t *rx_options, const uint8_t *frame_data, uint16_t frame_length)
{
    (void)frame_data;
    (void)frame_length;
    if (connection_info == nullptr || rx_options == nullptr) {
        return;
    }
    zwave_node_id_t source_node_id      = connection_info->remote.node_id;
    attribute_store_node_t node_id_node = attribute_store_network_helper_get_zwave_node_id_node(source_node_id);
    if (node_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }
    attribute_store_set_child_reported(node_id_node, ATTRIBUTE_LAST_RX_RSSI, &rx_options->rssi, sizeof(rx_options->rssi));
}

void zwave_component::network_monitor_handler::create_attribute_store_network_nodes(zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type)
{
    // Make sure we have the latest node list:
    zwave_network_management_get_network_node_list(current_node_list_);

    for (zwave_node_id_t node_id = ZW_MIN_NODE_ID; node_id <= ZW_LR_MAX_NODE_ID; node_id++) {
        if (!ZW_IS_NODE_IN_MASK(node_id, current_node_list_)) {
            continue;
        }

        const bool zpc_node = (node_id == zwave_network_management_get_node_id());
        // Create the node, set NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL for ZPC node,
        // NETWORK_MONITOR_NETWORK_STATUS_COMMISIONING_STARTED for all end devices
        attribute attr_node_id_node = network_monitor_add_attribute_store_node(node_id, zpc_node ? NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL : NETWORK_MONITOR_NETWORK_STATUS_COMMISIONING_STARTED);
        // If it's our own NodeID, make sure to have our granted keys saved
        if (zpc_node) {
            // Configure our Granted keys and KEX Fail type.
            attribute_store_set_child_reported(attr_node_id_node, ATTRIBUTE_GRANTED_SECURITY_KEYS, &granted_keys, sizeof(granted_keys));
            attribute_store_set_child_reported(attr_node_id_node, ATTRIBUTE_KEX_FAIL_TYPE, &kex_fail_type, sizeof(kex_fail_type));
        } else {
            // Make sure everything we need is under the NodeID:
            attribute_store_add_if_missing(attr_node_id_node, node_id_additional_attributes, COUNT_OF(node_id_additional_attributes));

            // Create the non-secure NIF attribute under EP0 if it is missing
            attribute attr_endpoint0 = attr_node_id_node.child_by_type(ATTRIBUTE_ENDPOINT_ID, 0);
            if (attr_endpoint0.child_by_type(ATTRIBUTE_ZWAVE_NIF, 0) == ATTRIBUTE_STORE_INVALID_NODE) {
                attr_endpoint0.add_node(ATTRIBUTE_ZWAVE_NIF);
            }
        }
    }
}

void zwave_component::network_monitor_handler::pause_nl_nodes_resolution(attribute_store_node_t current_network_node)
{
    uint32_t node_id_node_index         = 0;
    attribute_store_node_t node_id_node = attribute_store_get_node_child_by_type(current_network_node, ATTRIBUTE_NODE_ID, node_id_node_index);
    zwave_node_id_t node_id             = 0;
    while (ATTRIBUTE_STORE_INVALID_NODE != node_id_node) {
        node_id = 0;
        attribute_store_read_value(node_id_node, REPORTED_ATTRIBUTE, &node_id, sizeof(zwave_node_id_t));
        if (OPERATING_MODE_NL == zwave_get_operating_mode(node_id)) {
            sl_log_debug(LOG_TAG, "Pausing attribute resolution for NL node: NodeID %d", node_id);
            attribute_resolver_pause_node_resolution(node_id_node);
        }
        node_id_node_index++;
        node_id_node = attribute_store_get_node_child_by_type(current_network_node, ATTRIBUTE_NODE_ID, node_id_node_index);
    }
}

void zwave_component::network_monitor_handler::activate_network_resolution(bool resolve_our_network)
{
    zwave_home_id_t current_home_id             = zwave_network_management_get_home_id();
    uint8_t home_id_node_index                  = 0;
    attribute_store_node_t root                 = attribute_store_get_root();
    attribute_store_node_t current_network_node = attribute_store_get_node_child_by_value(root, ATTRIBUTE_HOME_ID, REPORTED_ATTRIBUTE, (uint8_t *)&current_home_id, sizeof(current_home_id), 0);
    attribute_store_node_t network_node         = attribute_store_get_node_child_by_type(root, ATTRIBUTE_HOME_ID, home_id_node_index);
    home_id_node_index++;

    // Pause the resolutions for all foreign networks
    while (ATTRIBUTE_STORE_INVALID_NODE != network_node) {
        attribute_resolver_pause_node_resolution(network_node);
        sl_log_debug(LOG_TAG, "Pausing HomeID Network resolution. (Attribute ID %d)", network_node);
        if ((network_node == current_network_node) && (resolve_our_network)) {
            attribute_resolver_resume_node_resolution(network_node);
            sl_log_debug(LOG_TAG, "Resuming HomeID Network resolution. (Attribute ID %d)", network_node);
        }
        network_node = attribute_store_get_node_child_by_type(root, ATTRIBUTE_HOME_ID, home_id_node_index);
        home_id_node_index++;
    }

    // Then look at our network... But before we enable resolution, ensure
    // that NL nodes are paused, else we will send commands to sleeping nodes
    pause_nl_nodes_resolution(current_network_node);
}

void zwave_component::network_monitor_handler::remove_attribute_store_home_id(zwave_home_id_t old_home_id)
{
    sl_log_debug(LOG_TAG, "Removing HomeID %08X from the Attribute Store.", old_home_id);

    attribute_store_node_t home_id_node = attribute_store_network_helper_get_home_id_node(old_home_id);
    if (home_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }

    // Delete the node
    attribute_store_delete_node(home_id_node);
}

void zwave_component::network_monitor_handler::remove_attribute_store_node(zwave_node_id_t node_id)
{
    sl_log_debug(LOG_TAG, "Removing NodeID %d from the Attribute Store.", node_id);
    // Find out attribute store node based on the zwave_node_id_t
    attribute_store_node_t node_id_node = attribute_store_network_helper_get_zwave_node_id_node(node_id);

    // Delete the node
    attribute_store_delete_node(node_id_node);
}

attribute zwave_component::network_monitor_handler::network_monitor_add_attribute_store_node(zwave_node_id_t node_id, NetworkMonitorNetworkStatus network_status)
{
    sl_log_debug(LOG_TAG,
                 "Making sure that NodeID %d (with endpoint 0) "
                 "is in the Attribute Store.",
                 node_id);
    attribute_store_node_t node_id_node = attribute_store_network_helper_ensure_zwave_node_placeholder(node_id);

    // Add the network monitor group node for the node
    // This group node is used to store all the network monitor attributes for the node
    attribute_store_node_t network_monitor_group_node = attribute_store_get_node_child_by_type(node_id_node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
    if (network_monitor_group_node == ATTRIBUTE_STORE_INVALID_NODE) {
        // This check is important to avoid adding the node multiple times to the attribute store. (e.g: ZPC restart)
        network_monitor_group_node = attribute_store_add_node(network_monitor_attributes_t::NETWORK_MONITOR_GROUP, node_id_node);
    }

    // Set the network status under the group node (only if not already present)
    attribute_store_node_t network_status_node = attribute_store_get_first_child_by_type(network_monitor_group_node, network_monitor_attributes_t::network_status);
    if (network_status_node == ATTRIBUTE_STORE_INVALID_NODE) {
        attribute_store_set_child_reported(network_monitor_group_node, network_monitor_attributes_t::network_status, &network_status, sizeof(network_status));
    }

    return attribute(node_id_node);
}

static NetworkMonitorNetworkStatus attribute_store_network_helper_get_network_status(attribute_store_node_t node)
{
    // Default to UNAVAILABLE if the value is undefined in the attribute store
    NetworkMonitorNetworkStatus network_status = NETWORK_MONITOR_NETWORK_STATUS_UNAVAILABLE;

    attribute_store_node_t group_node          = attribute_store_get_node_child_by_type(node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
    attribute_store_node_t network_status_node = attribute_store_get_first_child_by_type(group_node, network_monitor_attributes_t::network_status);
    attribute_store_get_reported(network_status_node, &network_status, sizeof(network_status));

    return network_status;
}

/**
 * @brief Ask device_interviewer to (re-)run the InterviewStateMachine for a node.
 *
 * Used after interview failure when the node becomes reachable again (AL/FL TX/RX
 * or NL Wake Up Notification). Must fire NODE_INTERVIEW_REQUESTED — not the legacy
 * ucl_mqtt_initiate_node_interview undefine path, which only clears Get-rule attributes.
 */
static void network_monitor_request_node_interview(zwave_node_id_t node_id)
{
    using namespace zwave_command_class;

    component_connector_node_interview_requested_payload_t payload;
    payload.node_id = node_id;

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_INTERVIEW_REQUESTED), payload);
}

/**
 * @brief Makes the Network status transition to Offline for a node
 *
 * If the node is interviewing, it will be placed in "offline interview failed",
 * else just offline.
 *
 * @param node_id_node Attribute Store Node for the NodeID
 */
void zwave_component::network_monitor_handler::mark_node_as_offline(attribute_store_node_t node_id_node)
{
    zwave_node_id_t node_id = 0;
    attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));
    sl_log_debug(LOG_TAG, "NodeID %d is now considered as failing/offline", node_id);

    NetworkMonitorNetworkStatus network_status = attribute_store_network_helper_get_network_status(node_id_node);
    NetworkMonitorNetworkStatus new_status     = NETWORK_MONITOR_NETWORK_STATUS_OFFLINE;
    if (network_status == NETWORK_MONITOR_NETWORK_STATUS_ONLINE_INTERVIEWING) {
        // If the network status was interviewing and the frame transmission failed
        // Set it to Failed interview, so we try again a ful interview when it responds again
        new_status = NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL;
    }
    attribute_store_node_t group_node = attribute_store_get_node_child_by_type(node_id_node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
    attribute_store_set_child_reported(group_node, network_monitor_attributes_t::network_status, &new_status, sizeof(new_status));
    if (network_status != new_status) {
        network_monitor::NetworkMonitorMqttApi::publish_network_status(node_id, new_status);
    }
}

void zwave_component::network_monitor_handler::mark_node_as_online(attribute_store_node_t node_id_node)
{
    NetworkMonitorNetworkStatus network_status = attribute_store_network_helper_get_network_status(node_id_node);

    // Don't modify anything if the node is not offline / non-functional.
    if (network_status != NETWORK_MONITOR_NETWORK_STATUS_OFFLINE && network_status != NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL) {
        return;
    }

    zwave_node_id_t node_id = 0;
    attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));

    // NL failed interviews stay NON_FUNCTIONAL until Wake Up Notification (see
    // on_wake_up_notification_received). Generic TX/RX must not re-arm interviewing
    // (races with stall-abort callbacks and blocks SmartStart).
    if (network_status == NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL && OPERATING_MODE_NL == zwave_get_operating_mode(node_id)) {
        return;
    }

    NetworkMonitorNetworkStatus new_status = NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL;
    if (network_status == NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL) {
        // AL/FL: re-interview when the node responds again after interview failure.
        new_status = NETWORK_MONITOR_NETWORK_STATUS_ONLINE_INTERVIEWING;
        network_monitor_request_node_interview(node_id);
    }
    attribute_store_node_t group_node = attribute_store_get_node_child_by_type(node_id_node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
    attribute_store_set_child_reported(group_node, network_monitor_attributes_t::network_status, &new_status, sizeof(new_status));
    network_monitor::NetworkMonitorMqttApi::publish_network_status(node_id, new_status);
}

void zwave_component::network_monitor_handler::update_new_node_attribute_store(const node_added_event_data &node_added_data)
{
    zwave_home_id_t home_id             = zwave_network_management_get_home_id();
    attribute_store_node_t node_id_node = attribute_store_network_helper_create_node_id_node(home_id, node_added_data.node_id);

    // Write down the granted keys for that node
    attribute_store_set_child_reported(node_id_node, ATTRIBUTE_GRANTED_SECURITY_KEYS, &node_added_data.granted_keys, sizeof(zwave_keyset_t));

    // Find the KEX Fail type for that node
    attribute_store_set_child_reported(node_id_node, ATTRIBUTE_KEX_FAIL_TYPE, &node_added_data.kex_fail_type, sizeof(zwave_kex_fail_type_t));

    // Find the DSK for that node if it has S2 capabilities
    if (zwave_security_validation_is_node_s2_capable(node_added_data.node_id)) {
        // Write the S2 DSK for that node
        attribute_store_set_child_reported(node_id_node, ATTRIBUTE_S2_DSK, node_added_data.dsk, sizeof(zwave_dsk_t));
    }

    // Find the protocol listening byte from the NIF
    attribute_store_set_child_reported(node_id_node, ATTRIBUTE_ZWAVE_PROTOCOL_LISTENING, &node_added_data.nif.listening_protocol, sizeof(node_added_data.nif.listening_protocol));

    // Find the optional protocol byte from the NIF
    attribute_store_set_child_reported(node_id_node, ATTRIBUTE_ZWAVE_OPTIONAL_PROTOCOL, &node_added_data.nif.optional_protocol, sizeof(node_added_data.nif.optional_protocol));

    // Undefined NIF triggers interview detection. Security-failed adds skip interview
    // and self-destruct — do not create a NIF placeholder that would flip ONLINE_INTERVIEWING.
    if (node_add_succeeded(node_added_data)) {
        attribute_store_node_t endpoint_id_node = attribute_store_network_helper_get_endpoint_node(home_id, node_added_data.node_id, 0);
        attribute_store_create_child_if_missing(endpoint_id_node, ATTRIBUTE_ZWAVE_NIF);
    }
}

void zwave_component::network_monitor_handler::update_all_network_statuses(attribute_store_node_t home_id_node, NetworkMonitorNetworkStatus old_value, NetworkMonitorNetworkStatus new_value)
{
    uint32_t node_id_index              = 0;
    attribute_store_node_t node_id_node = attribute_store_get_node_child_by_type(home_id_node, ATTRIBUTE_NODE_ID, node_id_index);
    node_id_index += 1;

    while (node_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
        attribute_store_node_t group_node = attribute_store_get_node_child_by_type(node_id_node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
        if (group_node != ATTRIBUTE_STORE_INVALID_NODE) {
            attribute_store_node_t status_node = attribute_store_get_node_child_by_value(group_node, network_monitor_attributes_t::network_status, REPORTED_ATTRIBUTE, (uint8_t *)(&old_value), sizeof(old_value), 0);
            attribute_store_set_reported(status_node, &new_value, sizeof(new_value));
        }
        node_id_node = attribute_store_get_node_child_by_type(home_id_node, ATTRIBUTE_NODE_ID, node_id_index);
        node_id_index += 1;
    }
}

// Handler Functions for events
void zwave_component::network_monitor_handler::on_network_address_update_internal(network_data *event_data)
{
    zwave_home_id_t const old_home_id = zpc_home_id_;
    remove_attribute_store_home_id(old_home_id);

    zpc_home_id_ = event_data->home_id;
    zpc_node_id_ = event_data->node_id;

    // Clear all the static cache for the network
    failed_transmission_data_.clear();

    // Prep the attribute store with our new address, create our keys and KEX fail.
    create_attribute_store_network_nodes(event_data->granted_keys, event_data->kex_fail_type);

    // Pause any network resolution, we have to wait for the network to be ready
    activate_network_resolution(false);

    // Let the component know that we are in a valid network now.
    network_initialized_ = true;
}

void zwave_component::network_monitor_handler::handle_event_network_address_update(network_data *event_data)
{
    // This is called from the event queue, so we don't need to remove old home_id here
    zpc_home_id_ = event_data->home_id;
    zpc_node_id_ = event_data->node_id;

    // Clear all the static cache for the network
    failed_transmission_data_.clear();

    // Prep the attribute store with our new address, create our keys and KEX fail.
    create_attribute_store_network_nodes(event_data->granted_keys, event_data->kex_fail_type);

    // Pause any network resolution, we have to wait for the network to be ready
    activate_network_resolution(false);

    // Let the component know that we are in a valid network now.
    network_initialized_ = true;
}

void zwave_component::network_monitor_handler::handle_event_network_ready(const network_data &event_data)
{
    zpc_home_id_ = event_data.home_id;
    zpc_node_id_ = event_data.node_id;

    // Save our updated granted keys/KEX fail
    create_attribute_store_network_nodes(event_data.granted_keys, event_data.kex_fail_type);

    // Pause node resolution on any other network than ours in the Attribute Store.
    activate_network_resolution(true);
}

void zwave_component::network_monitor_handler::handle_event_node_id_assigned(const node_id_assigned_event_data &event_data)
{
    network_monitor_add_attribute_store_node(event_data.node_id, NETWORK_MONITOR_NETWORK_STATUS_COMMISIONING_STARTED);
    zwave_store_inclusion_protocol(event_data.node_id, event_data.inclusion_protocol);
}

void zwave_component::network_monitor_handler::handle_event_node_added(const node_added_event_data &event_data)
{
    // Attribute store node should already exist, but in case NODE_ID_ASSIGNED_EVENT
    // did not happen before this event, we ensure the node exists in the attribute store.
    update_new_node_attribute_store(event_data);

    zwave_store_inclusion_protocol(event_data.node_id, event_data.inclusion_protocol);

    // Finally we want to update our local cache of the node list:
    zwave_network_management_get_network_node_list(current_node_list_);
    // Interview stall abort is owned by device_interviewer and surfaced via
    // COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED (non-OK status → ONLINE_NON_FUNCTIONAL).
}

void zwave_component::network_monitor_handler::handle_event_node_interview_initiated(attribute_store_node_t node_id_node)
{
    // Security-failed ghosts must not enter ONLINE_INTERVIEWING (matches interviewer skip).
    zwave_kex_fail_type_t kex_fail_type  = ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE;
    attribute_store_node_t kex_fail_node = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_KEX_FAIL_TYPE, 0);
    if (kex_fail_node != ATTRIBUTE_STORE_INVALID_NODE) {
        attribute_store_get_reported(kex_fail_node, &kex_fail_type, sizeof(kex_fail_type));
    }
    if (kex_fail_type != ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE) {
        return;
    }

    attribute_store_node_t group_node          = attribute_store_get_node_child_by_type(node_id_node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
    attribute_store_node_t network_status_node = attribute_store_get_first_child_by_type(group_node, network_monitor_attributes_t::network_status);
    NetworkMonitorNetworkStatus network_status = NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL;
    attribute_store_get_reported(network_status_node, &network_status, sizeof(network_status));

    if (network_status == NETWORK_MONITOR_NETWORK_STATUS_OFFLINE) {
        network_status = NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL;
    } else {
        network_status = NETWORK_MONITOR_NETWORK_STATUS_ONLINE_INTERVIEWING;
    }

    // Network status will also be created, if it was not here.
    attribute_store_set_child_reported(group_node, network_monitor_attributes_t::network_status, &network_status, sizeof(network_status));
}

void zwave_component::network_monitor_handler::handle_event_node_interview_done(attribute_store_node_t node_id_node)
{
    handle_event_node_interview_done(node_id_node, SL_STATUS_OK);
}

void zwave_component::network_monitor_handler::handle_event_node_interview_done(attribute_store_node_t node_id_node, sl_status_t interview_status)
{
    NetworkMonitorNetworkStatus network_status = (interview_status == SL_STATUS_OK) ? NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL : NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL;
    attribute_store_node_t group_node          = attribute_store_get_node_child_by_type(node_id_node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
    attribute_store_set_child_reported(group_node, network_monitor_attributes_t::network_status, &network_status, sizeof(network_status));
}

void zwave_component::network_monitor_handler::update_last_received_frame_timestamp(attribute_store_node_t node_id_node)
{
    auto now     = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    attribute_store_set_child_reported(node_id_node, ATTRIBUTE_LAST_RECEIVED_FRAME_TIMESTAMP, &seconds, sizeof(seconds));
}

void zwave_component::network_monitor_handler::handle_event_node_deleted(zwave_node_id_t node_id)
{
    if (node_id < ZW_MIN_NODE_ID) {
        return;
    }

    // Remove the node from the attribute store
    remove_attribute_store_node(node_id);

    // Cleaning data structures that contains the zwave_node_id key
    auto it_failed_transmission = failed_transmission_data_.find(node_id);
    if (it_failed_transmission != failed_transmission_data_.end()) {
        failed_transmission_data_.erase(it_failed_transmission);
    }
    sl_log_debug(LOG_TAG, "Removing NodeID %d from all the associations in network.", node_id);
    remove_desired_node_id_from_all_associations_in_network(node_id);
}

void zwave_component::network_monitor_handler::handle_event_failed_frame_transmission(zwave_node_id_t node_id)
{
    zwave_operating_mode_t operating_mode = zwave_get_operating_mode(node_id);
    attribute_store_node_t node_id_node   = attribute_store_network_helper_get_zwave_node_id_node(node_id);

    // Node does not exist, did we try to transmit to a non-existing node?
    if (node_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
        sl_log_debug(LOG_TAG,
                     "Warning: transmission failure with a non-existing "
                     "NodeID %d from the Attribute Store. This should not happen.",
                     node_id);
        return;
    }

    // Sleeping node: consider them asleep again if communication failed
    // and we did not manage to talk to them in the last 10s.
    if (operating_mode == OPERATING_MODE_NL) {
        if (network_monitor_is_node_asleep_due_to_inactivity(node_id_node, 10)) {
            sl_log_debug(LOG_TAG, "NodeID %d is now considered asleep", node_id);
            attribute_resolver_pause_node_resolution(node_id_node);
        }
        return;
    }

    uint8_t &failed_transmission_count = failed_transmission_data_[node_id];
    failed_transmission_count++;
    // Check if we are within the accepted number of failures.
    if (failed_transmission_count < zpc_get_config()->accepted_transmit_failure) {
        return;
    }

    // Mark the node as offline:
    mark_node_as_offline(node_id_node);
    attribute_resolver_pause_node_resolution(node_id_node);
}

void zwave_component::network_monitor_handler::handle_event_success_frame_transmission(zwave_node_id_t node_id)
{
    // Gather information about the node:
    attribute_store_node_t node_id_node = attribute_store_network_helper_get_zwave_node_id_node(node_id);
    // Save that we got a successful transmission.
    update_last_received_frame_timestamp(node_id_node);

    // Non-Sleeping nodes
    auto it = failed_transmission_data_.find(node_id);
    if (it != failed_transmission_data_.end()) {
        failed_transmission_data_.erase(it);
        // Failing node monitor does not monitor NL nodes so no need of stopping
        if (OPERATING_MODE_NL != zwave_get_operating_mode(node_id)) {
            attribute_resolver_resume_node_resolution(node_id_node);
        }
    }

    // For NL nodes, restart the offline timer since we just heard from them
    if (OPERATING_MODE_NL == zwave_get_operating_mode(node_id)) {
        component_connector connector;
        wake_up_interval_requested_payload_t request;
        request.device_endpoint_node = attribute_store::attribute(node_id_node);

        auto future                             = connector.fire_event_async<wake_up_interval_requested_payload_t, uint32_t>(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_REQUESTED), request);
        auto [status, wake_up_interval_seconds] = future.get();

        if (status == SL_STATUS_OK && wake_up_interval_seconds > 0) {
            start_or_restart_nl_offline_timer(node_id_node, wake_up_interval_seconds);
        }
    }

    // In any case, mark the node as online when we tx or rx successfully.
    mark_node_as_online(node_id_node);
}

////////////////////////////////////////////////////////////////////////////////
// NL node offline monitoring
////////////////////////////////////////////////////////////////////////////////

static void network_monitor_on_interview_fully_resolved(const zwave_command_class::component_connector_interview_done_payload_t &payload)
{
    if (g_network_monitor_instance == nullptr) {
        sl_log_debug(LOG_TAG, "Network monitor instance is nullptr, ignoring interview fully resolved");
        return;
    }
    attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(payload.endpoint_node, ATTRIBUTE_NODE_ID);
    if (node_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
        sl_log_debug(LOG_TAG, "No NodeID parent for endpoint node %d, ignoring interview fully resolved", payload.endpoint_node);
        return;
    }
    // Stall/fail aborts publish non-OK status and must clear ONLINE_INTERVIEWING here.
    // Success can also arrive via the attribute-resolver listener / NODE_INTERVIEW_DONE event.
    if (payload.status != SL_STATUS_OK) {
        zwave_component::network_monitor_handler::handle_event_node_interview_done(node_id_node, payload.status);
        return;
    }
    g_network_monitor_instance->post_event(zwave_component::network_monitor_handler::EventType::NODE_INTERVIEW_DONE, node_id_node);

    // NL: interview (and post-interview CC resolutions) are done — stop keep-alive
    // via status change above, then arm No More Information so the device can sleep.
    zwave_node_id_t node_id = 0;
    attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));
    if (OPERATING_MODE_NL == zwave_get_operating_mode(node_id)) {
        sl_log_debug(LOG_TAG, "Interview fully resolved for NL NodeID %d — arming No More Information", node_id);
        component_connector connector;
        wake_up_arm_no_more_information_payload_t arm_payload;
        arm_payload.device_node_id_node = attribute(node_id_node);
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_ARM_NO_MORE_INFORMATION), arm_payload);
    }
}

void zwave_component::network_monitor_handler::register_component_connector_handlers()
{
    component_connector connector;
    connector.connect_typed<command_class_wake_up_events_t, wake_up_interval_report_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_INTERVAL_REPORT_RECEIVED, [](const wake_up_interval_report_payload_t &p) {
        on_wake_up_interval_report_received(p);
        return SL_STATUS_OK;
    });
    connector.connect_typed<command_class_wake_up_events_t, wake_up_notification_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_NOTIFICATION_RECEIVED, [](const wake_up_notification_payload_t &p) {
        on_wake_up_notification_received(p);
        return SL_STATUS_OK;
    });
    connector.connect_typed<command_class_wake_up_events_t, wake_up_no_more_information_sent_payload_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_NO_MORE_INFORMATION_SENT, [](const wake_up_no_more_information_sent_payload_t &p) {
        on_wake_up_no_more_information_sent(p);
        return SL_STATUS_OK;
    });
    connector.connect_typed<zwave_command_class::component_connector_common_events_t, zwave_command_class::component_connector_interview_done_payload_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED,
                                                                                                                                                         [](const zwave_command_class::component_connector_interview_done_payload_t &payload) -> sl_status_t {
                                                                                                                                                             network_monitor_on_interview_fully_resolved(payload);
                                                                                                                                                             return SL_STATUS_OK;
                                                                                                                                                         });
}

void zwave_component::network_monitor_handler::on_wake_up_interval_report_received(const wake_up_interval_report_payload_t &payload)
{
    attribute_store_node_t endpoint_node = payload.device_endpoint_node;
    attribute_store_node_t node_id_node  = attribute_store_get_first_parent_with_type(endpoint_node, ATTRIBUTE_NODE_ID);

    zwave_node_id_t node_id = 0;
    attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));

    if (OPERATING_MODE_NL != zwave_get_operating_mode(node_id)) {
        return;
    }

    sl_log_debug(LOG_TAG, "Wake Up Interval Report received for NL NodeID %d, scheduling offline timer", node_id);
    start_or_restart_nl_offline_timer(node_id_node, payload.seconds);
}

void zwave_component::network_monitor_handler::on_wake_up_notification_received(const wake_up_notification_payload_t &payload)
{
    attribute_store_node_t endpoint_node = payload.device_endpoint_node;
    attribute_store_node_t node_id_node  = attribute_store_get_first_parent_with_type(endpoint_node, ATTRIBUTE_NODE_ID);

    zwave_node_id_t node_id = 0;
    attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));

    if (OPERATING_MODE_NL != zwave_get_operating_mode(node_id)) {
        return;
    }

    NetworkMonitorNetworkStatus network_status = attribute_store_network_helper_get_network_status(node_id_node);
    if (network_status == NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL) {
        const NetworkMonitorNetworkStatus new_status = NETWORK_MONITOR_NETWORK_STATUS_ONLINE_INTERVIEWING;
        attribute_store_node_t group_node            = attribute_store_get_node_child_by_type(node_id_node, network_monitor_attributes_t::NETWORK_MONITOR_GROUP, 0);
        attribute_store_set_child_reported(group_node, network_monitor_attributes_t::network_status, &new_status, sizeof(new_status));
        network_monitor::NetworkMonitorMqttApi::publish_network_status(node_id, new_status);
        sl_log_debug(LOG_TAG, "Wake Up Notification for NL NodeID %d after failed interview — starting re-interview", node_id);
        network_monitor_request_node_interview(node_id);
        network_status = new_status;
    }

    // Reopen exhausted Gets so unfinished resolution (post-interview) or a
    // resumed interview after an unexpected sleep can progress this wake window.
    attribute_resolver_restart_exhausted_get_resolutions(node_id_node);

    sl_log_debug(LOG_TAG, "Wake Up Notification received for NL NodeID %d, resuming resolution", node_id);
    sl_log_debug(LOG_TAG, "Wakeup resume call: node_id=%d node_id_node=%d tid=%lu", node_id, node_id_node, sl_log_thread_id());
    attribute_resolver_resume_node_resolution(node_id_node);

    // While interviewing, keep-alive NOPs hold the device awake — do not send
    // No More Information until the interview fully resolves. After that, arm
    // WUNMI so pending Gets can run this wake window and the node can sleep.
    if (network_status != NETWORK_MONITOR_NETWORK_STATUS_ONLINE_INTERVIEWING) {
        component_connector connector;
        wake_up_arm_no_more_information_payload_t arm_payload;
        arm_payload.device_node_id_node = attribute(node_id_node);
        connector.fire_event(static_cast<uint32_t>(command_class_wake_up_events_t::COMMAND_CLASS_WAKE_UP_ARM_NO_MORE_INFORMATION), arm_payload);
    }
}

void zwave_component::network_monitor_handler::on_wake_up_no_more_information_sent(const wake_up_no_more_information_sent_payload_t &payload)
{
    attribute_store_node_t endpoint_node = payload.device_endpoint_node;
    attribute_store_node_t node_id_node  = attribute_store_get_first_parent_with_type(endpoint_node, ATTRIBUTE_NODE_ID);

    zwave_node_id_t node_id = 0;
    attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));

    if (OPERATING_MODE_NL != zwave_get_operating_mode(node_id)) {
        return;
    }

    sl_log_debug(LOG_TAG, "Wake Up No More Information sent for NL NodeID %d, pausing resolution", node_id);
    sl_log_debug(LOG_TAG, "Wakeup pause call: node_id=%d node_id_node=%d tid=%lu", node_id, node_id_node, sl_log_thread_id());
    attribute_resolver_pause_node_resolution(node_id_node);
    sl_log_debug(LOG_TAG, "Wakeup pause done: node_id=%d node_id_node=%d tid=%lu", node_id, node_id_node, sl_log_thread_id());
}

void zwave_component::network_monitor_handler::start_or_restart_nl_offline_timer(attribute_store_node_t node_id_node, uint32_t wake_up_interval_seconds)
{
    clock_time_t timeout_ms = static_cast<clock_time_t>(zpc_get_config()->missing_wake_up_notification) * wake_up_interval_seconds * 1000;

    if (timeout_ms != 0) {
        attribute_timeout_set_callback(node_id_node, timeout_ms, &on_nl_node_offline_timeout);
    } else {
        sl_log_error(LOG_TAG, "Wake up interval is 0 for NL NodeID, skipping offline timer");
    }
}

void zwave_component::network_monitor_handler::on_nl_node_offline_timeout(attribute_store_node_t node_id_node)
{
    if (g_network_monitor_instance == nullptr) {
        sl_log_error(LOG_TAG, "Network monitor instance is nullptr, skipping offline timeout");
        return;
    }

    zwave_node_id_t node_id = 0;
    attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));
    sl_log_debug(LOG_TAG, "NL NodeID %d missed wake-up period, marking as offline", node_id);

    zwave_component::network_monitor_handler::mark_node_as_offline(node_id_node);
    attribute_resolver_pause_node_resolution(node_id_node);
}

////////////////////////////////////////////////////////////////////////////////
// Shared functions
////////////////////////////////////////////////////////////////////////////////
// C API wrapper functions
void network_state_monitor_init()
{
    // This function is kept for backward compatibility but does nothing
    // The handler is now initialized in main.cpp via process_init()
}

uint8_t *network_monitor_get_cached_current_node_list()
{
    if (g_network_monitor_instance == nullptr) {
        return nullptr;
    }
    return g_network_monitor_instance->get_cached_current_node_list();
}

static bool network_monitor_any_end_device_has_status(NetworkMonitorNetworkStatus match)
{
    attribute_store_node_t network_node = get_zpc_network_node();
    if (network_node == ATTRIBUTE_STORE_INVALID_NODE) {
        return false;
    }

    const zwave_node_id_t zpc_node_id   = zwave_network_management_get_node_id();
    uint32_t index                      = 0;
    attribute_store_node_t node_id_node = attribute_store_get_node_child_by_type(network_node, ATTRIBUTE_NODE_ID, index);
    while (node_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
        zwave_node_id_t node_id = 0;
        attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));
        if (node_id != zpc_node_id) {
            const NetworkMonitorNetworkStatus status = attribute_store_network_helper_get_network_status(node_id_node);
            if (status == match) {
                return true;
            }
        }
        index++;
        node_id_node = attribute_store_get_node_child_by_type(network_node, ATTRIBUTE_NODE_ID, index);
    }
    return false;
}

bool network_monitor_is_end_device_inclusion_ongoing(void)
{
    return network_monitor_any_end_device_has_status(NETWORK_MONITOR_NETWORK_STATUS_COMMISIONING_STARTED);
}

bool network_monitor_is_any_end_device_interviewing(void)
{
    return network_monitor_any_end_device_has_status(NETWORK_MONITOR_NETWORK_STATUS_ONLINE_INTERVIEWING);
}
