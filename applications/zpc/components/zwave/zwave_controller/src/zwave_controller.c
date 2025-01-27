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

//Generic includes
#include <stdlib.h>
#include <stdio.h>

// Includes from other components
#include "sl_log.h"
#include "zwapi_protocol_basis.h"
#include "ZW_classcmd.h"
#include "zwave_s2_transport.h"

#ifndef ZWAVE_BUILD_SYSTEM
#include "zpc_config.h"
#endif

// Includes from this component
#include "zwave_controller_utils.h"
#include "zwave_controller_types.h"
#include "zwave_controller_callbacks.h"
#include "zwave_controller_transport.h"

// Setup Log ID
#define LOG_TAG "zwapi_controller"

void zwave_controller_set_application_nif(const uint8_t *command_classes,
                                          uint8_t command_classes_length)
{
#ifdef ZWAVE_BUILD_SYSTEM
  node_type_t my_node_type = {.basic    = BASIC_TYPE_STATIC_CONTROLLER,
                              .generic  = GENERIC_TYPE_GENERIC_CONTROLLER,
                              .specific = SPECIFIC_TYPE_NOT_USED};
#else
  node_type_t my_node_type
    = {.basic    = zpc_get_config()->zpc_basic_device_type,
       .generic  = zpc_get_config()->zpc_generic_device_type,
       .specific = zpc_get_config()->zpc_specific_device_type};
#endif
  // Tell the Z-Wave module what our NIF should be:
  zwapi_set_application_node_information(APPLICATION_NODEINFO_LISTENING,
                                         my_node_type,
                                         command_classes,
                                         command_classes_length);
}

void zwave_controller_set_secure_application_nif(const uint8_t *command_classes,
                                                 uint8_t command_classes_length)
{
  // Simply forward our secure NIF to the S2 library.
  zwave_s2_set_secure_nif(command_classes, command_classes_length);
}