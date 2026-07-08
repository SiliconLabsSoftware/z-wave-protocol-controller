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

#include "zwave_rx_process.hpp"
#include "zwapi_init.h"
#include "zwave_rx_internals.h"
#include "zpc_config.h"
#include "zwave_rf_region_config.h"
#include "sl_status.h"
#include "log.h"
#include <thread>
#include <sys/poll.h>
#include <mutex>
#include <atomic>
#include <errno.h>
#include <cstring>
#include <memory>

#define LOG_TAG "zwave_rx_process"

// Forward declaration
namespace zwave_component
{
    class zwave_rx_process;
}

// Global singleton instance for C API callbacks
// Declared here so it can be accessed from both C++ class methods and C wrapper functions
zwave_component::zwave_rx_process *zwave_rx_process_instance = nullptr;

namespace zwave_component
{
    zwave_rx_process::zwave_rx_process() : threading("Z-Wave RX Process"), zpc_connection_fd(-1), initialized(false) {}

    zwave_rx_process::~zwave_rx_process()
    {
        if (initialized) {
            shutdown();
        }
    }

    sl_status_t zwave_rx_process::initialize()
    {
        if (initialized) {
            return SL_STATUS_OK;
        }

        // Set the global singleton instance for C API callbacks
        // This matches what zwave_rx_process_init_and_start() does
        zwave_rx_process_instance = this;

        sl_status_t rx_init_status = SL_STATUS_FAIL;

        // Enable serial log before init, so we can see initialization frames too
        rx_init_status = zwapi_log_to_file_enable(zpc_get_config()->connection_log_file);

        // Configure type
        const zwapi_connection_params_t connection_params = {
          .serial_port = zpc_get_config()->serial_port,
          .ip_address  = zpc_get_config()->ip_address,
          .ip_port     = zpc_get_config()->ip_port,
        };

        // Now initialize the serial port
        if (SL_STATUS_OK == rx_init_status) {
            zwave_rf_region_t rf_region = ZWAVE_RF_REGION_UNDEFINED;
            // Validated in zpc_config_fixt_setup; failure here is defensive only.
            if (zwave_rf_region_config_from_string(zpc_get_config()->zwave_rf_region, &rf_region) != SL_STATUS_OK) {
                sl_log_error(LOG_TAG, "Invalid zpc.rf_region '%s'", zpc_get_config()->zwave_rf_region);
                return SL_STATUS_FAIL;
            }
            rx_init_status = zwave_rx_init(&connection_params, &zpc_connection_fd, zpc_get_config()->zwave_normal_tx_power_dbm, zpc_get_config()->zwave_measured_0dbm_power, zpc_get_config()->zwave_max_lr_tx_power_dbm, static_cast<zwave_controller_region_t>(rf_region));
        }

        if (SL_STATUS_OK != rx_init_status) {
            return rx_init_status;
        }

        initialized = true;
        return SL_STATUS_OK;
    }

    int zwave_rx_process::shutdown()
    {
        if (!initialized) {
            // Clear the global singleton instance even if not initialized
            if (zwave_rx_process_instance == this) {
                zwave_rx_process_instance = nullptr;
            }
            return 0;
        }
        stop();
        zwave_rx_shutdown();
        sl_status_t res = zwapi_log_to_file_disable();
        initialized     = false;
        // Clear the global singleton instance
        if (zwave_rx_process_instance == this) {
            zwave_rx_process_instance = nullptr;
        }
        return res;
    }

    std::string zwave_rx_process::name() const
    {
        return "Z-Wave RX";
    }

    void zwave_rx_process::run()
    {
        // Check if we should stop before doing any work
        if (should_stop() || threading::threading::is_kill_switch_activated()) {
            return;
        }

        pollfd pfd = {zpc_connection_fd, POLLIN, 0};
        auto value = poll_queue.try_pop();

        bool should_poll = value.has_value();

        // If queue is empty, wait for file descriptor to be ready
        // Use a timeout (100ms) so we can periodically check should_stop()
        if (!should_poll) {
            int poll_timeout_ms = 100;  // 100ms timeout to allow checking should_stop()
            int poll_result     = poll(&pfd, 1, poll_timeout_ms);

            // Check if we should stop after poll returns (or was interrupted)
            if (should_stop() || threading::threading::is_kill_switch_activated()) {
                return;
            }

            // Handle poll errors (EINTR is expected when signals are received)
            if (poll_result < 0) {
                if (errno == EINTR) {
                    // Interrupted by signal, check should_stop and continue
                    if (should_stop() || threading::threading::is_kill_switch_activated()) {
                        return;
                    }
                } else {
                    sl_log_error(LOG_TAG, "poll() failed: %s", strerror(errno));
                }
                should_poll = false;
            } else {
                should_poll = (poll_result > 0);
            }
        }

        // Process zwapi_poll if queue had a value or file descriptor is ready
        if (should_poll && zwapi_poll()) {
            poll_queue.push(0);
        }
        // sl_log_critical(LOG_TAG, "zw_rx_process: thread polling %d", zpc_connection_fd);
        std::this_thread::yield();
    }
}  // namespace zwave_component

extern "C" {

sl_status_t zwave_rx_process_init_and_start(int fd)
{
    // This C wrapper function is kept for backward compatibility
    // but the fd parameter is now ignored - initialization happens in initialize()
    (void)fd;
    try {
        zwave_rx_process_instance = new zwave_component::zwave_rx_process();
        sl_status_t status        = zwave_rx_process_instance->initialize();
        if (status != SL_STATUS_OK) {
            delete zwave_rx_process_instance;
            zwave_rx_process_instance = nullptr;
            return status;
        }
        return SL_STATUS_OK;
    } catch (const std::exception &e) {
        sl_log_error(LOG_TAG, "Failed to initialize Z-Wave RX process: %s", e.what());
        if (zwave_rx_process_instance != nullptr) {
            delete zwave_rx_process_instance;
            zwave_rx_process_instance = nullptr;
        }
        return SL_STATUS_FAIL;
    }
}

sl_status_t zwave_rx_process_stop_and_cleanup(void)
{
    try {
        if (zwave_rx_process_instance != nullptr) {
            zwave_rx_process_instance->shutdown();
            delete zwave_rx_process_instance;
            zwave_rx_process_instance = nullptr;
        }
        return SL_STATUS_OK;
    } catch (const std::exception &e) {
        sl_log_error(LOG_TAG, "Error during cleanup: %s", e.what());
        return SL_STATUS_FAIL;
    }
}

void zwave_rx_process_request_poll(void)
{
    if (zwave_rx_process_instance != nullptr) {
        zwave_rx_process_instance->poll_queue.push(0);
    }
}

}  // extern "C"