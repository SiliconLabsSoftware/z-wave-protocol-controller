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
#include "zwave_network_management.h"
#include "zwave_network_management_helpers.h"
#include "zwave_network_management_callbacks.h"
#include "nm_state_machine.h"

// Interfaces
#include "zwave_node_id_definitions.h"
#include "zwave_generic_types.h"

// Z-Wave API
#include "zwapi_protocol_basis.h"
#include "zwapi_protocol_mem.h"
#include "zwapi_protocol_controller.h"

// Z-Wave Controller
#include "zwave_controller.h"

// Generic includes
#include <string.h>
#include <pthread.h>

// ZPC
#include "log.h"
#define LOG_TAG "zwave_network_management_helpers"

static pthread_mutex_t cached_node_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void network_management_copy_cached_node_list(zwave_nodemask_t node_list)
{
    pthread_mutex_lock(&cached_node_list_mutex);
    memcpy(node_list, nms.cached_node_list, sizeof(zwave_nodemask_t));
    pthread_mutex_unlock(&cached_node_list_mutex);
}

bool network_management_cached_node_list_contains(zwave_node_id_t node_id)
{
    zwave_nodemask_t node_list = {};
    network_management_copy_cached_node_list(node_list);
    return ZW_IS_NODE_IN_MASK(node_id, node_list) != 0;
}

void reset_nms_last_operation_data()
{
    nms.node_id_being_handled    = 0;
    nms.exclusion_node_id_filter = 0;
    nms.is_controller            = 0;
    nms.flags                    = 0;
    nms.inclusion_protocol       = PROTOCOL_ZWAVE;
    nms.requested_csa            = false;
    nms.requested_keys           = 0;
    nms.expected_s2_user_input   = NM_S2_USER_INPUT_NONE;
    nms.reported_dsk_blanked     = 0;
    nms.kex_fail_type            = 0;
    nms.s2_bootstrapping_started = false;
    nms.learn_mode_intent        = ZWAVE_NETWORK_MANAGEMENT_LEARN_NONE;
    memset(nms.reported_dsk, 0, sizeof(zwave_dsk_t));
    memset(nms.expected_dsk, 0, sizeof(zwave_dsk_t));
    memset(nms.verified_dsk_input, 0, sizeof(zwave_dsk_t));
}

bool we_are_alone_in_our_network()
{
    zwave_nodemask_t node_list = {};
    network_management_copy_cached_node_list(node_list);
    for (zwave_node_id_t node_id = ZW_MIN_NODE_ID; node_id <= ZW_LR_MAX_NODE_ID; node_id++) {
        if (node_id == nms.cached_local_node_id) {
            // We just found ourselves in the node mask.
            continue;
        }
        if (ZW_IS_NODE_IN_MASK(node_id, node_list) == 1) {
            return false;
        }
    }
    return true;
}

bool network_management_is_ready_for_a_new_operation()
{
    // NOTE: nm_state_load() is only written on the NMS FSM thread. This read is
    // advisory — a caller may observe NM_IDLE while an event that changes
    // state is already queued. Callers must tolerate SL_STATUS_BUSY from
    // the NMS API functions even after a true return here.
    if (nm_state_load() != NM_IDLE) {
        return false;
    }
    if (zwave_controller_is_reset_ongoing()) {
        return false;
    }

    return true;
}

void network_management_refresh_cached_ids()
{
    zwapi_memory_get_ids(&nms.cached_home_id, &nms.cached_local_node_id);
}

void network_management_refresh_cached_node_list()
{
    zwave_nodemask_t node_list = {};
    network_management_copy_cached_node_list(node_list);
    if (zwapi_get_full_node_list(node_list) == SL_STATUS_OK) {
        pthread_mutex_lock(&cached_node_list_mutex);
        memcpy(nms.cached_node_list, node_list, sizeof(node_list));
        pthread_mutex_unlock(&cached_node_list_mutex);
    }
}

void network_management_refresh_controller_capabilities_bitmask()
{
    nms.controller_capabilities_bitmask = zwapi_get_controller_capabilities();
}

void network_management_refresh_network_information()
{
    network_management_refresh_cached_ids();
    network_management_refresh_cached_node_list();
    network_management_refresh_controller_capabilities_bitmask();
}

void network_management_stop_ongoing_operations()
{
    zwapi_add_node_to_network(ADD_NODE_STOP, NULL);
    zwapi_remove_node_from_network(REMOVE_NODE_STOP, NULL);
    zwapi_set_learn_mode(LEARN_MODE_DISABLE, NULL);
}

bool is_command_class_in_nif(const zwave_command_class_t *nif, uint8_t nif_length, zwave_command_class_t command_class)
{
    if (nif_length > ZWAVE_CONTROLLER_MAXIMUM_COMMAND_CLASS_LIST_LENGTH) {
        sl_log_warning(LOG_TAG,
                       "NIF Length is too large (%d). "
                       "Clipping to the maximum size %d.",
                       nif_length,
                       ZWAVE_CONTROLLER_MAXIMUM_COMMAND_CLASS_LIST_LENGTH);
        nif_length = ZWAVE_CONTROLLER_MAXIMUM_COMMAND_CLASS_LIST_LENGTH;
    }

    for (int i = 0; i < nif_length; i++) {
        if (nif[i] == command_class) {
            return true;
        }
    }

    return false;
}

bool we_should_self_destruct()
{
    // Self destruct only if we did a SmartStart inclusion.
    if (nms.learn_mode_intent != ZWAVE_NETWORK_MANAGEMENT_LEARN_SMART_START) {
        return false;
    }

    // KEX fail type set means a failure.
    // Bootstrapping not started at all is also a failure in the SmartStart world
    // NWK:0202.1 - LR-NWK:0064.1
    if ((nms.kex_fail_type != 0) || (nms.s2_bootstrapping_started == false)) {
        return true;
    }

    // Here a tricky bit, we assume that if we have a granted NodeID > 255, we are
    // included with Z-Wave LR. Ideally, the Z-Wave API or S2 engine should tell us this.
    if (IS_ZWAVE_NODE_ID(nms.cached_local_node_id)) {
        // With Z-Wave, we accept any S2 keyset for after SmartStart
        return false;
    }
    if (IS_ZWAVE_EXTENDED_NODE_ID(nms.cached_local_node_id)) {
        // We need either Access Control or Authenticated to accept our network.
        return !(nms.granted_keys & (ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY | ZWAVE_CONTROLLER_S2_ACCESS_KEY));
    }

    // We do not know if we are running on Z-Wave or Z-Wave LR ... just leave the
    // network.
    sl_log_error(LOG_TAG,
                 "Unkown protocol after SmartStart inclusion."
                 "NodeID %d. Assuming we have to reset.",
                 nms.cached_local_node_id);
    return true;
}

void network_management_take_sis_role_if_no_suc_in_network()
{
    if ((nms.controller_capabilities_bitmask & (CONTROLLER_NODEID_SERVER_PRESENT | CONTROLLER_IS_SECONDARY)) == 0) {
        sl_log_info(LOG_TAG, "Attempting to take the SUC/SIS Role");
        zwapi_set_suc_node_id(nms.cached_local_node_id, true, false, ZW_SUC_FUNC_NODEID_SERVER, &on_set_suc_node_id_completed);
        // Z-Wave API does not seem to callback
        // if we have our own NodeID being set.
        network_management_refresh_controller_capabilities_bitmask();
    }
}
