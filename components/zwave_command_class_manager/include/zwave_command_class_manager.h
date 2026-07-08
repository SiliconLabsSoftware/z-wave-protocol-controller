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

#ifndef ZWAVE_COMMAND_HANDLER_H
#define ZWAVE_COMMAND_HANDLER_H

/**
 * @defgroup zwave_command_handler Z-Wave Application Command Class Handler
 * @ingroup zpc_components
 * @brief Application Command Class handler framework,
 *  dispatching incoming Z-Wave Commands to the corresponding handler.
 *
 * This component takes care of keeping track of the list of supported
 * and controlled Command Classes.
 *
 * When a frame is received, it is in charge of verifying the security at
 * which the frame has been received and forward the Z-Wave Command accordingly.
 *
 * @{
 */

// Includes from other components
#include "zwave_controller_connection_info.h"
#include "zwave_controller.h"
#include "zwave_rx.h"
#include "sl_status.h"

// Interface includes
#include "zwave_command_class_base.h"  // zwave_command_class_base

// Generic includes
#include <stdbool.h>

#include <string>
#include <functional>
#include <set>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif

class zwave_command_class_manager
{
    public:
        zwave_command_class_manager()  = delete;
        ~zwave_command_class_manager() = default;

        /**
         * @brief Initialize the command handlers.
         * @returns SL_STATUS_OK, it will always be considered as successful.
         */
        static sl_status_t init();
        /**
         * @brief Teardown of the Z-Wave command handler.
         * @returns 0 in case of success.
         */
        static int teardown();

        /**
         * @brief Register a new command class
         *
         * @param command_class_handler The handler for the Command Class. This class will take ownership of the pointer.
         *
         * @returns SL_STATUS_OK    If the Command Class was successfully
         *                          registered
         * @returns SL_STATUS_FAIL  Otherwise
         */
        static sl_status_t register_command_class(zwave_command_class::zwave_command_class_base *command_class_handler);

        /**
         * @brief Get the version of Command Class that the handler handles.
         *
         * @param command_class Command class to query with.
         * @returns The version number of the indicated Command Class.
         *          0 if the Command Class is not supported (control-only or unregistered).
         */
        static uint8_t get_version(zwave_command_class_t command_class);

        // It is moved here to insert supervision decapsulated commands back to
        // the command handler. This may be updated with TX/RX validation scheme
        // component work.
        /**
         * @brief Dispatches a frame to its respective Command Class handler
         *
         * @param connection_info The connection information for the received Z-Wave
         *                        Frame
         * @param frame_data      The payload of the Z-Wave Frame
         * @param frame_length    The length of the payload (in bytes) contained
         *                        in the frame_data pointer.
         * @returns The handler return code
         */
        static sl_status_t dispatch(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length);

        /**
         * @brief Check if we control a given command class
         *
         * @param command_class
         * @return true if we control this command class
         */
        static bool controls(zwave_command_class_t command_class);

        /**
         * @brief Print Command Class Version info
         *
         * @param fd File descriptor to print to, if fd < 0 it will use sl_log_info
         */
        static void print_info(int fd);

        /**
         * Callback for entering a new network.
         * Refer to @ref zwave_controller_callbacks_t
         * @ref zwave_controller_callbacks_t on_new_network_entered for parameter description
         */
        static void zwave_command_handler_on_new_network_entered(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type);

        /**
         * Callback for receiving a Z-Wave frame.
         * Refer to @ref zwave_controller_callbacks_t
         * @ref zwave_controller_callbacks_t on_frame_received for parameter description
         */
        static void zwave_command_handler_on_frame_received(const zwave_controller_connection_info_t *connection_info, const zwave_rx_receive_options_t *rx_options, const uint8_t *frame_data, uint16_t frame_length);

    private:
        static zwave_command_class::zwave_command_class_base *get_command_class_handler(zwave_command_class_t command_class_id);

        static inline zwave_controller_encapsulation_scheme_t zpc_highest_scheme = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
        static const zwave_controller_callbacks_t zwave_command_handler_callbacks;
        static inline std::vector<std::unique_ptr<zwave_command_class::zwave_command_class_base>> command_handler_list;
};  // class zwave_command_class_handler_manager

#ifdef __cplusplus
}
#endif

/** @} end zwave_command_handler */

#endif /* ZWAVE_COMMAND_HANDLER_H */
