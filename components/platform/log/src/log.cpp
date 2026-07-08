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
#include "log.h"
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <unistd.h>

#include "sl_status.h"

#define LOG_TAG "sl_log"

namespace
{

    void init_log_output();

    // Map storing tag specific log levels
    std::unordered_map<std::string, sl_log_level_t> log_levels;
    // Global log level (default matches config default "i")
    sl_log_level_t log_level = SL_LOG_INFO;
    std::mutex log_mutex;
    bool use_color = false;

    const char *log_level_short[] = {"d", "i", "W", "E", "C"};
    const char *log_level_long[]  = {"debug", "info", "Warning", "Error", "CRITICAL"};
    const char *log_level_color[] = {"\033[34;1m", "\033[32;1m", "\033[33;1m", "\033[31;1m", "\033[37;41;1m"};

    bool should_log(const char *tag, sl_log_level_t level)
    {
        auto it                  = log_levels.find(tag);
        sl_log_level_t threshold = (it != log_levels.end()) ? it->second : log_level;
        return level >= threshold;
    }

    std::string format_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::tm tm_buf;
        localtime_r(&t, &tm_buf);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d", tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, static_cast<int>(ms.count()));
        return buf;
    }

    void write_log(const char *tag, sl_log_level_t level, const std::string &msg)
    {
        if (!should_log(tag, level)) {
            return;
        }
        static bool once = (init_log_output(), true);
        (void)once;
        std::lock_guard<std::mutex> lock(log_mutex);
        std::string ts = format_timestamp();
        if (use_color) {
            std::fprintf(stderr, "%s %s <%s> [%s] %s\033[0m\n", ts.c_str(), log_level_color[level], log_level_short[level], tag, msg.c_str());
        } else {
            std::fprintf(stderr, "%s <%s> [%s] %s\n", ts.c_str(), log_level_short[level], tag, msg.c_str());
        }
    }

    void to_lower(std::string &s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }

    std::string trim(const std::string &s)
    {
        auto start = s.find_first_not_of(" \t");
        if (start == std::string::npos) {
            return "";
        }
        auto end = s.find_last_not_of(" \t");
        return s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
    }

    std::vector<std::string> split(const std::string &s, char delim)
    {
        std::vector<std::string> out;
        size_t pos = 0;
        while (true) {
            size_t next = s.find(delim, pos);
            if (next == std::string::npos) {
                out.push_back(s.substr(pos));
                break;
            }
            out.push_back(s.substr(pos, next - pos));
            pos = next + 1;
        }
        return out;
    }

    void init_log_output()
    {
        use_color = (isatty(fileno(stderr)) != 0);
    }

    bool is_non_empty(const char *s)
    {
        return s != nullptr && s[0] != '\0';
    }

}  // namespace

void sl_log_set_level(sl_log_level_t level)
{
    log_level = level;
    sl_log_debug(LOG_TAG, "Setting log level to %s\n", log_level_long[level]);
}

sl_log_level_t sl_log_get_level()
{
    return log_level;
}

void sl_log_set_tag_level(const char *tag, sl_log_level_t level)
{
    log_levels[tag] = level;
    sl_log_debug(LOG_TAG, "Setting log level for '%s' to %s", tag, log_level_long[level]);
}

void sl_log_unset_tag_level(const char *tag)
{
    log_levels.erase(tag);
    sl_log_debug(LOG_TAG, "Unsetting log level for '%s'\n", tag);
}

sl_status_t sl_log_level_from_string(const char *level, sl_log_level_t *result)
{
    std::string level_lower(level);
    to_lower(level_lower);
    if (level_lower == "d" || level_lower == "debug") {
        *result = SL_LOG_DEBUG;
    } else if (level_lower == "i" || level_lower == "info") {
        *result = SL_LOG_INFO;
    } else if (level_lower == "w" || level_lower == "warning") {
        *result = SL_LOG_WARNING;
    } else if (level_lower == "e" || level_lower == "error") {
        *result = SL_LOG_ERROR;
    } else if (level_lower == "c" || level_lower == "critical") {
        *result = SL_LOG_CRITICAL;
    } else {
        return SL_STATUS_FAIL;
    }
    return SL_STATUS_OK;
}

sl_status_t sl_log_apply_config(const char *log_level_str, const char *tag_level_str)
{
    sl_log_level_t level = SL_LOG_INFO;
    std::vector<std::pair<std::string, sl_log_level_t>> tag_levels_to_apply;

    if (is_non_empty(log_level_str)) {
        if (sl_log_level_from_string(log_level_str, &level) != SL_STATUS_OK) {
            std::fprintf(stderr, "Invalid log.level value: '%s'\n", log_level_str);
            return SL_STATUS_FAIL;
        }
    }
    if (is_non_empty(tag_level_str)) {
        for (const std::string &entry: split(tag_level_str, ',')) {
            const std::string trimmed_entry = trim(entry);
            if (trimmed_entry.empty()) {
                continue;
            }
            std::vector<std::string> parts = split(trimmed_entry, ':');
            if (parts.size() != 2) {
                std::fprintf(stderr, "Invalid log.tag_level entry: '%s'\n", trimmed_entry.c_str());
                return SL_STATUS_FAIL;
            }
            const std::string tag       = trim(parts[0]);
            const std::string lvl_s     = trim(parts[1]);
            sl_log_level_t parsed_level = SL_LOG_INFO;
            if (tag.empty() || lvl_s.empty()) {
                std::fprintf(stderr, "Invalid log.tag_level entry: '%s'\n", trimmed_entry.c_str());
                return SL_STATUS_FAIL;
            }
            if (sl_log_level_from_string(lvl_s.c_str(), &parsed_level) != SL_STATUS_OK) {
                std::fprintf(stderr, "Invalid log.tag_level entry: '%s'\n", trimmed_entry.c_str());
                return SL_STATUS_FAIL;
            }
            tag_levels_to_apply.emplace_back(tag, parsed_level);
        }
    }

    if (is_non_empty(log_level_str)) {
        sl_log_set_level(level);
    }
    for (const auto &tag_level: tag_levels_to_apply) {
        sl_log_set_tag_level(tag_level.first.c_str(), tag_level.second);
    }
    return SL_STATUS_OK;
}

void sl_log(const char *const tag, sl_log_level_t level, const char *fmtstr, ...)
{
    va_list myargs;
    va_start(myargs, fmtstr);
    size_t size = static_cast<size_t>(std::vsnprintf(nullptr, 0, fmtstr, myargs)) + 1;
    va_end(myargs);
    std::string mystr(size - 1, '\0');
    va_start(myargs, fmtstr);
    std::vsnprintf(mystr.data(), size, fmtstr, myargs);
    va_end(myargs);
    if (!mystr.empty() && mystr.back() == '\n') {
        mystr.pop_back();
    }
    write_log(tag, level, mystr);
}
