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
#include "zwave_tx_callbacks.h"
#include "zwave_tx_process.h"
#include "zwave_tx_route_cache.h"
#include "zwave_tx_queue.hpp"

// Includes from other components
#include "zwave_helper_macros.h"
#include "zwave_controller_internal.h"
#include "zwave_controller_callbacks.h"

// ZPC includes
#include "log.h"

// Generic includes
#include <string.h>
#include <atomic>

#define LOG_TAG "zwave_tx_callbacks"

// Shared variables from the Z-Wave TX Process
extern zwave_tx_queue tx_queue;
extern zwave_tx_session_id_t current_tx_session_id;
extern std::atomic<bool> is_protocol_frame;

// Static variables
zwave_controller_connection_info_t last_received_connection_info;
zwapi_tx_report_t last_received_tx_report;

void zwave_tx_on_frame_received(zwave_node_id_t node_id, const uint8_t *frame_data, uint16_t frame_length)
{
    zwave_tx_process_inspect_received_frame(node_id, frame_data, frame_length);
}

void on_zwave_transport_send_data_complete(uint8_t status, const zwapi_tx_report_t *tx_status, void *user)
{
    if (tx_status != nullptr) {
        last_received_tx_report = *tx_status;
    } else {
        memset(&last_received_tx_report, 0, sizeof(zwapi_tx_report_t));
    }

    void *session_id = user;
    if (is_protocol_frame.load()) {
        protocol_metadata_t *protocol_metadata = (protocol_metadata_t *)user;
        session_id                             = protocol_metadata->tx_session_id;
    }

    // Copy the transmission results to the session_id (user pointer)
    sl_status_t queue_status = tx_queue.set_transmissions_results(session_id, status, &last_received_tx_report);

    if (SL_STATUS_OK != queue_status) {
        sl_log_debug(LOG_TAG, "TX callback unknown: id=%p status=%d tid=%lu", session_id, (int)status, sl_log_thread_id());
        tx_queue.log(false);
        return;
    }

    // Save if we used routing to reach a destination.
    zwave_tx_queue_element_t element = {};
    if (SL_STATUS_OK != tx_queue.get_by_id(&element, session_id)) {
        return;
    }
    if (IS_TRANSMISSION_SUCCESSFUL(status)) {
        sl_log_debug(LOG_TAG, "TX callback: id=%p status=%d elapsed_ms=%lu tid=%lu", session_id, (int)status, (unsigned long)element.transmission_time, sl_log_thread_id());
    } else {
        sl_log_debug(LOG_TAG, "TX callback: id=%p status=%d elapsed_ms=%lu tid=%lu", session_id, (int)status, (unsigned long)element.transmission_time, sl_log_thread_id());
    }
    if ((element.send_data_tx_status.number_of_repeaters > 0) && IS_TRANSMISSION_SUCCESSFUL(element.send_data_status) && (!element.connection_info.remote.is_multicast)) {
        zwave_tx_route_cache_set_number_of_repeaters(element.connection_info.remote.node_id, element.send_data_tx_status.number_of_repeaters);
    }

    // Get TX to look at the queue again, now that we are done.
    zwave_tx_process_post_event(ZWAVE_TX_SEND_OPERATION_COMPLETE, session_id);
}

void zwave_tx_on_new_network_entered(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type)
{
    // This will re-init and unlock the Tx Queue to accept frames again.
    zwave_tx_process_open_tx_queue();
}

void zwave_tx_on_node_deleted(zwave_node_id_t node_id)
{
    zwave_tx_process_on_node_deleted(node_id);
}