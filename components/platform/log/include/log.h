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
 * @defgroup sl_log ZPC logging system
 * @ingroup zpc_components
 * @brief Logging library for ZPC applications and components.
 *
 * Logging library for ZPC applications and components. The
 * logging system features log scoping and filtering. All ZPC components
 * should use the logging system for printing messages.
 *
 * @{
 */

#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <sl_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log levels
 */
typedef enum sl_log_level { SL_LOG_DEBUG, SL_LOG_INFO, SL_LOG_WARNING, SL_LOG_ERROR, SL_LOG_CRITICAL } sl_log_level_t;

// TODO: move array to string(hex) to utility function instead
/** log a byte array as hex string. */
#define sl_log_byte_arr(tag, lvl, _arr, _arr_len)                                                                                                     \
    {                                                                                                                                                 \
        char _tmp_log_arr[(_arr_len * 2) + 1] = {0};                                                                                                  \
        char *_cur_log_arr_idx                = _tmp_log_arr;                                                                                         \
        for (unsigned int _log_arr_count = 0; _log_arr_count < _arr_len; _log_arr_count++) {                                                          \
            _cur_log_arr_idx += snprintf(_cur_log_arr_idx, sizeof(_tmp_log_arr) - (_cur_log_arr_idx - _tmp_log_arr), "%02X", (_arr)[_log_arr_count]); \
        }                                                                                                                                             \
        sl_log(tag, lvl, "%s\n", _tmp_log_arr);                                                                                                       \
    }

/**
 * @brief Set log level.
 *
 * @param level log level
 */
void sl_log_set_level(sl_log_level_t level);

/**
 * @brief Get log level.
 *
 * @return log level
 */
sl_log_level_t sl_log_get_level();

/**
 * @brief Set log level for a given tag.
 *
 * This level will override the log level set in \ref sl_log_set_level
 * To remove a tag specific log level use \ref sl_log_unset_tag_level
 *
 * @param tag tag to set log level for
 * @param level log level to set for the tag
 */
void sl_log_set_tag_level(const char *tag, sl_log_level_t level);

/**
 * @brief Remove tag specific log level for tag.
 *
 * By removing the tag specific log level,
 * the log level for the tag will use the log level set by \ref sl_log_set_level
 *
 * @param tag tag to unset specific log level for
 */
void sl_log_unset_tag_level(const char *tag);

/**
 * @brief Convert sl_log_level as string to sl_log_level_t.
 *
 * @param level string representation of sl_log_level_t, supported values are:
 *              "d", "debug"
 *              "i", "info"
 *              "w", "warning"
 *              "e", "error"
 *              "c", "critical"
 * @param result
 * @return sl_status_t
 */
sl_status_t sl_log_level_from_string(const char *level, sl_log_level_t *result);

/**
 * @brief Apply log configuration from strings (e.g. read by the application from config).
 * Either parameter may be NULL to leave that part unchanged.
 *
 * @param log_level_str  Global log level: "d"/"debug", "i"/"info", "w"/"warning", "e"/"error", "c"/"critical"
 * @param tag_level_str  Optional tag-specific levels, comma-separated "tag:level" pairs, e.g. "zwave:debug,mqtt:info"
 * @return SL_STATUS_OK on success, SL_STATUS_FAIL if any provided level string is invalid
 */
sl_status_t sl_log_apply_config(const char *log_level_str, const char *tag_level_str);

/**
 * @brief Write to the log
 *
 * @param tag Log tag to use
 * @param level Log level
 * @param fmtstr Formatted string (printf style)
 * @param ... arguments for the string
 */
void sl_log(const char *const tag, sl_log_level_t level, const char *fmtstr, ...);

#define COLOR_START "\033[32;1m"
#define COLOR_END   "\033[0m"
// Logging macros for calling sl_log with levels
#define sl_log_debug(tag, fmtstr, ...)    sl_log(tag, SL_LOG_DEBUG, fmtstr, ##__VA_ARGS__)
#define sl_log_info(tag, fmtstr, ...)     sl_log(tag, SL_LOG_INFO, fmtstr, ##__VA_ARGS__)
#define sl_log_warning(tag, fmtstr, ...)  sl_log(tag, SL_LOG_WARNING, fmtstr, ##__VA_ARGS__)
#define sl_log_error(tag, fmtstr, ...)    sl_log(tag, SL_LOG_ERROR, fmtstr, ##__VA_ARGS__)
#define sl_log_critical(tag, fmtstr, ...) sl_log(tag, SL_LOG_CRITICAL, fmtstr, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <functional>
#include <thread>
/**
 * @brief Returns the current thread ID as an unsigned long for use in log
 *        messages.  The value is opaque but stable within a process run and
 *        unique per thread.
 */
inline unsigned long sl_log_thread_id()
{
    return static_cast<unsigned long>(std::hash<std::thread::id> {}(std::this_thread::get_id()));
}
#endif

#endif  // LOG_H
/** @} end log */
