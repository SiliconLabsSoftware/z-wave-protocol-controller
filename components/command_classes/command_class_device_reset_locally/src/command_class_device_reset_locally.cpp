
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

#include <cstddef>
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>
#include <vector>

// Base class
#include "command_class_device_reset_locally.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"

#include "log.h"

#include "zwave_network_management.h"
#include "zwave_controller_callbacks.h"
#include "zwapi_protocol_basis.h"
#include "zwave_tx_definitions.h"
#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"

// Lifeline destinations are owned by AGI; we query them through the connector.
#include "component_connector.hpp"
#include "command_class_association_grp_info_events.hpp"
#include "command_class_association_grp_info_types.hpp"

namespace zwave_command_class
{
    struct timer_handle_t command_class_device_reset_locally::timer                            = {nullptr};
    std::map<zwave_node_id_t, uint8_t> command_class_device_reset_locally::nodes_to_be_removed = {};
    std::atomic<size_t> command_class_device_reset_locally::pending_reset_notifications {0};

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_device_reset_locally";

    command_class_device_reset_locally::command_class_device_reset_locally()
    {
        static zwave_controller_callbacks_t zwave_command_class_device_reset_callbacks = {
          .on_node_deleted = command_class_device_reset_locally::zwave_command_class_device_reset_on_node_deleted,
        };

        // Register zwave controller callbacks for node delete
        zwave_controller_register_callbacks(&zwave_command_class_device_reset_callbacks);

        // Tell the Z-Wave Controller that we have to do something on reset
        zwave_controller_register_reset_step(&zwave_command_class_device_reset_on_zpc_reset, ZWAVE_CONTROLLER_DEVICE_RESET_LOCALLY_STEP_PRIORITY);

        // Declare to AGI that this CC sends Device Reset Locally Notification via
        // the Lifeline group, so it is advertised in Association Group Command
        // List Report for group 1.
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND),
                             command_class_association_grp_info_types::component_connector_agi_lifeline_command_payload_t {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION});
    }

    void command_class_device_reset_locally::on_reset_notification_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
    {
        // Atomically decrement only when there is still work outstanding,
        // preventing an unsigned underflow if the callback fires unexpectedly
        // (e.g. after the step has already been completed).
        size_t current = pending_reset_notifications.load();
        while (current > 0) {
            if (pending_reset_notifications.compare_exchange_weak(current, current - 1)) {
                if (current == 1) {
                    sl_log_debug(LOG_TAG.data(),
                                 "Reset step: Device Reset Locally Notification "
                                 "to the lifeline destinations completed.");
                    zwave_controller_on_reset_step_complete(ZWAVE_CONTROLLER_DEVICE_RESET_LOCALLY_STEP_PRIORITY);
                }
                return;
            }
        }
    }

    sl_status_t command_class_device_reset_locally::zwave_command_class_device_reset_on_zpc_reset(void)
    {
        sl_log_info(LOG_TAG.data(),
                    "Reset step: Sending a Device Reset Locally Notification "
                    "to the lifeline destinations.");

        nodes_to_be_removed.clear();
        pending_reset_notifications.store(0);

        const uint8_t reset_notification[] = {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION};
        zwave_tx_options_t tx_options      = {};
        tx_options.discard_timeout_ms      = MAXIMUM_TIME_FOR_RESET_NOTIFICATION;
        tx_options.qos_priority            = ZWAVE_TX_QOS_RECOMMENDED_TIMING_CRITICAL_PRIORITY;
        tx_options.number_of_responses     = 0;

        component_connector connector;
        auto lifeline_future = connector.fire_event_async<command_class_association_grp_info_types::component_connector_agi_empty_payload_t, command_class_association_grp_info_types::component_connector_agi_lifeline_destinations_t>(
          static_cast<uint32_t>(command_class_association_grp_info_events_t::COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS),
          {});
        auto [lifeline_status, lifeline] = lifeline_future.get();
        if (lifeline_status != SL_STATUS_OK) {
            sl_log_warning(LOG_TAG.data(), "Reset step: Failed to query AGI lifeline destinations, skipping notification");
            return SL_STATUS_NOT_AVAILABLE;
        }

        // Gather all destinations up front so the pending counter can be set to
        // the final total before any send is issued. Otherwise a completion
        // callback that runs before the counter is incremented could see it at
        // zero and prematurely advance the reset chain.
        std::vector<zwave_controller_connection_info_t> destinations;

        for (const auto &dest_node_id: lifeline.node_ids) {
            zwave_controller_connection_info_t connection = {};
            zwave_tx_scheme_get_node_connection_info(dest_node_id, 0, &connection);
            destinations.push_back(connection);
        }

        // The endpoint byte's MSB is a bit-resolution flag per the spec; the endpoint id is in the low 7 bits.
        for (const auto &[dest_node_id, endpoint_byte]: lifeline.endpoint_associations) {
            zwave_controller_connection_info_t connection = {};
            zwave_tx_scheme_get_node_connection_info(dest_node_id, endpoint_byte & 0x7F, &connection);
            destinations.push_back(connection);
        }

        if (destinations.empty()) {
            sl_log_debug(LOG_TAG.data(), "Reset step: No lifeline destinations found, skipping notification");
            return SL_STATUS_NOT_AVAILABLE;
        }

        // Seed the counter with the total before any send is issued so a
        // completion callback can never observe the counter at zero while
        // there are still frames in flight.
        pending_reset_notifications.store(destinations.size());

        for (const auto &connection: destinations) {
            if (zwave_tx_send_data(&connection, sizeof(reset_notification), reset_notification, &tx_options, on_reset_notification_send_complete, nullptr, nullptr) != SL_STATUS_OK) {
                // The send failed, so the completion callback will not fire
                // for this destination. Account for it by reusing the same
                // atomic-decrement path the callback takes.
                on_reset_notification_send_complete(0, nullptr, nullptr);
            }
        }

        return SL_STATUS_OK;
    }

    void command_class_device_reset_locally::zwave_command_class_device_reset_on_node_deleted(zwave_node_id_t node_id)
    {
        nodes_to_be_removed.erase(node_id);
    }

    void command_class_device_reset_locally::on_timeout_device_reset(void *data)
    {
        auto it = nodes_to_be_removed.begin();
        if (it != nodes_to_be_removed.end()) {
            if (it->second < DEVICE_RESET_LOCALLY_REMOVE_RETRIES_MAX) {
                if (zwave_network_management_get_state() == NM_IDLE) {
                    zwave_network_management_remove_failed(it->first);
                    it->second++;
                    if (it->second == DEVICE_RESET_LOCALLY_REMOVE_RETRIES_MAX) {
                        timer_set(&command_class_device_reset_locally::timer, DEVICE_RESET_LOCALLY_REMOVE_TRIGGER_TIMEOUT_DEFAULT, command_class_device_reset_locally::on_timeout_device_reset, 0);
                    } else {
                        timer_set(&command_class_device_reset_locally::timer, 5 * DEVICE_RESET_LOCALLY_REMOVE_TRIGGER_TIMEOUT_DEFAULT, command_class_device_reset_locally::on_timeout_device_reset, 0);
                    }
                } else {
                    timer_restart(&command_class_device_reset_locally::timer);
                }
            } else {
                nodes_to_be_removed.erase(it);
                // Iterative call to trigger removing next node (if any) immediately
                on_timeout_device_reset(nullptr);
            }
        }
    }

    sl_status_t command_class_device_reset_locally::on_device_reset_locally_notification_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_device_reset_locally_attribute_map_t payload)
    {
        nodes_to_be_removed.try_emplace(connection_info->remote.node_id, 0);
        timer_set(&command_class_device_reset_locally::timer, DEVICE_RESET_LOCALLY_REMOVE_TRIGGER_TIMEOUT_DEFAULT, command_class_device_reset_locally::on_timeout_device_reset, 0);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class