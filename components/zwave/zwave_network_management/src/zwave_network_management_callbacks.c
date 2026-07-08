/******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "zwave_network_management_callbacks.h"
#include "zwave_network_management_process.h"
#include "zwave_network_management_helpers.h"
#include "nm_state_machine.h"

// Z-Wave Controller callbacks.
#include "zwave_controller_internal.h"

#include "log.h"
#define LOG_TAG "zwave_network_management_callbacks"

// SWPROT-7300 Protocol returns value 5
#define LEARN_MODE_UNDOCUMENTED_VALUE 5

void on_learn_mode_callback(LEARN_INFO *learn_mode_info)
{
    switch (learn_mode_info->bStatus) {
        case LEARN_MODE_STARTED:
            sl_log_info(LOG_TAG,
                        "New Learn Mode operation "
                        "(inclusion/exclusion/replication) started\n");

            nms.node_id_being_handled = learn_mode_info->bSource;
            zwave_network_management_post_event(NM_EV_LEARN_STARTED, 0);

            // If we were in SmartStart learn mode (which keeps the nms state to IDLE)
            // change the nms state from IDLE to LEARN_MODE
            if (nms.learn_mode_intent == ZWAVE_NETWORK_MANAGEMENT_LEARN_SMART_START) {
                nms.state = NM_LEARN_MODE;
            }

            break;
        case LEARN_MODE_UNDOCUMENTED_VALUE:
            sl_log_warning(LOG_TAG,
                           "Accepting Learn Mode callback status 5 as if it "
                           "was LEARN_MODE_DONE.");
        case LEARN_MODE_DONE:
            sl_log_info(LOG_TAG, "Learn mode completed successfully\n");
            zwave_network_management_post_event(NM_EV_LEARN_DONE, 0);
            break;
        case LEARN_MODE_FAILED:
            sl_log_warning(LOG_TAG, "Learn mode failed. No network changes.\n");
            zwave_network_management_post_event(NM_EV_LEARN_FAILED, 0);
            break;
        default:
            sl_log_warning(LOG_TAG,
                           "Learn Mode Callback is called "
                           "without accepted status value\n");
            break;
    }
}

void on_set_suc_node_id_completed(uint8_t set_suc_status)
{
    (void)set_suc_status;
    sl_log_info(LOG_TAG, "Set SUC Status completed. Status= %d", set_suc_status);
    network_management_refresh_controller_capabilities_bitmask();
}

void on_set_default_complete()
{
    zwave_network_management_post_event(NM_EV_SET_DEFAULT_COMPLETE, 0);
}

void on_request_node_neighbor_update_completed(uint8_t status)
{
    zwave_controller_on_request_neighbor_update(status);
    if (0x21 != status) {
        zwave_network_management_post_event(NM_EV_REQUEST_NODE_NEIGHBOR_REQUEST_COMPLETE, 0);
    }
}

void on_zwave_api_started()
{
    // If we were waiting for zwapi_set_default to complete and the chip soft-reset,
    // treat the API-started notification as the completion signal so the reset chain
    // can proceed without blocking the FSM thread.
    if (zwave_network_management_get_state() == NM_SET_DEFAULT) {
        zwave_network_management_post_event(NM_EV_SET_DEFAULT_COMPLETE, 0);
        return;
    }

    // Ensure that no add/remove/learn mode was active
    if (zwave_network_management_get_state() != NM_IDLE) {
        sl_log_warning(LOG_TAG,
                       "Network management was busy during a Z-Wave API restart. "
                       "Aborting.");
        zwave_network_management_abort();
    }
}