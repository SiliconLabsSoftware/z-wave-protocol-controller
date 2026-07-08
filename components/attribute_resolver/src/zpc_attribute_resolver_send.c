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
// Includes from this component
#include "attribute_resolver_internal.h"
#include "zpc_attribute_resolver_group.h"
#include "zpc_attribute_resolver_send.h"
#include "zpc_attribute_resolver_callbacks.h"

// ZPC includes
#include "attribute_resolver.h"
#include "attribute_resolver_rule.h"

// Includes from other ZPC components
#include "zpc_attribute_store_network_helper.h"
#include "attribute_store_defined_attribute_types.h"
#include "zwave_controller_utils.h"
#include "zwave_tx_scheme_selector.h"
#include "zwave_command_class_supervision.h"
#include "zwave_tx.h"
#include "log.h"
#include <pthread.h>

#define LOG_TAG "attribute_resolver_send_zwave"

///////////////////////////////////////////////////////////////////////////////
// No-supervision type registry
///////////////////////////////////////////////////////////////////////////////
#define MAX_NO_SUPERVISION_TYPES 16
static attribute_store_type_t no_supervision_registry[MAX_NO_SUPERVISION_TYPES];
static size_t no_supervision_registry_count          = 0;
static pthread_mutex_t no_supervision_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

void attribute_resolver_register_no_supervision_type(attribute_store_type_t node_type)
{
    bool already_registered = false;
    pthread_mutex_lock(&no_supervision_registry_mutex);
    for (size_t i = 0; i < no_supervision_registry_count; i++) {
        if (no_supervision_registry[i] == node_type) {
            already_registered = true;
            break;
        }
    }
    if (!already_registered) {
        if (no_supervision_registry_count < MAX_NO_SUPERVISION_TYPES) {
            no_supervision_registry[no_supervision_registry_count++] = node_type;
        } else {
            sl_log_warning(LOG_TAG, "No-supervision registry full; cannot exempt attribute type %u", node_type);
        }
    }
    pthread_mutex_unlock(&no_supervision_registry_mutex);
}

bool attribute_resolver_is_no_supervision_type(attribute_store_type_t node_type)
{
    bool found = false;
    pthread_mutex_lock(&no_supervision_registry_mutex);
    for (size_t i = 0; i < no_supervision_registry_count; i++) {
        if (no_supervision_registry[i] == node_type) {
            found = true;
            break;
        }
    }
    pthread_mutex_unlock(&no_supervision_registry_mutex);
    return found;
}

sl_status_t attribute_resolver_send(attribute_store_node_t node, const uint8_t *frame_data, uint16_t frame_data_len, bool is_set)
{
    // Are we already resolving it?
    // (e.g. multicat started before the resolver got to ask to resolve it)
    if (true == is_node_in_resolution_list(node)) {
        sl_log_debug(LOG_TAG, "Send skip in resolution list: node=%d is_set=%d tid=%lu", node, is_set ? 1 : 0, (unsigned long)pthread_self());
        return SL_STATUS_OK;
    }

    // Else, does the Group send module want to take care of that for us ?
    if (zpc_attribute_resolver_send_group(node) == SL_STATUS_OK) {
        sl_log_debug(LOG_TAG, "Send handled via group: node=%d is_set=%d tid=%lu", node, is_set ? 1 : 0, (unsigned long)pthread_self());
        return SL_STATUS_OK;
    }

    // If not, we take care of it ourselves.
    zwave_node_id_t node_id         = 0;
    zwave_endpoint_id_t endpoint_id = 0;

    if (SL_STATUS_OK != attribute_store_network_helper_get_zwave_ids_from_node(node, &node_id, &endpoint_id)) {
        sl_log_warning(LOG_TAG,
                       "Cannot find out NodeID / Endpoint ID for Attribute Store "
                       "node %p. Ignoring",
                       node);
        return SL_STATUS_FAIL;
    }

    add_node_in_resolution_list(node, is_set ? RESOLVER_SET_RULE : RESOLVER_GET_RULE);
    // Prepare the Connection Info data:
    zwave_controller_connection_info_t connection_info;
    zwave_tx_scheme_get_node_connection_info(node_id, endpoint_id, &connection_info);

    // Prepare the Z-Wave TX options.
    zwave_tx_options_t tx_options = {0};
    zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_RECOMMENDED_NODE_INTERVIEW_PRIORITY, is_set ? 0 : 1, 0, &tx_options);

    intptr_t user                       = node;
    sl_status_t send_status             = SL_STATUS_OK;
    zwave_tx_session_id_t tx_session_id = NULL;
    if (is_set == true && !attribute_resolver_is_no_supervision_type(attribute_store_get_node_type(node)) && zwave_command_class_supervision_want_supervision_frame(node_id, endpoint_id)) {
        // Send with Supervision
        send_status = zwave_command_class_supervision_send_data(&connection_info, frame_data_len, frame_data, &tx_options, on_resolver_zwave_supervision_complete, (void *)user, &tx_session_id);
        if (send_status != SL_STATUS_OK) {
            // If we could not queue, just mark the attribute as failed supervision,
            // it will trigger the resolver to try to "get resolve" it again.
            on_resolver_zwave_supervision_complete(SUPERVISION_REPORT_FAIL, NULL, (void *)user);
        }
    } else {
        // Just send without Supervision encapsulation
        send_status = zwave_tx_send_data(&connection_info, frame_data_len, frame_data, &tx_options, on_resolver_zwave_send_data_complete, (void *)user, &tx_session_id);

        if (send_status != SL_STATUS_OK) {
            // If we could not queue, just mark the attribute as failed transmission.
            on_resolver_zwave_send_data_complete(TRANSMIT_COMPLETE_FAIL, NULL, (void *)user);
        }
    }

    if (send_status == SL_STATUS_OK) {
        attribute_resolver_associate_node_with_tx_sessions_id(node, tx_session_id);
        sl_log_debug(LOG_TAG, "Send queued: node=%d node_id=%d endpoint=%d is_set=%d tid=%lu", node, node_id, endpoint_id, is_set ? 1 : 0, (unsigned long)pthread_self());
    }
    return send_status;
}

///////////////////////////////////////////////////////////////////////////////
// Function shared among the component. Used for send_data callbacks
///////////////////////////////////////////////////////////////////////////////
void attribute_resolver_send_init()
{
    // Reset our list of pending nodes/resolutions
    attribute_resolver_callbacks_reset();
}
