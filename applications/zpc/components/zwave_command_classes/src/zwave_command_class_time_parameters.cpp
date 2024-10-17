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

#include "zwave_command_class_time_parameters.h"
#include "ZW_classcmd.h"
#include "zwave_command_handler.h"
#include "zwave_command_class_indices.h"
#include "zwave_command_classes_utils.h"
#include "assert.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "platform_date_time.h"

#include "zwave_controller_connection_info.h"
#include "zwave_tx.h"

///////////////////////////////////////////////////////////////////////////////
// Private functions, used to handle individual incoming commands.
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Command handler for the Time parameters Get command.
 *
 * The Gateway will send return a Time Report frame containing
 * the current time (year, month, day, hours, minutes and seconds).
 *
 * @param connection_info Info about the connection properties of this frame.
 * @returns sl_status_t indicating the outcome of returning the time.
 */
static sl_status_t zwave_command_class_time_parameters_get(
  const zwave_controller_connection_info_t *connection_info)
{
  if (connection_info && connection_info->local.is_multicast) {
    return SL_STATUS_OK;
  }

  date_time_t time = platform_get_date_time();
  ZW_TIME_PARAMETERS_REPORT_FRAME report
    = {.cmdClass  = COMMAND_CLASS_TIME_PARAMETERS,
       .cmd       = TIME_PARAMETERS_REPORT,
       .year1     = static_cast<uint8_t>((time.year + 1900) >> 8),    //MSB
       .year2     = static_cast<uint8_t>((time.year + 1900) & 0xFF),  //LSB
       .month     = static_cast<uint8_t>(time.mon + 1),
       .day       = static_cast<uint8_t>(time.day),
       .hourUtc   = static_cast<uint8_t>(time.hour & 0xF),
       .minuteUtc = static_cast<uint8_t>(time.min),
       .secondUtc = static_cast<uint8_t>(time.sec)};

  return zwave_command_class_send_report(connection_info,
                                         sizeof(report),
                                         (uint8_t *)&report);
}

sl_status_t
  zwave_command_class_time_parameters_set(const uint8_t *frame_data,
                                          uint16_t frame_length)
{
  if (frame_length < sizeof(ZW_TIME_PARAMETERS_SET_FRAME)) {
    return SL_STATUS_FAIL;
  }

  // Extract and parse the time from the frame data
  uint16_t year  = (frame_data[2] << 8) | frame_data[3];
  uint8_t month  = frame_data[4];
  uint8_t day    = frame_data[5];
  uint8_t hour   = frame_data[6];
  uint8_t minute = frame_data[7];
  uint8_t second = frame_data[8];

  // Create a date_time_t structure and populate it
  date_time_t new_time;
  new_time.year = year - 1900;
  new_time.mon  = month - 1;
  new_time.day  = day;
  new_time.hour = hour;
  new_time.min  = minute;
  new_time.sec  = second;

  // Update the system time
  if (platform_set_date_time(&new_time) != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_time_parameters_support_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  assert(frame_data[COMMAND_CLASS_INDEX] == COMMAND_CLASS_TIME_PARAMETERS);

  switch (frame_data[COMMAND_INDEX]) {
    case TIME_PARAMETERS_GET:
      return zwave_command_class_time_parameters_get(connection_info);
    case TIME_PARAMETERS_SET:
      return zwave_command_class_time_parameters_set(frame_data, frame_length);

    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

sl_status_t zwave_command_class_time_parameters_init()
{
  zwave_command_handler_t handler = {0};
  handler.support_handler
    = &zwave_command_class_time_parameters_support_handler;
  handler.control_handler            = NULL;
  handler.minimal_scheme             = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  handler.manual_security_validation = false;
  handler.command_class              = COMMAND_CLASS_TIME_PARAMETERS;
  handler.version                    = TIME_PARAMETERS_VERSION;
  handler.command_class_name         = "Time Parameters";

  return zwave_command_handler_register_handler(handler);
}

#ifdef __cplusplus
}
#endif
