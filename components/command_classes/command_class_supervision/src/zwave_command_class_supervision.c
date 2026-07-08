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
#include "zwave_command_class_supervision.h"
#include "zwave_command_class_supervision_internals.h"
#include "zwave_command_class_supervision_process.h"
#include "zwave_command_class_indices.h"
// Attribute store helpers
#include "attribute_store_defined_attribute_types.h"
#include "attribute_store_helper.h"

// Includes from other components
#include "log.h"
#include "attribute_store.h"
#include "attribute_resolver.h"
#include "utils.h"
#include "ZW_classcmd.h"
#include "zwave_tx.h"
#include "zwave_controller_keyset.h"
#include "zwave_controller_utils.h"
#include "zwave_utils.h"

// Generic includes
#include "assert.h"
#include "string.h"

// Log tag
#define LOG_TAG "zwave_command_class_supervision"

// Keep a list of nodes that must be "awaken" on demand
static zwave_nodemask_t wake_on_demand_list = {0};

///////////////////////////////////////////////////////////////////////////////
// Callback functions
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief Callback function registered to @ref zwave_tx used
 * to track when a Supervision frame has been sent successfully.
 *
 * @param status Refer to @ref on_zwave_tx_send_data_complete_t
 * @param tx_info Refer to @ref on_zwave_tx_send_data_complete_t
 * @param user Refer to @ref on_zwave_tx_send_data_complete_t
 */
