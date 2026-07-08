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

#include "system_events.hpp"
#include "component_connector.hpp"
#include "component_connector_types.hpp"
#include "component_connector_common_events.hpp"
#include "zwave_controller_callbacks.h"
#include "zwave_network_management.h"
#include "zwave_network_management_remove_failed_report.h"
#include "zwave_command_class_utils.hpp"
#include "zpc_attribute_store_network_helper.h"
#include "zwave_utils.h"
#include "log.h"
#include <string_view>
#include <cstring>

[[maybe_unused]] static constexpr std::string_view LOG_TAG = "system";

std::atomic<bool> system_events::factory_reset_pending {false};

// Constructor
system_events::system_events()
{
    // Constructor does not initialize - initialization happens via initialize() method
}

// Initializable interface
sl_status_t system_events::initialize()
{
    static const zwave_controller_callbacks_t component_connector_callbacks = {
      .on_state_updated                  = nullptr,
      .on_error                          = nullptr,
      .on_node_id_assigned               = &system_events::on_node_id_assigned,
      .on_node_deleted                   = &system_events::on_node_deleted,
      .on_node_added                     = &system_events::on_node_added,
      .on_network_address_update         = &system_events::on_network_address_update,
      .on_new_network_entered            = &system_events::on_new_network_entered,
      .on_keys_report                    = nullptr,
      .on_dsk_report                     = nullptr,
      .on_application_frame_received     = nullptr,
      .on_protocol_frame_received        = nullptr,
      .on_protocol_cc_encryption_request = nullptr,
      .on_smart_start_inclusion_request  = nullptr,
      .on_node_information               = &system_events::on_node_information,
      .on_new_suc                        = nullptr,
      .on_node_info_req_failed           = nullptr,
      .on_multicast_group_deleted        = nullptr,
      .on_request_neighbor_update        = nullptr,
      .on_frame_transmission             = nullptr,
      .on_rx_frame_received              = nullptr,
    };

    sl_status_t status = zwave_controller_register_callbacks(&component_connector_callbacks);
    if (status == SL_STATUS_OK) {
        sl_log_info(LOG_TAG.data(), "System events initialized and Z-Wave callbacks registered");
    } else {
        sl_log_error(LOG_TAG.data(), "Failed to register Z-Wave callbacks: %d", status);
        return status;
    }

    // Register a final reset step that just marks the chain as a factory-reset
    // completion, so on_new_network_entered can fire COMPONENT_CONNECTOR_FACTORY_RESET_COMPLETE
    // only when the new network was reached via a controller reset (not learn mode).
    status = zwave_controller_register_reset_step(&system_events::on_factory_reset_step, ZWAVE_CONTROLLER_FACTORY_RESET_REPORT_STEP_PRIORITY);
    if (status != SL_STATUS_OK) {
        sl_log_error(LOG_TAG.data(), "Failed to register factory-reset notification step: %d", status);
    }
    return status;
}

int system_events::shutdown()
{
    // No cleanup needed - static callbacks remain registered
    return 0;
}

std::string system_events::name() const
{
    return "System Events";
}

// Static callback implementations
void system_events::on_node_information(zwave_node_id_t node_id, const zwave_node_info_t *node_info)
{
    zwave_command_class::component_connector_node_information_received_payload_t payload;
    payload.node_id = node_id;
    // Copy the node_info data instead of storing a pointer
    // This is necessary because fire_event queues the event asynchronously,
    // and the pointer might become invalid by the time the event is processed
    if (node_info != nullptr) {
        payload.node_info = *node_info;
    } else {
        // Initialize to zero if node_info is null
        std::memset(&payload.node_info, 0, sizeof(payload.node_info));
    }

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_INFORMATION_RECEIVED), payload);
}

