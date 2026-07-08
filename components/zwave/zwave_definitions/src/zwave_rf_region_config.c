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

#include "zwave_rf_region_config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/** Longest entry in zwave_rf_region_config_table is "EU_LR" (5 chars). */
#define ZWAVE_RF_REGION_CONFIG_NAME_MAX 16

typedef struct {
        const char *config_name;
        const char *description;
        zwave_rf_region_t region;
} zwave_rf_region_config_entry_t;

/** Single source of ZPC-supported RF region config names. */
static const zwave_rf_region_config_entry_t zwave_rf_region_config_table[] = {
  {"EU", "Europe", ZWAVE_RF_REGION_EU},
  {"EU_LR", "EU Long Range", ZWAVE_RF_REGION_EU_LR},
  {"US", "US", ZWAVE_RF_REGION_US},
  {"US_LR", "US Long Range", ZWAVE_RF_REGION_US_LR},
  {"ANZ", "Australia / New Zealand", ZWAVE_RF_REGION_ANZ},
  {"HK", "Hong Kong", ZWAVE_RF_REGION_HK},
  {"IN", "India", ZWAVE_RF_REGION_IN},
  {"IL", "Israel", ZWAVE_RF_REGION_IL},
  {"RU", "Russia", ZWAVE_RF_REGION_RU},
  {"CN", "China", ZWAVE_RF_REGION_CN},
  {"JP", "Japan", ZWAVE_RF_REGION_JP},
  {"KR", "Korea", ZWAVE_RF_REGION_KR},
};

static const size_t zwave_rf_region_config_table_count = sizeof(zwave_rf_region_config_table) / sizeof(zwave_rf_region_config_table[0]);

sl_status_t zwave_rf_region_config_from_string(const char *region_string, zwave_rf_region_t *out_region)
{
    char normalized[ZWAVE_RF_REGION_CONFIG_NAME_MAX];

    if (region_string == NULL || out_region == NULL) {
        return SL_STATUS_FAIL;
    }

    size_t len = strnlen(region_string, sizeof(normalized));
    if (len == 0 || len >= sizeof(normalized)) {
        return SL_STATUS_FAIL;
    }

    for (size_t i = 0; i <= len; i++) {
        normalized[i] = (char)toupper((unsigned char)region_string[i]);
    }

    for (size_t i = 0; i < zwave_rf_region_config_table_count; i++) {
        if (strcmp(normalized, zwave_rf_region_config_table[i].config_name) == 0) {
            *out_region = zwave_rf_region_config_table[i].region;
            return SL_STATUS_OK;
        }
    }

    return SL_STATUS_FAIL;
}

const char *zwave_rf_region_config_default_name(void)
{
    return zwave_rf_region_config_table[0].config_name;
}

int zwave_rf_region_config_format_valid_list(char *buffer, size_t buffer_length)
{
    if (buffer == NULL || buffer_length == 0) {
        return -1;
    }

    size_t offset = 0;
    buffer[0]     = '\0';

    for (size_t i = 0; i < zwave_rf_region_config_table_count; i++) {
        const char *name = zwave_rf_region_config_table[i].config_name;
        int written;
        if (i == 0) {
            written = snprintf(buffer + offset, buffer_length - offset, "%s", name);
        } else {
            written = snprintf(buffer + offset, buffer_length - offset, ", %s", name);
        }
        if (written < 0 || (size_t)written >= buffer_length - offset) {
            return -1;
        }
        offset += (size_t)written;
    }

    return (int)offset;
}

int zwave_rf_region_config_format_description(char *buffer, size_t buffer_length)
{
    if (buffer == NULL || buffer_length == 0) {
        return -1;
    }

    int written = snprintf(buffer, buffer_length, "Z-Wave RF region setting");
    if (written < 0 || (size_t)written >= buffer_length) {
        return -1;
    }

    size_t offset = (size_t)written;

    for (size_t i = 0; i < zwave_rf_region_config_table_count; i++) {
        const zwave_rf_region_config_entry_t *entry = &zwave_rf_region_config_table[i];
        written                                     = snprintf(buffer + offset, buffer_length - offset, "\n  %s: %s", entry->config_name, entry->description);
        if (written < 0 || (size_t)written >= buffer_length - offset) {
            return -1;
        }
        offset += (size_t)written;
    }

    return (int)offset;
}
