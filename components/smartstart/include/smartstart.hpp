/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 *****************************************************************************/

#ifndef SMARTSTART_HPP
#define SMARTSTART_HPP

#include "sl_status.h"
#include "zwave_controller_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool find_dsk_obfuscated_bytes_from_smart_start_list(zwave_dsk_t dsk, uint8_t obfuscated_bytes);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "log.h"

namespace smartstart
{
    using notification_function_t = std::function<void(bool)>;

    class Entry
    {
        public:
            std::string dsk;
            std::vector<std::string> preferred_protocols;
            Entry() = default;
            Entry(const std::string &dsk) : dsk {dsk}, preferred_protocols {} {}
    };

    class Management
    {
            std::unordered_map<std::string, Entry> _smartstart_cache;
            mutable std::recursive_mutex _cache_mutex;
            notification_function_t _notify_has_entries_awaiting_inclusion;
            static Management *_instance;
            Management() = default;

            // Parse a SmartStart list payload (`{ "value": [ ... ] }`) into `parsed`.
            // Sets `has_entries_awaiting_inclusion` to true if any parsed entry would
            // be eligible for automatic inclusion. Returns SL_STATUS_OK only when the
            // entire payload parses cleanly (no partial state on failure).
            [[nodiscard]] static sl_status_t parse_smartstart_list_payload(const std::string &smartstart_list, std::unordered_map<std::string, Entry> &parsed, bool &has_entries_awaiting_inclusion);

        public:
            Management(const Management &)            = delete;
            Management &operator=(const Management &) = delete;
            [[nodiscard]] static Management *get_instance();
            sl_status_t init(notification_function_t const &f);
            static sl_status_t teardown();
            sl_status_t update_smartstart_cache(const std::string &smartstart_list);
            // Merge entries from `smartstart_list` into the existing cache. DSKs that
            // are already in the cache are left untouched (Add is purely additive).
            sl_status_t add_to_smartstart_cache(const std::string &smartstart_list);
            // Remove the entries identified by the DSKs in `smartstart_remove_list`.
            // Payload shape matches Update / Add (`{ "value": [ { "DSK": "..." }, ... ] }`);
            // only the DSK field is read, other fields are ignored. DSKs that are not
            // in the cache are silently skipped.
            sl_status_t remove_from_smartstart_cache(const std::string &smartstart_remove_list);
            // Purge the entire SmartStart provisioning list.
            sl_status_t clear_smartstart_cache();
            [[nodiscard]] bool has_entries_awaiting_inclusion() const;
            // Remove a DSK after a *successful* inclusion. On security failure the
            // DSK is retained (see zwave_smartstart_management_on_node_added).
            sl_status_t notify_node_added(const std::string &dsk);
            [[nodiscard]] std::unordered_map<std::string, Entry> get_cache() const;
    };

}  // namespace smartstart

#include "init_builder.hpp"
#include "safe_queue.hpp"
#include "threading.hpp"
#include "smartstart_mqtt_api.hpp"

namespace zwave_component
{
    class smartstart_handler : public threading::threading, public Initializable
    {
        public:
            smartstart_handler();
            ~smartstart_handler();
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        private:
            enum class smartstart_event_t { LIST_UPDATE_EVENT, LIST_REQUEST_EVENT, LIST_ADD_EVENT, LIST_REMOVE_EVENT, LIST_CLEAR_EVENT };
            struct smartstart_event_data {
                    smartstart_event_t event;
                    std::string data;
            };
            ::threading::safe_queue<smartstart_event_data> event_queue;
            zwave_command_class::SmartStartMqttApi smartstart_mqtt_api_instance;
            void run() override;
            static smartstart_handler *instance;
    };
}  // namespace zwave_component

#endif  // __cplusplus
#endif  // SMARTSTART_HPP