void system_events::on_node_id_assigned(zwave_node_id_t node_id, bool included_by_us, zwave_protocol_t inclusion_protocol)
{
    if (included_by_us) {
        return;
    }

    // Same placeholder as network_monitor's NODE_ID_ASSIGNED handler; run synchronously here
    // before connector events because network_monitor applies it asynchronously from its queue.
    attribute_store_network_helper_ensure_zwave_node_placeholder(node_id);
    zwave_store_inclusion_protocol(node_id, inclusion_protocol);

    zwave_command_class::component_connector_node_id_assigned_by_other_controller_payload_t payload;
    payload.node_id            = node_id;
    payload.inclusion_protocol = inclusion_protocol;

    sl_log_info(LOG_TAG.data(),
                "NodeID %d assigned while NMS idle (e.g. by another controller): "
                "firing COMPONENT_CONNECTOR_NODE_ID_ASSIGNED_BY_OTHER_CONTROLLER",
                static_cast<int>(node_id));

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_ID_ASSIGNED_BY_OTHER_CONTROLLER), payload);
}

void system_events::on_node_added(sl_status_t status, const zwave_node_info_t *node_info, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol)
{
    zwave_command_class::component_connector_node_added_payload_t payload;
    payload.status = status;
    // Copy the node_info data instead of storing a pointer
    // This is necessary because fire_event queues the event asynchronously,
    // and the pointer might become invalid by the time the event is processed
    if (node_info != nullptr) {
        payload.node_info = *node_info;
    } else {
        // Initialize to zero if node_info is null
        std::memset(&payload.node_info, 0, sizeof(payload.node_info));
    }
    payload.node_id = node_id;
    payload.set_dsk(dsk);
    payload.granted_keys       = granted_keys;
    payload.kex_fail_type      = kex_fail_type;
    payload.inclusion_protocol = inclusion_protocol;

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_ADDED), payload);
}

void system_events::on_network_address_update(zwave_home_id_t home_id, zwave_node_id_t node_id)
{
    zwave_command_class::component_connector_network_address_updated_payload_t payload;
    payload.home_id = home_id;
    payload.node_id = node_id;

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_NETWORK_ADDRESS_UPDATED), payload);
}

void system_events::on_new_network_entered(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type)
{
    zwave_command_class::component_connector_new_network_entered_payload_t payload;
    payload.home_id       = home_id;
    payload.node_id       = node_id;
    payload.granted_keys  = granted_keys;
    payload.kex_fail_type = kex_fail_type;

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_NEW_NETWORK_ENTERED), payload);

    // If on_factory_reset_step ran during this reset chain, the new network was
    // reached via a controller reset (not learn mode). Surface a dedicated event
    // so listeners can publish "factory reset complete" without having to filter
    // learn-mode joins.
    if (factory_reset_pending.exchange(false)) {
        zwave_command_class::component_connector_factory_reset_complete_payload_t reset_payload;
        reset_payload.home_id = home_id;
        reset_payload.node_id = node_id;
        connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_FACTORY_RESET_COMPLETE), reset_payload);
    }
}

sl_status_t system_events::on_factory_reset_step()
{
    // Returning a status other than SL_STATUS_OK tells the reset driver this step
    // is synchronous; the chain advances to the next step (if any) and then clears
    // reset_ongoing. The flag is consumed in on_new_network_entered, which fires
    // shortly after this step inside nm_state_machine.c's NM_EV_SET_DEFAULT_COMPLETE
    // handler.
    factory_reset_pending.store(true);
    return SL_STATUS_NOT_AVAILABLE;
}

void system_events::on_node_exclusion_started(zwave_node_id_t node_id)
{
    zwave_command_class::component_connector_node_deleted_payload_t payload;
    payload.node_id = node_id;
    zwave_command_class::command_class_utils::get_node_dsk(node_id, payload.dsk);

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_DELETED), payload);
}

void system_events::on_node_deleted(zwave_node_id_t node_id)
{
    zwave_command_class::component_connector_node_deleted_payload_t payload;
    payload.node_id = node_id;
    zwave_command_class::command_class_utils::get_node_dsk(node_id, payload.dsk);

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_NODE_DELETED), payload);
}

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t node_id, const char *status)
{
    zwave_command_class::component_connector_node_remove_failed_payload_t payload;
    payload.node_id = node_id;
    payload.reason  = status;

    component_connector connector;
    connector.fire_event(static_cast<uint32_t>(zwave_command_class::component_connector_common_events_t::COMPONENT_CONNECTOR_FAILED_NODE_DELETED), payload);
}
