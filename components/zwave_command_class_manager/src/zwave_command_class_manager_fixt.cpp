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
#include "zwave_command_class_manager_fixt.h"

#include "zwave_command_class_manager.h"       // zwave_command_class_manager::register_command_class
#include "zwave_command_class_registration.h"  // zwave_command_classes_register_all

sl_status_t zwave_command_class_manager_init()
{
    auto status = zwave_command_classes_register_all(&zwave_command_class_manager::register_command_class);

    zwave_command_class_manager::init();

    zwave_command_class_manager::print_info(-1);

    return status;
}