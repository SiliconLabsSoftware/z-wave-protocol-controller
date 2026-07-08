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

#ifndef SYSTEM_EVENTS_HPP
#define SYSTEM_EVENTS_HPP

#include "init_builder.hpp"
#include "sl_status.h"
#include "zwave_controller_types.h"
#include "zwave_controller_connection_info.h"
#include "zwave_keyset_definitions.h"
#include "zwave_network_management_types.h"
#include <atomic>
#include <string>

/**
 * @brief System Events component
 *
 * This component registers Z-Wave Controller callbacks and converts them
 * into component_connector events. It bridges system-level Z-Wave events
 * (node added, node deleted, node information, externally assigned node ID) to the component_connector
 * event system.
 */
class system_events : public Initializable
{
    public:
        system_events();
        ~system_events() = default;

        // Initializable interface
        sl_status_t initialize() override;
        int shutdown() override;
        std::string name() const override;

    private:
        // Static callback functions for Z-Wave Controller
        static void on_node_information(zwave_node_id_t node_id, const zwave_node_info_t *node_info);
        static void on_node_added(sl_status_t status, const zwave_node_info_t *node_info, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol);
        static void on_node_exclusion_started(zwave_node_id_t node_id);
        static void on_node_deleted(zwave_node_id_t node_id);
        static void on_node_id_assigned(zwave_node_id_t node_id, bool included_by_us, zwave_protocol_t inclusion_protocol);
        static void on_network_address_update(zwave_home_id_t home_id, zwave_node_id_t node_id);
        static void on_new_network_entered(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type);
        // Reset-chain step: set the flag so on_new_network_entered knows the
        // controller is finishing a factory reset (versus a learn-mode join).
        static sl_status_t on_factory_reset_step();

        // Set by on_factory_reset_step, consumed by on_new_network_entered.
        static std::atomic<bool> factory_reset_pending;
};

#endif  // SYSTEM_EVENTS_HPP
