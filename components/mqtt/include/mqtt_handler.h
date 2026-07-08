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

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include "zwave_node_id_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void mqtt_handler_reset_subscriptions(zwave_home_id_t new_home_id);

#ifdef __cplusplus
}
#endif

#endif  // MQTT_HANDLER_H
