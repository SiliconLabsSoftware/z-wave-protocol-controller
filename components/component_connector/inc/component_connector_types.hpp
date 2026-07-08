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

#ifndef COMPONENT_CONNECTOR_TYPES_H
#define COMPONENT_CONNECTOR_TYPES_H

#include <stdint.h>
#include <array>
#include <cstring>
#include <string>
#include "zwave_controller_types.h"
#include "zwave_controller_connection_info.h"
#include "zwave_keyset_definitions.h"
#include "zwave_network_management_types.h"
#include "sl_status.h"
#include "attribute_store.h"

namespace zwave_command_class
{
    /**
     * @brief Payload for node information received event
     *
     * @note The node_info is copied, so it's safe to use after the handler returns.
     */
    struct component_connector_node_information_received_payload_t {
            zwave_node_id_t node_id;
            zwave_node_info_t node_info;  // Copy of node info data (not a pointer)
    };

    /**
     * @brief Payload for node added event
     *
     * @note The node_info is copied, so it's safe to use after the handler returns.
     *       The dsk array is copied, so it's safe to use after the handler returns.
     */
    struct component_connector_node_added_payload_t {
            sl_status_t status;
            zwave_node_info_t node_info;  // Copy of node info data (not a pointer)
            zwave_node_id_t node_id;
            zwave_dsk_t dsk;  // C array - kept as-is for compatibility with zwave_dsk_t typedef
            zwave_keyset_t granted_keys;
            zwave_kex_fail_type_t kex_fail_type;
            zwave_protocol_t inclusion_protocol;

            // Helper method to set DSK from zwave_dsk_t (safer than memcpy)
            void set_dsk(const zwave_dsk_t src)
            {
                std::memcpy(dsk, src, sizeof(zwave_dsk_t));
            }
    };

    /**
     * @brief Payload for network address update event.
     *
     * Fired when ZPC's HomeID and/or NodeID is updated.
     */
    struct component_connector_network_address_updated_payload_t {
            zwave_home_id_t home_id;
            zwave_node_id_t node_id;
    };

    /**
     * @brief Payload for new network entered event.
     *
     * Fired when controller has completed entering a new network.
     */
    struct component_connector_new_network_entered_payload_t {
            zwave_home_id_t home_id;
            zwave_node_id_t node_id;
            zwave_keyset_t granted_keys;
            zwave_kex_fail_type_t kex_fail_type;
    };

    /**
     * @brief Payload for factory reset complete event.
     *
     * Fired exactly once after a controller factory reset has completed and
     * ZPC has entered the new network. The home_id and node_id are the new
     * ones assigned during the reset.
     */
    struct component_connector_factory_reset_complete_payload_t {
            zwave_home_id_t home_id;
            zwave_node_id_t node_id;
    };

    /**
     * @brief Payload for COMPONENT_CONNECTOR_NODE_ID_ASSIGNED_BY_OTHER_CONTROLLER.
     *
     * Fired when the Z-Wave protocol assigns a NodeID while our NM is idle, i.e. another
     * controller included the node. system_events creates the attribute-store placeholder
     * before firing the event. The Inclusion Controller CC arbitrates whether a handoff is
     * coming (INITIATE) or whether to fall back to a plain interview-request after a grace.
     */
    struct component_connector_node_id_assigned_by_other_controller_payload_t {
            zwave_node_id_t node_id;
            zwave_protocol_t inclusion_protocol;
    };

    /**
     * @brief Payload for COMPONENT_CONNECTOR_NODE_INTERVIEW_REQUESTED.
     *
     * Imperative "please (re-)interview this node". Fired by the Inclusion Controller CC
     * after its grace window elapses without an INITIATE, by OTA after firmware
     * activation, and by network_monitor after interview failure when the node is
     * reachable again (AL/FL TX/RX or NL Wake Up Notification). Consumed by device_interviewer.
     */
    struct component_connector_node_interview_requested_payload_t {
            zwave_node_id_t node_id;
    };

    /**
     * @brief Payload for node deleted event
     *
     * @note The dsk array is copied, so it's safe to use after the handler returns.
     */
    struct component_connector_node_deleted_payload_t {
            zwave_node_id_t node_id;
            zwave_dsk_t dsk;
    };

    /**
     * @brief Payload for node exclusion requested event
     *
     * Fired when we know which node is being excluded (during REMOVE_NODE_STATUS_REMOVING_END_NODE
     * or REMOVE_NODE_STATUS_REMOVING_CONTROLLER), before the node is actually deleted.
     * This allows components to prepare for exclusion (e.g., cancel the interview for that specific node).
     */
    struct component_connector_node_exclusion_requested_payload_t {
            zwave_node_id_t node_id;  // The node ID that is being excluded
    };

    /**
     * @brief Payload for node remove-failed event
     *
     * Fired from the NM state machine when a remove-failed operation
     * finishes (success or failure).
     */
    struct component_connector_node_remove_failed_payload_t {
            zwave_node_id_t node_id;
            std::string reason;
    };

    /**
     * @brief Payload for interview done event
     *
     * Fired when device interview completes for an endpoint.
     * Command classes should use this event to initialize their attributes
     * based on the discovered command class versions.
     */
    struct component_connector_interview_done_payload_t {
            attribute_store_node_t endpoint_node;  // The endpoint node that completed interview
            sl_status_t status;                    // Interview result (e.g. SL_STATUS_OK for success)
    };
}  // namespace zwave_command_class

#endif  // COMPONENT_CONNECTOR_TYPES_H
