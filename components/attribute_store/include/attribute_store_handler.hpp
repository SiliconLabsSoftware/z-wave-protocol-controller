
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

#ifndef ATTRIBUTE_STORE_HANDLER_H
#define ATTRIBUTE_STORE_HANDLER_H

#include "threading.hpp"
#include "init_builder.hpp"
#include "sl_status.h"
#include <string>

namespace zwave_component
{
    class attribute_store_handler : public Initializable
    {
        public:
            attribute_store_handler();
            ~attribute_store_handler();

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;
    };
}  // namespace zwave_component

#endif  // ATTRIBUTE_STORE_HANDLER_H
