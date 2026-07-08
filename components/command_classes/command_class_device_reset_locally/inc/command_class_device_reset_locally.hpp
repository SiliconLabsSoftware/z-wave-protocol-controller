
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

#ifndef COMMAND_CLASS_DEVICE_RESET_LOCALLY_H
#define COMMAND_CLASS_DEVICE_RESET_LOCALLY_H

#include "command_class_device_reset_locally_mqtt.hpp"
#include "command_class_device_reset_locally_attribute_store.hpp"
#include "timer.hpp"
#include "zwave_tx.h"
#include <stdint.h>
#include <atomic>
#include <cstddef>
#include <map>

namespace zwave_command_class
{

    class command_class_device_reset_locally final : public command_class_device_reset_locally_attribute_store, public command_class_device_reset_locally_mqtt
    {

        public:
            command_class_device_reset_locally();
            ~command_class_device_reset_locally() = default;

        private:
            static struct timer_handle_t timer;
            static std::map<zwave_node_id_t, uint8_t> nodes_to_be_removed;

            // Z-Wave nodes issue Device Reset Locally command before they reset their
            // HomeID and NodeID. Due to that, ZPC shall wait at-least 1 second
            // before triggering removal of the node.
            static constexpr uint16_t DEVICE_RESET_LOCALLY_REMOVE_TRIGGER_TIMEOUT_DEFAULT = 1000;
            static constexpr uint16_t DEVICE_RESET_LOCALLY_REMOVE_RETRIES_MAX             = 3;
            static constexpr uint32_t MAXIMUM_TIME_FOR_RESET_NOTIFICATION                 = 30000;

            sl_status_t on_device_reset_locally_notification_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_device_reset_locally_attribute_map_t payload) override;

            static void on_timeout_device_reset(void *data);
            static void zwave_command_class_device_reset_on_node_deleted(zwave_node_id_t node_id);
            static sl_status_t zwave_command_class_device_reset_on_zpc_reset(void);
            static void on_reset_notification_send_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user);
            // Counted atomically because the TX completion callback may run on a
            // different context than the reset step that queues the notifications.
            static std::atomic<size_t> pending_reset_notifications;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_DEVICE_RESET_LOCALLY_H
