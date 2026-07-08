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

#ifndef ZWAVE_COMMAND_CLASSES_REGISTRATION_H
#define ZWAVE_COMMAND_CLASSES_REGISTRATION_H

// Generic includes
#include "sl_status.h"
#include "zwave_command_class_base.h"

// Cpp include
#include <functional>

#ifdef __cplusplus
extern "C" {
#endif

using registration_function = std::function<sl_status_t(zwave_command_class::zwave_command_class_base *)>;

sl_status_t zwave_command_classes_register_all(const registration_function &);

#ifdef __cplusplus
}
#endif

#endif  // ZWAVE_COMMAND_CLASSES_REGISTRATION_H
