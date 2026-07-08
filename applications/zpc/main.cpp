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

#include <cstdio>
#include <string_view>
#include <csignal>
#include <cstring>
#include <thread>
#include <chrono>

// config.h and timer.hpp have C++ sections, so include them outside extern "C"
#include "config.h"
#include "timer.hpp"

// C includes - wrap in extern "C" for C++ compatibility
// These must come first because C++ handler headers depend on Z-Wave type definitions
extern "C" {
#include "sl_status.h"
#include "log.h"

// Z-Wave type definitions (must come before C++ handler headers)
#include "zwave_controller.h"
#include "zwave_network_management_fixt.h"

// Component fixture includes
#include "zpc_attribute_resolver_fixt.h"
#include "attribute_store_fixt.h"
#include "attribute_store.h"
#include "attribute_timeouts.h"
#include "datastore_fixt.h"
#include "zpc_config_fixt.h"
#include "zpc_config.h"
#include "zpc_attribute_store.h"
#include "zwave_s2_fixt.h"
#include "zwave_transports_fixt.h"
#include "zpc_datastore_fixt.h"
#include "zwave_command_class_manager_fixt.h"

}  // extern "C"

// Threading includes
#include "threading.hpp"

// Define LOG_TAG before including init_builder.hpp (needed for logging)
[[maybe_unused]] static constexpr std::string_view LOG_TAG = "zpc";

// C++ handler includes (after Z-Wave type definitions)
#include "smartstart.hpp"
#include "network_management_handler.hpp"
#include "attribute_store_handler.hpp"
#include "attribute_resolver_handler.hpp"
#include "network_monitor.hpp"
#include "mqtt_handler.hpp"
#include "zwave_network_management_handler.hpp"
#include "zwave_rx_process.hpp"
#include "zwave_tx_process.hpp"
// Non-command class components
#include "granted_keys_resolver.hpp"
#include "node_info_resolver.hpp"
#include "device_interviewer.hpp"
#include "update_manager.hpp"
#include "system_events.hpp"
// Initialization builder (must come after log.h and LOG_TAG definition)
#include "init_builder.hpp"
#include "discovery_mqtt_api.hpp"
#include "security_keys_dump_mqtt_api.hpp"

/**
 * @brief Signal handler for SIGINT and SIGTERM
 * Activates the kill switch to gracefully shutdown all threads
 */
static void signal_handler(int sig)
{
    (void)sig;
    threading::threading::kill_switch_activate();
}

/**
 * @brief Setup signal handlers for SIGINT and SIGTERM
 *
 * Uses sigaction() instead of signal() for reliable POSIX semantics.
 */
static sl_status_t setup_signal_handlers(void)
{
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    // Do not set SA_RESTART so that blocking calls (select, sleep, etc.)
    // return with EINTR, allowing threads to check the kill switch promptly.
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) != 0) {
        return SL_STATUS_FAIL;
    }

    if (sigaction(SIGTERM, &sa, nullptr) != 0) {
        return SL_STATUS_FAIL;
    }

    return SL_STATUS_OK;
}

/**
 * @brief Setup system configuration and logging
 */
static sl_status_t setup_config_and_logging(int argc, char **argv, const char *version)
{
    int ret = 0;
    ret     = config_parse(argc, argv, version);
    if (ret != 0) {
        if (ret == CONFIG_STATUS_INFO_MESSAGE) {
            return SL_STATUS_PRINT_INFO_MESSAGE;
        }
        if (ret != CONFIG_STATUS_OK) {
            sl_log_critical(LOG_TAG.data(), "Failed to parse configuration.\n");
            return SL_STATUS_FAIL;
        }
    }
    if (config_apply_log_settings() != CONFIG_STATUS_OK) {
        sl_log_critical(LOG_TAG.data(), "Failed to apply logging configuration.\n");
        return SL_STATUS_FAIL;
    }
    // Instead of sl_log, use std::cout to print the version, because it must be visible all the time.
    std::cout << "ZPC build version: " << version << std::endl;
    return SL_STATUS_OK;
}