void zwave_command_class_supervision_on_send_data_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user)
{
    // At that point, we know TX has sent the frame
    // However, we rely on the Supervision Report to make the final callback.

    // We need the process to start a timer, waiting for a report
    // If the report was already received and processed, the timer will not do
    // anything bad as the session will be closed when expiring.
    zwave_command_class_supervision_restart_timer();

    supervision_id_t supervision_id = (intptr_t)user;

    supervised_session_t *ongoing_session = zwave_command_class_supervision_find_session_by_unique_id(supervision_id);

    if (ongoing_session == NULL) {
        // RX has already received the report and the session is closed.
        // Or somebody asked for a callback for Multicast, which won't happen just yet
        return;
    }

    // Verify that transmission was successful, else just callback a fail and close
    // the supervision session
    if ((status != TRANSMIT_COMPLETE_OK) && (status != TRANSMIT_COMPLETE_VERIFIED)) {
        if (ongoing_session->callback != NULL) {
            ongoing_session->callback(SUPERVISION_REPORT_FAIL, tx_info, ongoing_session->user);
        }
        zwave_command_class_supervision_close_session(supervision_id);
        return;
    }

    // Start waiting for a report
    // Verify here that we are not in the WORKING stage because sometimes the
    // report comes in before the send_data callback.
    if (ongoing_session->status != SUPERVISION_REPORT_WORKING) {
        ongoing_session->expiry_time = clock_time() + SUPERVISION_REPORT_TIMEOUT;
    }
    // Save the tx info for the user callback later on.
    ongoing_session->tx_info_valid = true;
    memcpy(&ongoing_session->tx_info, tx_info, sizeof(zwapi_tx_report_t));
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t
  zwave_command_class_supervision_send_data(const zwave_controller_connection_info_t *connection, uint16_t data_length, const uint8_t *data_payload, const zwave_tx_options_t *tx_options, const on_zwave_tx_send_data_complete_t on_supervision_complete, void *user, zwave_tx_session_id_t *session)
{
    // Check if we can handle the frame size, else bail out.
    if (data_length > SUPERVISION_ENCAPSULATED_COMMAND_MAXIMUM_SIZE) {
        sl_log_warning(LOG_TAG,
                       "Frame too large (length = %d) to be Supervision "
                       "encapsulated. Discarding.",
                       data_length);
        return SL_STATUS_WOULD_OVERFLOW;
    }

    // Pick up the connection info and update the currently controlled session from it.
    supervision_id_t supervision_id       = zwave_command_class_supervision_create_session(connection, tx_options, on_supervision_complete, user);
    supervised_session_t *ongoing_session = zwave_command_class_supervision_find_session_by_unique_id(supervision_id);

    if (ongoing_session == NULL) {
        zwave_command_class_supervision_process_log();
        sl_log_warning(LOG_TAG,
                       "Cannot allocate Supervision ID for sending session. "
                       "Dropping send data call.");
        return SL_STATUS_FAIL;
    }

    // Make a new payload with the encapsulated frame.
    zwave_supervision_get_frame_t frame = {0};
    frame.command_class                 = COMMAND_CLASS_SUPERVISION;
    frame.command                       = SUPERVISION_GET;

    // We always ask for status updates.
    frame.status_wake_session_field   = SUPERVISION_GET_PROPERTIES1_STATUS_UPDATES_BIT_MASK | ongoing_session->session.session_id;
    frame.encapsulated_command_length = data_length;
    memcpy(frame.encapsulated_command, data_payload, data_length);
    uint16_t supervision_frame_size = sizeof(frame) - SUPERVISION_ENCAPSULATED_COMMAND_MAXIMUM_SIZE + data_length;

    // Supervision Get expect 1 frame (or more) frames in response.
    // Update the tx option accordingly.
    zwave_tx_options_t supervision_tx_options  = *tx_options;
    supervision_tx_options.number_of_responses = tx_options->number_of_responses;
    supervision_tx_options.number_of_responses += 1;

    intptr_t user_parameter;
    if (!tx_options->transport.is_protocol_frame) {
        user_parameter = (intptr_t)INVALID_SUPERVISION_ID;
        if (connection->remote.is_multicast == false) {
            user_parameter = (intptr_t)supervision_id;
        }
    } else {
        user_parameter = (intptr_t)user;
    }

    sl_status_t zwave_tx_status = zwave_tx_send_data(connection, supervision_frame_size, (uint8_t *)&frame, &supervision_tx_options, connection->remote.is_multicast ? NULL : &zwave_command_class_supervision_on_send_data_complete, (void *)user_parameter, session);

    if (zwave_tx_status != SL_STATUS_OK) {
        // Abort the supervision session, no callback to the user
        zwave_command_class_supervision_close_session(supervision_id);
    } else if (session != NULL) {
        zwave_command_class_supervision_assign_session_tx_id(supervision_id, *session);
    }
    // Ensure a timer is running for the newly created sessions.
    //  (if we created mulitcast sessions, the session == NULL but we still
    //  want to start that timer)
    zwave_command_class_supervision_restart_timer();

    return zwave_tx_status;
}

sl_status_t zwave_command_class_supervision_abort_send_data(zwave_tx_session_id_t session)
{
    // Just try to cancel the transmission at the TX level and
    // close the supervision session. Supervision status updates received subsequently
    // will be ignored
    sl_log_debug(LOG_TAG,
                 "Attempting to abort Supervision "
                 "sesssion associated with Tx Session %p",
                 session);
    zwave_command_class_supervision_close_session_by_tx_session(session);
    return zwave_tx_abort_transmission(session);
}

sl_status_t zwave_command_class_supervision_wake_on_demand(zwave_node_id_t node_id)
{
    // Buffer overflow protection
    if ((node_id - 1) < 0 || ((node_id - 1) / 8) >= sizeof(zwave_nodemask_t)) {
        sl_log_debug(LOG_TAG,
                     "Wake Up On Demand requested on a node (%d) out of supported "
                     "range. Ignoring",
                     node_id);
        return SL_STATUS_WOULD_OVERFLOW;
    }

    // Add the NodeID to our Wake On Demand list.
    wake_on_demand_list[(node_id - 1) / 8] |= (1 << ((node_id - 1) % 8));
    return SL_STATUS_OK;
}

sl_status_t zwave_command_class_supervision_stop_wake_on_demand(zwave_node_id_t node_id)
{
    // Buffer overflow protection
    if ((node_id - 1) < 0 || ((node_id - 1) / 8) >= sizeof(zwave_nodemask_t)) {
        return SL_STATUS_WOULD_OVERFLOW;
    }

    // Just remove the node from the Wake On Demand list:
    wake_on_demand_list[(node_id - 1) / 8] &= (0xFF - (1 << ((node_id - 1) % 8)));
    return SL_STATUS_OK;
}
