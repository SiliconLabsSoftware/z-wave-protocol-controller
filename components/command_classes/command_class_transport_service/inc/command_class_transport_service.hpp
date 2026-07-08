
/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMMAND_CLASS_TRANSPORT_SERVICE_H
#define COMMAND_CLASS_TRANSPORT_SERVICE_H

#include "command_class_transport_service_attribute_store.hpp"
#include "attribute_store_defined_attribute_types.h"
#include <any>
#include "sl_status.h"
#include "command_class_transport_service_types.hpp"

namespace zwave_command_class
{

    class command_class_transport_service : public command_class_transport_service_attribute_store
    {
        public:
            command_class_transport_service();
            ~command_class_transport_service() = default;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_BATTERY_H