int main(int argc, char **argv)
{
    // Early config initialization (before zpc_config_fixt_setup)
    if (zpc_config_init() != 0) {
        return -1;
    }

    // Setup configuration and logging
    sl_status_t status = setup_config_and_logging(argc, argv, ZPC_VERSION);
    if (status != SL_STATUS_OK) {
        if (status == SL_STATUS_PRINT_INFO_MESSAGE) {
            return 0;  // Info message printed, exit normally
        }
        return -1;
    }

    // Build initialization sequence
    InitBuilder builder;

    // Initialize system components in order
    builder

      /**
       * Initialize Component Connector.
       * This component manages event routing between components.
       * Must be initialized before command classes and device_interviewer.
       */
      .add(std::make_unique<component_connector>())

      /**
       * Initialize System Events.
       * This component registers Z-Wave Controller callbacks and converts them
       * into component_connector events. Must be initialized after component_connector
       * but before Z-Wave Controller starts processing events.
       */
      .add(std::make_unique<system_events>())

      /** Initialize zpc_config. NB: This shall be first in the setup fixtures list */
      .add("ZPC Config", zpc_config_fixt_setup)

      /** Initialize data store.  The data-store component depends on the
       * configuration component for file locations. */
      .add("Datastore", zpc_datastore_fixt_setup)

      /**
       * Initialize the attribute store library.
       * The datastore must be initialized first.
       */
      .add("Attribute store", attribute_store_init, attribute_store_teardown)

      /** Connect to the Z-Wave API interface and initialize the RX part.
       * The Z-Wave API depends on the configuration component.
       * Thread will be started after all callbacks are registered. */
      .add(std::make_unique<zwave_component::zwave_rx_process>())

      /** Start Z-Wave TX.
       * Thread will be started after all callbacks are registered. */
      .add(std::make_unique<zwave_component::zwave_tx_process>())

      /** Let the Network management initialize its state machine
        and cache our NodeID/ HomeID from the Z-Wave API */
      .add("Z-Wave Network Management", zwave_network_management_fixt_setup, zwave_network_management_fixt_teardown)

      /** Start Z-Wave S2 */
      .add("Z-Wave S2", zwave_s2_fixt_setup)

      /** Initializes all Z-Wave transports */
      .add("Z-Wave Transports", zwave_transports_init)

      /**
       * Initializes the attribute timeouts functionality.
       * The attribute store must be initialized first
       */
      .add("Attribute timeouts", attribute_timeouts_init, attribute_timeouts_teardown)

      /**
       * Initialize the ZPC attribute resolver
       * Keep that right before the network monitor,
       * in order to get proper paused/enabled resolution right after init.
       */
      .add("ZPC attribute resolver", zpc_attribute_resolver_init, zpc_attribute_resolver_teardown)

      /**
       * Network monitor starts and aligns the attribute store with the ZPC address
       * and verify that everything that should be in the Attribute Store
       * is there.
       * Requires Network Management, Attribute Store and ZPC Attribute Resolver
       * to be initialized.
       * It pauses the resolution of other networks and activates our network
       * for the resolver.
       */
      .add(std::make_unique<zwave_component::network_monitor_handler>())

      /**
       * Initialize MQTT Handler.
       * MqttApi component depends on MQTT Handler for subscribing to topics,
       * so it must be initialized before the non-command class components.
       */
      .add(zwave_component::mqtt_handler::create_initializable_wrapper())

      /**
       * Initialize non-command class components.
       * These components must be initialized after Network Monitor Handler
       * (which ensures Network Management has set the HomeID) and MQTT Handler,
       * and before Command Classes Manager, as command classes fire events to these
       * components during registration.
       *
       * Dependencies:
       * - Network Management: MqttApi needs HomeID for MQTT topic subscriptions
       * - MQTT Handler: MqttApi needs MQTT Handler for topic subscriptions
       * - Attribute Store: All components register callbacks
       * - Attribute Resolver: Granted Keys and Node Info resolvers register rules
       * - Command Class Connector: All components connect to events
       */
      .add(std::make_unique<zwave_command_class::granted_keys_resolver>())
      .add(std::make_unique<zwave_command_class::node_info_resolver>())
      .add(std::make_unique<zwave_command_class::device_interviewer>())
      .add(std::make_unique<ota::update_manager>())

      /**
       * Initialize Z-Wave Command Classes Manager.
       * This component manages all Z-Wave command class handlers.
       * Requires the four non-command class components above to be initialized,
       * as command classes fire events to them during registration.
       */
      .add("Z-Wave Command Classes Manager", zwave_command_class_manager_init)

      /**
       * Initialize C++ handler threads.
       * These threads handle various Z-Wave and MQTT events.
       * Threads will be started automatically after all components are initialized.
       * Handlers inherit from Initializable and are managed by the InitBuilder.
       */
      .add(std::make_unique<zwave_component::network_management_handler>())
      .add(std::make_unique<zwave_component::smartstart_handler>())
      .add(std::make_unique<zwave_component::attribute_store_handler>())
      .add(std::make_unique<zwave_component::attribute_resolver_handler>())
      .add(std::make_unique<zwave_component::zwave_network_management_handler>())

      /**
       * Initialize Discovery MQTT API.
       * Must be initialized after mqtt_handler is ready.
       * Note: NetworkManagementMqttApi and SmartStartMqttApi are initialized
       * in their respective handlers (network_management_handler and smartstart_handler).
       */
      .add(std::make_unique<zwave_command_class::DiscoveryMqttApi>())

      /**
       * Initialize Security Keys Dump MQTT API.
       * Disabled by default; see components/security/doc for details.
       */
      .add(std::make_unique<zwave_command_class::SecurityKeysDumpMqttApi>())

      /**
       * Initialize attribute_store specifically for ZPC.
       * Requires Network Management, Attribute Store, and all handler threads to be initialized.
       * This component will trigger an update callback for all attributes under our current HomeID.
       * It must be initialized after handler threads are ready to process callbacks.
       */
      .add("ZPC Attribute Store", zpc_attribute_store_init);

    // Initialize all components in the order specified above.
    // Threads will be started automatically after all components are initialized.
    // This prevents race conditions where frames are received before handlers are ready.
    status = builder.initialize_all();
    if (status != SL_STATUS_OK) {
        sl_log_critical(LOG_TAG.data(), "Failed to initialize components: %d\n", status);
        return -1;
    }

    status = setup_signal_handlers();
    if (status != SL_STATUS_OK) {
        sl_log_critical(LOG_TAG.data(), "Failed to setup signal handlers\n");
        return -1;
    }

    // Main event loop: wait for shutdown signal (SIGINT or SIGTERM).
    // All actual work is performed by background threads, so the main thread
    // simply polls the kill switch and sleeps until shutdown is requested.
    while (!threading::threading::is_kill_switch_activated()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup: shutdown all components in reverse order
    int shutdown_errors = builder.shutdown_all();
    if (shutdown_errors > 0) {
        sl_log_warning(LOG_TAG.data(), "Some components failed to shutdown properly: %d\n", shutdown_errors);
    }

    return shutdown_errors;
}
