#ifndef NETWORK_MONITOR_NETWORK_STATUS_H
#define NETWORK_MONITOR_NETWORK_STATUS_H

#include <stdbool.h>
#include "zwave_controller_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NETWORK_MONITOR_NETWORK_STATUS_ONLINE_FUNCTIONAL     = 0,
    NETWORK_MONITOR_NETWORK_STATUS_ONLINE_INTERVIEWING   = 1,
    NETWORK_MONITOR_NETWORK_STATUS_ONLINE_NON_FUNCTIONAL = 2,
    NETWORK_MONITOR_NETWORK_STATUS_UNAVAILABLE           = 3,
    NETWORK_MONITOR_NETWORK_STATUS_OFFLINE               = 4,
    NETWORK_MONITOR_NETWORK_STATUS_COMMISIONING_STARTED  = 5,
} NetworkMonitorNetworkStatus;

/** True if any end device (not ZPC) is in protocol commissioning (`COMMISIONING_STARTED`). */
bool network_monitor_is_end_device_inclusion_ongoing(void);

/** True if any end device (not ZPC) has status `ONLINE_INTERVIEWING`. */
bool network_monitor_is_any_end_device_interviewing(void);

/**
 * @brief True when MQTT Command topics for this NodeID may be acted on.
 *
 * End devices are ready only after inclusion + interview complete
 * (`ONLINE_FUNCTIONAL`). Until then MQTT commands are ignored so clients
 * cannot race the interviewer. The ZPC's own NodeID is always ready.
 */
bool network_monitor_is_node_ready_for_commands(zwave_node_id_t node_id);

#ifdef __cplusplus
}
#endif

#endif
