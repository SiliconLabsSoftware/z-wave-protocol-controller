/*******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

// Includes from this component
#include "zpc_config.h"
#include "device_id.h"

// ZPC components
#include "config.h"
#include "log.h"

// Generic includes
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

// Interfaces
#include "zpc_version.h"
#include "ZW_classcmd.h"
#include "zwave_rf_region_config.h"

#define LOG_TAG "zpc_config"

static char zpc_rf_region_config_description[512];

// List of default settings
#define DEFAULT_ZWAVE_NORMAL_TX_POWER_DBM                   0
#define DEFAULT_ZWAVE_MEASURED_0DBM_POWER                   0
#define DEFAULT_ZWAVE_MAX_LR_TX_POWER_DBM                   0
#define DEFAULT_SERIAL_PORT                                 "/dev/ttyUSB0"
#define DEFAULT_IP_ADDRESS                                  "localhost"
#define DEFAULT_IP_PORT                                     4901
#define DEFAULT_WAKE_UP_INTERVAL                            4200
#define DEFAULT_MANUFACTURER_ID                             0x0000
#define DEFAULT_PRODUCT_TYPE                                0x0005
#define DEFAULT_PRODUCT_ID                                  0x0001
#define DEFAULT_HARDWARE_VERSION                            1
#define DEFAULT_NUMBER_OF_MISSING_WAKE_UP_NOTIFICATION      2
#define DEFAULT_NUMBER_OF_ACCEPTED_FRAME_TRANSMISSION_ERROR 2
#define DEFAULT_INCLUSION_PROTOCOL_PREFERENCE               "1,2"
#define DEFAULT_OTA_CACHE_PATH                              "/tmp/ota_cache"
#define ZPC_DEVICE_ID_MAX_HEX_CHARS                         (0x1FU * 2U)

// Config keys
#define ZPC_SERIAL                        "zpc.serial"
#define ZPC_IP_ADDRESS                    "zpc.ip_address"
#define ZPC_IP_PORT                       "zpc.ip_port"
#define ZPC_RF_REGION                     "zpc.rf_region"
#define ZPC_MANUFACTURER_ID               "zpc.manufacturer_id"
#define ZPC_PRODUCT_TYPE                  "zpc.product_type"
#define ZPC_PRODUCT_ID                    "zpc.product_id"
#define ZPC_DEVICE_ID                     "zpc.device_id"
#define ZPC_NORMAL_TX_POWER_DBM           "zpc.normal_tx_power_dbm"
#define ZPC_MEASURED_0DBM_POWER           "zpc.measured_0dbm_power"
#define ZPC_MAX_LR_TX_POWER_DBM           "zpc.max_lr_tx_power_dbm"
#define ZPC_CONNECTION_LOG_FILE           "zpc.connection_log_file"
#define ZPC_ACCEPTED_TRANSMIT_FAILURE     "zpc.accepted_transmit_failure"
#define ZPC_HARDWARE_VERSION              "zpc.hardware_version"
#define ZPC_DEFAULT_WAKE_UP_INTERVAL      "zpc.default_wake_up_interval"
#define ZPC_MISSING_WAKE_UP_NOTIFICATION  "zpc.missing_wake_up_notification"
#define ZPC_INCLUSION_PROTOCOL_PREFERENCE "zpc.inclusion_protocol_preference"
#define ZPC_CONFIG_NCP_VERSION            "zpc.ncp_version"
#define ZPC_CONFIG_NCP_UPDATE             "zpc.ncp_update"
#define ZPC_OTA_CACHE_PATH                "zpc.ota_cache_path"

#define ZPC_SECURITY_KEYS_DUMP_ENABLE                "security.security_keys_dump_enable"
#define ZPC_SECURITY_KEYS_DUMP_RECIPIENT_PUBKEY_PATH "security.security_keys_dump_recipient_pubkey_path"
#define ZPC_SECURITY_KEYS_DUMP_OUTPUT_DIR            "security.security_keys_dump_output_dir"

#define ZPC_BASIC_DEVICE_TYPE    "zpc.device_type.basic"
#define ZPC_GENERIC_DEVICE_TYPE  "zpc.device_type.generic"
#define ZPC_SPECIFIC_DEVICE_TYPE "zpc.device_type.specific"

static zpc_config_t config;

int zpc_config_init()
{
    memset(&config, 0, sizeof(config));

    // These are options which are supported both command line and in config file
    config_status_t status = CONFIG_STATUS_OK;

    status |= config_add_string(CONFIG_KEY_ZPC_DATASTORE_FILE, "ZPC datastore database file", DEFAULT_ZPC_DATASTORE_FILE);
    status |= config_add_string(ZPC_SERIAL, "Serial port where Z-Wave module is connected", DEFAULT_SERIAL_PORT);
    status |= config_add_string(ZPC_IP_ADDRESS, "IP address where Z-Wave module is connected", DEFAULT_IP_ADDRESS);
    status |= config_add_int(ZPC_IP_PORT, "IP port where Z-Wave module is connected", DEFAULT_IP_PORT);
    if (zwave_rf_region_config_format_description(zpc_rf_region_config_description, sizeof zpc_rf_region_config_description) < 0) {
        sl_log_error(LOG_TAG, "Failed to format zpc.rf_region help description");
        status |= CONFIG_STATUS_ERROR;
    }
    status |= config_add_string(ZPC_RF_REGION, zpc_rf_region_config_description, zwave_rf_region_config_default_name());
    status |= config_add_int(ZPC_MANUFACTURER_ID, "Manufacturer ID (16 bit Decimal)", DEFAULT_MANUFACTURER_ID);
    status |= config_add_int(ZPC_PRODUCT_TYPE, "Product Type (16 bit Decimal)", DEFAULT_PRODUCT_TYPE);
    status |= config_add_int(ZPC_PRODUCT_ID, "Product ID (16 bit Decimal)", DEFAULT_PRODUCT_ID);
    status |= config_add_string(ZPC_DEVICE_ID, "Device Specific ID", get_device_id());
    status |= config_add_int(ZPC_NORMAL_TX_POWER_DBM,
                             "Z-Wave normal tx power (deci dBm), The power level"
                             " used when transmitting frames at normal power. The"
                             " power level is in deci dBm. E.g. 1dBm output power"
                             "will be 10 in normal_tx_power_dbm and -2dBm  will "
                             "be -20 in normal_tx_power_dbm. Not all Z-Wave "
                             "modules support this setting and it will be "
                             "applied only with compatible Z-Wave APIs.",
                             DEFAULT_ZWAVE_NORMAL_TX_POWER_DBM);
    status |= config_add_int(ZPC_MEASURED_0DBM_POWER,
                             "Z-Wave measured 0dBm output power (deci dBm). The "
                             " output power measured from the antenna when "
                             "normal_tx_power_dbm is set to 0dBm. The power level"
                             " is in deci dBm. E.g. 1dBm output power will be 10 "
                             "in measured_0dbm_power and -2dBm will be -20 in "
                             "measured_0dbm_power. Calibration value which should"
                             " be adjusted to the physical z-wave module. "
                             "This value can be obtained by measuring the output"
                             " power of the antenna when normal_tx_power_dbm"
                             " Not all Z-Wave modules support this setting and "
                             "it will be applied only with compatible Z-Wave "
                             "APIs.",
                             DEFAULT_ZWAVE_MEASURED_0DBM_POWER);
    status |= config_add_int(ZPC_MAX_LR_TX_POWER_DBM,
                             "Z-Wave Long Range Transmit power (deci dBm),"
                             " power level is in deci dBm. E.g. 1dBm output power"
                             "will be 10 in max_lr_tx_power_dbm and -2dBm  will "
                             "be -20 in max_lr_tx_power_dbm. Not all Z-Wave "
                             "modules support this setting and it will be "
                             "applied only with compatible Z-Wave APIs.",
                             DEFAULT_ZWAVE_MAX_LR_TX_POWER_DBM);
    status |= config_add_int(ZPC_DEFAULT_WAKE_UP_INTERVAL,
                             "Default wake up interval in seconds, which will be "
                             "configured on sleeping (NL) Z-Wave nodes after "
                             "inclusion. Used as default only if there are no"
                             " certification requirements. Also note that if the"
                             "sleeping (NL) Z-Wave device does NOT advertise a "
                             "default Wake Up internal in the Wake Up Command "
                             "Class, this value is used.",
                             DEFAULT_WAKE_UP_INTERVAL);

    status |= config_add_string(ZPC_CONNECTION_LOG_FILE,
                                "If this is set, the ZPC will write a log of the "
                                "communication interface with the "
                                "Z-Wave module to the path provided. "
                                "If the file exists, the log will be appended to "
                                "this file, otherwise the file will be created. "
                                "The ZPC will NOT handle log rotation etc.",
                                "");
    status |= config_add_int(ZPC_HARDWARE_VERSION,
                             "A unique hardware version value that identifies "
                             "the hardware version of the device where zpc is running on.",
                             DEFAULT_HARDWARE_VERSION);

    status |= config_add_int(ZPC_MISSING_WAKE_UP_NOTIFICATION,
                             "This value represents a maximum number of missing wake up periods."
                             "If the sleeping nodes missed issuing wake up notification for more than"
                             "a value defined here, the node shall be considered as failing node.",
                             DEFAULT_NUMBER_OF_MISSING_WAKE_UP_NOTIFICATION);

    status |= config_add_int(ZPC_ACCEPTED_TRANSMIT_FAILURE,
                             "Number of consecutive failed Z-Wave send-data attempts (one per "
                             "outgoing application command, typically one per MQTT command) "
                             "before an AL/FL node is marked OFFLINE. With value N, the node "
                             "goes OFFLINE on the N-th consecutive failure. Radio-level retries, "
                             "S2/Supervision retries, and MQTT-level errors are NOT counted. "
                             "The counter resets on any successful TX or RX from the node. "
                             "Does not apply to sleeping (NL) nodes.",
                             DEFAULT_NUMBER_OF_ACCEPTED_FRAME_TRANSMISSION_ERROR);

    status |= config_add_string(ZPC_INCLUSION_PROTOCOL_PREFERENCE,
                                "This value represents a prioritized list of protocols to prefer when "
                                "including Z-Wave nodes with SmartStart, when the SmartStart list does "
                                "not indicate any preference. The protocols are represented as follow:"
                                "\n  1: Z-Wave"
                                "\n  2: Z-Wave Long Range"
                                "\nFor example, '1,2' means Z-Wave, then Z-Wave Long Range. "
                                "'2' For Z-Wave Long Range only.",
                                DEFAULT_INCLUSION_PROTOCOL_PREFERENCE);

    status |= config_add_flag(ZPC_CONFIG_NCP_VERSION, "Print the NCP firmaware version and exit");
    status |= config_add_string(ZPC_CONFIG_NCP_UPDATE, "Update the NCP firmware and exit", "");

    status |= config_add_int(ZPC_BASIC_DEVICE_TYPE,
                             "The ZPC Basic Device Type identification that will "
                             "be used in the ZPC Node Information Frames",
                             BASIC_TYPE_STATIC_CONTROLLER);
    status |= config_add_int(ZPC_GENERIC_DEVICE_TYPE,
                             "The ZPC Generic Device Type identification that "
                             "will be used in the ZPC Node Information Frame",
                             GENERIC_TYPE_GENERIC_CONTROLLER);
    status |= config_add_int(ZPC_SPECIFIC_DEVICE_TYPE,
                             "The ZPC Specific Device Type identification that "
                             "will be used in the ZPC Node Information Frame",
                             SPECIFIC_TYPE_NOT_USED);
    status |= config_add_string(ZPC_OTA_CACHE_PATH, "OTA cache path", DEFAULT_OTA_CACHE_PATH);

    status |= config_add_bool(ZPC_SECURITY_KEYS_DUMP_ENABLE,
                              "Master switch for the encrypted Security Keys Dump MQTT request. "
                              "Disabled by default. When enabled, the topic "
                              "zpc/<home_id>/Network/DumpSecurityKeys becomes reachable and a "
                              "recipient public key + output path must also be configured.",
                              false);
    status |= config_add_string(ZPC_SECURITY_KEYS_DUMP_RECIPIENT_PUBKEY_PATH,
                                "Path to a PEM-encoded X25519 public key. The dump file is "
                                "encrypted to this key.",
                                "");
    status |= config_add_string(ZPC_SECURITY_KEYS_DUMP_OUTPUT_DIR,
                                "Destination directory for the encrypted security keys dump "
                                "file. The file basename is always <home_id>.bin and is "
                                "written with mode 0600.",
                                "");

    return status != CONFIG_STATUS_OK;
}

sl_status_t is_hex_string(const char *str)
{
    if (str == NULL) {
        sl_log_error(LOG_TAG, "zpc.device_id is not set in conf file");
        return CONFIG_STATUS_ERROR;
    }
    size_t len = strnlen(str, ZPC_DEVICE_ID_MAX_HEX_CHARS + 1);
    if (len > ZPC_DEVICE_ID_MAX_HEX_CHARS) {
        sl_log_error(LOG_TAG, "zpc.device_id in conf file exceeds maximum length of %u hex digits", ZPC_DEVICE_ID_MAX_HEX_CHARS);
        return CONFIG_STATUS_ERROR;
    }
    if (len % 2 != 0) {
        sl_log_error(LOG_TAG,
                     "Invalid length of zpc.device_id in conf file. "
                     "Expected hexstring (even number of hex digits)");
        return CONFIG_STATUS_ERROR;
    }
    for (size_t i = 0; i < len; i++) {
        if (!isxdigit((unsigned char)str[i])) {
            sl_log_error(LOG_TAG,
                         "Invalid character %c(%zuth) in zpc-device_id in "
                         "conf file. Expected hexstring",
                         str[i],
                         i);
            return CONFIG_STATUS_ERROR;
        }
    }
    return CONFIG_STATUS_OK;
}

static int config_get_int_safe(const char *key)
{
    int val = 0;
    if (SL_STATUS_OK != config_get_as_int(key, &val)) {
        sl_log_error(LOG_TAG, "Failed to get int for key: %s", key);
        assert(false);
    }
    return val;
}

sl_status_t zpc_config_fixt_setup()
{
    config_status_t status = CONFIG_STATUS_OK;
    status |= config_get_as_string(CONFIG_KEY_ZPC_DATASTORE_FILE, &config.datastore_file);
    status |= config_get_as_string(ZPC_SERIAL, &config.serial_port);
    status |= config_get_as_string(ZPC_IP_ADDRESS, &config.ip_address);
    status |= config_get_as_int(ZPC_IP_PORT, &config.ip_port);
    status |= config_get_as_string(ZPC_RF_REGION, &config.zwave_rf_region);
    {
        zwave_rf_region_t rf_region;
        char valid_list[256];
        if (zwave_rf_region_config_from_string(config.zwave_rf_region, &rf_region) != SL_STATUS_OK) {
            if (zwave_rf_region_config_format_valid_list(valid_list, sizeof valid_list) < 0) {
                valid_list[0] = '\0';
            }
            sl_log_error(LOG_TAG, "Invalid zpc.rf_region '%s'. Valid values: %s", config.zwave_rf_region, valid_list);
            status |= CONFIG_STATUS_ERROR;
        }
    }
    config.manufacturer_id = config_get_int_safe(ZPC_MANUFACTURER_ID);
    config.product_type    = config_get_int_safe(ZPC_PRODUCT_TYPE);
    config.product_id      = config_get_int_safe(ZPC_PRODUCT_ID);
    status |= config_get_as_string(ZPC_DEVICE_ID, &config.device_id);
    status |= is_hex_string(config.device_id);
    status |= config_get_as_string(CONFIG_KEY_MQTT_HOST, &config.mqtt_host);
    status |= config_get_as_string(CONFIG_KEY_MQTT_CAFILE, &config.mqtt_cafile);
    status |= config_get_as_string(CONFIG_KEY_MQTT_CERTFILE, &config.mqtt_certfile);
    status |= config_get_as_string(CONFIG_KEY_MQTT_KEYFILE, &config.mqtt_keyfile);
    config.mqtt_port                    = config_get_int_safe(CONFIG_KEY_MQTT_PORT);
    config.zwave_measured_0dbm_power    = config_get_int_safe(ZPC_MEASURED_0DBM_POWER);
    config.zwave_normal_tx_power_dbm    = config_get_int_safe(ZPC_NORMAL_TX_POWER_DBM);
    config.zwave_max_lr_tx_power_dbm    = config_get_int_safe(ZPC_MAX_LR_TX_POWER_DBM);
    config.default_wake_up_interval     = config_get_int_safe(ZPC_DEFAULT_WAKE_UP_INTERVAL);
    config.hardware_version             = config_get_int_safe(ZPC_HARDWARE_VERSION);
    config.accepted_transmit_failure    = config_get_int_safe(ZPC_ACCEPTED_TRANSMIT_FAILURE);
    config.missing_wake_up_notification = config_get_int_safe(ZPC_MISSING_WAKE_UP_NOTIFICATION);

    status |= config_get_as_string(ZPC_INCLUSION_PROTOCOL_PREFERENCE, &config.inclusion_protocol_preference);
    status |= config_get_as_string(ZPC_CONNECTION_LOG_FILE, &config.connection_log_file);

    config.ncp_version = config_has_flag(ZPC_CONFIG_NCP_VERSION) == CONFIG_STATUS_OK;
    status |= config_get_as_string(ZPC_CONFIG_NCP_UPDATE, &config.ncp_update_filename);

    config.zpc_basic_device_type    = (uint8_t)config_get_int_safe(ZPC_BASIC_DEVICE_TYPE);
    config.zpc_generic_device_type  = (uint8_t)config_get_int_safe(ZPC_GENERIC_DEVICE_TYPE);
    config.zpc_specific_device_type = (uint8_t)config_get_int_safe(ZPC_SPECIFIC_DEVICE_TYPE);

    status |= config_get_as_string(ZPC_OTA_CACHE_PATH, &config.ota_cache_path);

    status |= config_get_as_bool(ZPC_SECURITY_KEYS_DUMP_ENABLE, &config.security_keys_dump_enable);
    status |= config_get_as_string(ZPC_SECURITY_KEYS_DUMP_RECIPIENT_PUBKEY_PATH, &config.security_keys_dump_recipient_pubkey_path);
    status |= config_get_as_string(ZPC_SECURITY_KEYS_DUMP_OUTPUT_DIR, &config.security_keys_dump_output_dir);

    return status == CONFIG_STATUS_OK ? SL_STATUS_OK : SL_STATUS_FAIL;
}

const zpc_config_t *zpc_get_config()
{
    return &config;
}
