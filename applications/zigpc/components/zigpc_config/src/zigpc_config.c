/*******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include <string.h>

// Unify Shared components
#include "config.h"
#include "uic_version.h"

#include "zigpc_config.h"
#include "zigpc_config_validate.h"
#include "zigpc_config_fixt.h"

// List of default settings
static const char *CONFIG_KEY_ZIGPC_SERIAL_PORT = "zigpc.serial";
static const char *DEFAULT_SERIAL_PORT          = "/dev/ttyUSB0";
static const char *CONFIG_KEY_ZIGPC_USE_TC_WELL_KNOWN_KEY
  = "zigpc.tc_use_well_known_key";
static const bool DEFAULT_USE_TC_WELL_KNOWN_KEY = false;

static const char *CONFIG_KEY_ZIGPC_DATASTORE_FILE = "zigpc.datastore_file";
static const char *DEFAULT_ZIGPC_DATASTORE_FILE    = UIC_VAR_DIR "/zigpc.db";
static const char *CONFIG_KEY_ZIGPC_NCP_FIRMWARE_PATH
  = "zigpc.ncp_firmware_path";
static const char *CONFIG_FLAG_ZIGPC_NCP_UPDATE = "zigpc.ncp_update";

static const char *CONFIG_KEY_ZIGPC_OTA_PATH        = "zigpc.ota_path";
static const char *ZIGPC_DEFAULT_OTA_PATH           = UIC_VAR_DIR "/ota-files/";
static const char *CONFIG_FLAG_ZIGPC_POLL_ATTR_ONLY = "zigpc.poll_attr_only";
static const bool DEFAULT_POLL_ATTR_ONLY            = true;

static const char *CONFIG_USE_NETWORK_ARGS  = "zigpc.use_network_args";
static const char *CONFIG_NETWORK_PANID  = "zigpc.network_panid";
static const char *CONFIG_NETWORK_RADIO_POW  = "zigpc.network_radio_power";
static const char *CONFIG_NETWORK_CHANNEL  = "zigpc.network_channel";

static const char *CONFIG_KEY_ZIGPC_POLLING_RATE = "zigpc.attr_polling_rate_ms";
static const int ZIGPC_DEFAULT_POLLING_RATE_MS  = 10000;



static zigpc_config_t config;
int zigpc_config_init()
{
  memset(&config, 0, sizeof(config));

  /**
   * These are options which are supported both command line and in config file
   *
   * NOTE: the following options are already configured in
   * components/config/platform/posix/config.cpp:
   * - mqtt.host
   * - mqtt.port
   */
  config_status_t status = CONFIG_STATUS_OK;
  status |= config_add_string(CONFIG_KEY_ZIGPC_SERIAL_PORT,
                              "serial port to use",
                              DEFAULT_SERIAL_PORT);

  status |= config_add_string(CONFIG_KEY_ZIGPC_DATASTORE_FILE,
                              "Datastore database file",
                              DEFAULT_ZIGPC_DATASTORE_FILE);

  status
    |= config_add_bool(CONFIG_KEY_ZIGPC_USE_TC_WELL_KNOWN_KEY,
                       "Allow Trust Center joins using well-known link key",
                       DEFAULT_USE_TC_WELL_KNOWN_KEY);

  status |= config_add_flag(CONFIG_FLAG_ZIGPC_NCP_UPDATE,
                            "Flag to initiate an NCP firmware update");

  status |= config_add_string(CONFIG_KEY_ZIGPC_NCP_FIRMWARE_PATH,
                              "Path to firmware to update NCP",
                              "");  //No default path

  status |= config_add_string(CONFIG_KEY_ZIGPC_OTA_PATH,
                              "OTA file path for storing OTA firmware",
                              ZIGPC_DEFAULT_OTA_PATH);

  status |= config_add_bool(CONFIG_FLAG_ZIGPC_POLL_ATTR_ONLY,
                            "Flag to poll attributes only",
                            DEFAULT_POLL_ATTR_ONLY);
  status |= config_add_int(CONFIG_KEY_ZIGPC_POLLING_RATE,
                           "Polling rate in MS",
                           ZIGPC_DEFAULT_POLLING_RATE_MS);

  status |= config_add_flag(CONFIG_USE_NETWORK_ARGS,
                            "Specify PAN ID, radio power and channel of zigbee network");
  status |= config_add_int(CONFIG_NETWORK_PANID,
                           "PAN ID of zigbee network, required if using network args",
                           -1);
  status |= config_add_int(CONFIG_NETWORK_RADIO_POW,
                           "Power of zigbee radio, required if using network args",
                           -1);
  status |= config_add_int(CONFIG_NETWORK_CHANNEL,
                           "Zigbee network channel, required if using network args",
                           -1);

  return status != CONFIG_STATUS_OK;
}

sl_status_t zigpc_config_fixt_setup()
{
  config_status_t status
    = config_get_as_string(
            CONFIG_KEY_ZIGPC_SERIAL_PORT,
            &config.serial_port);

  status |=
      config_get_as_string(
            CONFIG_KEY_ZIGPC_DATASTORE_FILE,
            &config.datastore_file);

  status |=
      config_get_as_string(
            CONFIG_KEY_MQTT_HOST,
            &config.mqtt_host);
  status |=
      config_get_as_int(
            CONFIG_KEY_MQTT_PORT,
            &config.mqtt_port);

  status |=
      config_get_as_bool(
            CONFIG_KEY_ZIGPC_USE_TC_WELL_KNOWN_KEY,
            &config.tc_use_well_known_key);

  status |=
      config_get_as_string(
            CONFIG_KEY_ZIGPC_OTA_PATH,
            &config.ota_path);

  status |=
      config_get_as_bool(
            CONFIG_FLAG_ZIGPC_POLL_ATTR_ONLY,
            &config.poll_attr_only);
  status |=
      config_get_as_int(
            CONFIG_KEY_ZIGPC_POLLING_RATE,
            &config.attr_polling_rate_ms);

  status |=
      config_get_as_int(
            CONFIG_KEY_ZIGPC_POLLING_RATE,
            &config.attr_polling_rate_ms);

  config_status_t flag_status =
      config_has_flag(CONFIG_USE_NETWORK_ARGS);
  config.use_network_args = (flag_status == CONFIG_STATUS_OK);

  int temp_pan_id = 0;
  int temp_radio_power = 0;
  int temp_channel = 0;

  status |=
      config_get_as_int(
            CONFIG_NETWORK_PANID,
            &temp_pan_id );
  status |=
      config_get_as_int(
            CONFIG_NETWORK_RADIO_POW,
            &temp_radio_power);
  status |=
      config_get_as_int(
            CONFIG_NETWORK_CHANNEL,
            &temp_channel);

  status |=
      zigpc_config_network_args_validate(
              config.use_network_args,
              temp_pan_id,
              temp_radio_power,
              temp_channel);

  if(CONFIG_STATUS_OK == status)
  {
     config.network_pan_id = (uint16_t)temp_pan_id;
     config.network_radio_power = (int8_t)temp_radio_power;
     config.network_channel=(uint8_t)temp_channel;
  }

  return status == CONFIG_STATUS_OK ? SL_STATUS_OK : SL_STATUS_FAIL;
}

const zigpc_config_t *zigpc_get_config(void)
{
  return &config;
}
