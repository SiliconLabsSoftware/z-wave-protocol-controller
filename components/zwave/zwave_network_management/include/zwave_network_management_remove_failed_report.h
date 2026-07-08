/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef ZWAVE_NETWORK_MANAGEMENT_REMOVE_FAILED_REPORT_H
#define ZWAVE_NETWORK_MANAGEMENT_REMOVE_FAILED_REPORT_H

#include "zwave_node_id_definitions.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REMOVE_FAILED_STATUS_OPERATION_SUCCESSFUL "operation_successful"
#define REMOVE_FAILED_STATUS_OPERATION_FAILED     "operation_failed"
#define REMOVE_FAILED_STATUS_OPERATION_ABORTED    "operation_aborted"
#define REMOVE_FAILED_STATUS_NOT_REMOVED          "not_removed"
#define REMOVE_FAILED_STATUS_NODE_ONLINE          "node_online"
#define REMOVE_FAILED_STATUS_TIMEOUT              "timeout"

/**
 * @brief Publish the result of a Remove Failed Node operation to MQTT.
 *
 * Called from the NM state machine when the NM_WAITING_FOR_FAILED_NODE_REMOVAL
 * state resolves (either NM_EV_REMOVE_FAILED_OK or NM_EV_REMOVE_FAILED_FAIL).
 *
 * @param node_id  The NodeID that was being removed.
 * @param status   The status of the operation.
 */
void zwave_network_management_publish_remove_failed_report(zwave_node_id_t node_id, const char *status);

#ifdef __cplusplus
}
#endif

#endif  // ZWAVE_NETWORK_MANAGEMENT_REMOVE_FAILED_REPORT_H
