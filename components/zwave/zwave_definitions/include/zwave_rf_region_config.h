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
 * @defgroup zwave_rf_region_config ZPC RF region configuration mapping
 * @ingroup zwave_definitions
 * @brief Maps ZPC config strings (zpc.rf_region) to @ref zwave_rf_region_t values.
 *
 * Supported region names are defined only in zwave_rf_region_config.c.
 * @{
 */

#if !defined(ZWAVE_RF_REGION_CONFIG_H)
#define ZWAVE_RF_REGION_CONFIG_H

#include <stddef.h>

#include "sl_status.h"
#include "zwave_rf_region.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a ZPC RF region config string.
 *
 * @param region_string Value from zpc.rf_region (YAML or CLI). Matching is case-insensitive.
 * @param out_region Parsed region on success.
 * @return SL_STATUS_OK if recognized, SL_STATUS_FAIL otherwise.
 */
sl_status_t zwave_rf_region_config_from_string(const char *region_string, zwave_rf_region_t *out_region);

/**
 * @brief Default zpc.rf_region config string (first table entry).
 */
const char *zwave_rf_region_config_default_name(void);

/**
 * @brief Write a comma-separated list of valid region names into @p buffer.
 *
 * @return Number of characters written (excluding null terminator), or -1 if truncated.
 */
int zwave_rf_region_config_format_valid_list(char *buffer, size_t buffer_length);

/**
 * @brief Write multiline help text for config_add_string into @p buffer.
 *
 * @return Number of characters written (excluding null terminator), or -1 if truncated.
 */
int zwave_rf_region_config_format_description(char *buffer, size_t buffer_length);

#ifdef __cplusplus
}
#endif

#endif  // ZWAVE_RF_REGION_CONFIG_H
/** @} end zwave_rf_region_config */
