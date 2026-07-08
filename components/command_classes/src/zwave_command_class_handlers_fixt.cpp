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

// Base header
#include "zwave_command_class_registration.h"

// Generated command classes
#include "zwave_generated_command_classes_registration.h"

sl_status_t zwave_command_classes_register_all(const registration_function &register_function)
{
    sl_status_t status = SL_STATUS_OK;

    status |= generated_command_class_registration(register_function);

    return status;
}