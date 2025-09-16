/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef SCHEDULE_ENTRY_LOCK_SERVER_H
#define SCHEDULE_ENTRY_LOCK_SERVER_H

// Includes from other Unify Components
#include "dotdot_mqtt.h"
#include "dotdot_mqtt_generated_commands.h"
#include "attribute_store_helper.h"
#include "attribute_resolver.h"
#include "attribute_timeouts.h"
#include "sl_log.h"

// Cpp include
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the schedule entry lock cluster server
 *
 * @returns true on success
 * @returns false on failure
 *
 */
sl_status_t schedule_entry_lock_cluster_server_init(void);

sl_status_t zwave_command_class_publish_generated_week_day_report_command(
  dotdot_unid_t unid,
  dotdot_endpoint_id_t endpoint,
  attribute_store_node_t endpoint_node);

sl_status_t zwave_command_class_publish_generated_year_day_report_command(
  dotdot_unid_t unid,
  dotdot_endpoint_id_t endpoint,
  attribute_store_node_t endpoint_node);

sl_status_t
  zwave_command_class_publish_generated_daily_repeating_report_command(
    dotdot_unid_t unid,
    dotdot_endpoint_id_t endpoint,
    attribute_store_node_t endpoint_node);

sl_status_t zwave_command_class_schedule_entry_lock_write_attributes_callback(
  const dotdot_unid_t unid,
  const dotdot_endpoint_id_t endpoint,
  uic_mqtt_dotdot_callback_call_type_t call_type,
  uic_mqtt_dotdot_unify_schedule_entry_lock_state_t lock_state,
  uic_mqtt_dotdot_unify_schedule_entry_lock_updated_state_t updated_lock_state);

#ifdef __cplusplus
}
#endif

#endif  //SCHEDULE_ENTRY_LOCK_SERVER_H
/** @} end schedule_entry_lock_server */
