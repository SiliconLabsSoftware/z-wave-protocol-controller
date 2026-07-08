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

/**
 * @defgroup zpc_config ZPC Configuration Extension
 * @ingroup zpc_components
 *
 * @brief Add the ZPC-specific fixtures to the \ref config system.
 *
 * This module is the source of all ZPC-specific configuration parameters.
 * The source of the configuration parameters are command line arguments
 * and the configuration file.
 *
 * The after initialization the configurations parameters are constant.
 *
 * @{
 */

#if !defined(ZPC_CONFIG_H)
#define ZPC_CONFIG_H

// Generic includes
#include <stdint.h>
#include <stdbool.h>
// ZPC includes
#include "zpc_version.h"

// Inclusion protocol preference values
/// Z-Wave
#define ZWAVE_CONFIG_REPRESENTATION "1"
/// Z-Wave Long Range
#define ZWAVE_LONG_RANGE_CONFIG_REPRESENTATION "2"

// Default setting for the zpc.datastore_file.
#define DEFAULT_ZPC_DATASTORE_FILE "/zpc.db"
// Config key for the ZPC datastore file
#define CONFIG_KEY_ZPC_DATASTORE_FILE "zpc.datastore_file"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
        /// Hostname of the MQTT broker
        const char *mqtt_host;
        /// Path to a file containing the PEM encoded trusted CA certificate files.
        const char *mqtt_cafile;
        /// Path to a file containing the PEM encoded certificate file for this client.
        const char *mqtt_certfile;
        /// Path to a file containing the PEM encoded unencrypted private key for this client.
        const char *mqtt_keyfile;
        /// Client ID for MQTT Client for TLS Authentication and encryption
        const char *mqtt_client_id;
        /// Pre shared Key for MQTT Client for TLS Authentication and encryption
        const char *mqtt_client_psk;
        /// Port of the MQTT broker
        int mqtt_port;
        /// File name for persistent storage
        const char *datastore_file;
        /// Name of the serial port of the Z-Wave module
        const char *serial_port;
        /// IP address of the Z-Wave module
        const char *ip_address;
        /// IP port of the Z-Wave module
        int ip_port;
        /// If set the connection log will be written here
        const char *connection_log_file;
        /// Z-Wave RF region config string (see zwave_rf_region_config.h for valid names).
        const char *zwave_rf_region;
        /// Transmit power for the Z-Wave module. Refer to \ref zwave_rx_init
        int zwave_normal_tx_power_dbm;
        /// Measured 0dBm output power for the Z-Wave module.
        /// Refer to \ref zwave_rx_init
        int zwave_measured_0dbm_power;
        /// Max Z-Wave Long Range Transmit power
        int zwave_max_lr_tx_power_dbm;
        ///< Default wake up interval to be used when including wake up nodes
        int default_wake_up_interval;
        uint16_t manufacturer_id;
        uint16_t product_type;
        uint16_t product_id;
        const char *device_id;
        ///< hardware version of the device where ZPC is running on.
        int hardware_version;
        ///< This value represents a maximum number of missing wake up periods.
        uint8_t missing_wake_up_notification;
        ///< Number of consecutive failed Z-Wave send-data attempts (one per outgoing
        ///< application command) before an AL/FL node is marked OFFLINE. With value N,
        ///< the node goes OFFLINE on the N-th consecutive failure. Radio-level and
        ///< S2/Supervision retries are not counted. Counter resets on any successful
        ///< TX or RX from the node. Does not apply to sleeping (NL) nodes.
        uint8_t accepted_transmit_failure;
        ///< Prioritized list of protocols to use for SmartStart inclusions
        const char *inclusion_protocol_preference;
        ///< OTA cache path, writable location where we can cache OTA images
        const char *ota_cache_path;

        ///< Master switch for the Security Keys Dump MQTT request. Defaults
        ///< to false.
        bool security_keys_dump_enable;
        ///< Path to a PEM-encoded X25519 recipient public key. The dump file
        ///< is encrypted with this key.
        const char *security_keys_dump_recipient_pubkey_path;
        ///< Destination directory for the encrypted dump file. The file
        ///< basename is always <home_id>.bin and is written with mode 0600.
        const char *security_keys_dump_output_dir;

        ///< Should we return the NCP version and exit?
        bool ncp_version;
        /// If not zero length we should flash the NCP firmware and exit
        const char *ncp_update_filename;

        /// The following three configuration types describes the Node type structure
        /// that will be used in the ZPC Node Information Frames.
        // Z-Wave Basic Device Type
        uint8_t zpc_basic_device_type;
        /// Z-Wave Generic Device Type
        uint8_t zpc_generic_device_type;
        /// Z-Wave Specific Device Type
        uint8_t zpc_specific_device_type;
} zpc_config_t;

/**
 * @brief Get the current configuration. This must only be called after
 * zpc_config_init.
 */
const zpc_config_t *zpc_get_config();

/**
 * @brief Register ZPC configurations in \ref config.
 *
 * This must be called before ZPC main (see main.cpp).
 *
 * @returns 0 on success.
 */
int zpc_config_init();

#ifdef __cplusplus
}
#endif

/** @} end zpc_config */

#endif  // ZPC_CONFIG_H
