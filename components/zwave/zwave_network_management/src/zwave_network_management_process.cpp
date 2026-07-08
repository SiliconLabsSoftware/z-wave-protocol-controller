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

#include "zwave_network_management_handler.hpp"
#include "nm_state_machine.h"
#include "log.h"

#define LOG_TAG "zwave_network_management_process"

namespace zwave_component
{
    ::threading::safe_queue<zwave_network_management_handler::nm_event_data> zwave_network_management_handler::event_queue;

    zwave_network_management_handler::zwave_network_management_handler() : threading("Z-Wave Network Management Handler") {}

    zwave_network_management_handler::~zwave_network_management_handler() {}

    void zwave_network_management_handler::run()
    {
        // Block for a short period when idle so this thread doesn't busy-spin and
        // starve the main thread. Events wake the thread immediately via condition_variable.
        constexpr uint32_t idle_timeout_ms = 1;
        std::optional<nm_event_data> ev    = event_queue.pop(idle_timeout_ms);
        if (ev.has_value()) {
            nm_event_t event = ev.value().event;
            void *data       = ev.value().data;
            if (event == NM_EV_NODE_ADD_SMART_START) {
                data = &ev.value().smartstart;
            }
            if (event < NM_EV_MAX) {
                nm_fsm_post_event(event, data);
            } else {
                sl_log_warning(LOG_TAG, "Dropping event %d (>= NM_EV_MAX)", event);
            }
        }

        if (should_stop()) {
            return;
        }
    }

    sl_status_t zwave_network_management_handler::initialize()
    {
        return SL_STATUS_OK;
    }

    int zwave_network_management_handler::shutdown()
    {
        stop();
        return 0;
    }

    std::string zwave_network_management_handler::name() const
    {
        return "Z-Wave Network Management Handler";
    }
}  // namespace zwave_component

extern "C" {

void zwave_network_management_post_event(nm_event_t ev, void *data)
{
    zwave_component::zwave_network_management_handler::nm_event_data ev_data = {};
    ev_data.event                                                            = ev;
    ev_data.data                                                             = data;
    if (ev == NM_EV_NODE_ADD_SMART_START && data != nullptr) {
        ev_data.smartstart = *static_cast<const smartstart_event_data_t *>(data);
        ev_data.data       = nullptr;
    }
    zwave_component::zwave_network_management_handler::event_queue.push(ev_data);
}

}  // extern "C"
