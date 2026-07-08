/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 *****************************************************************************/

#include "smartstart.hpp"
#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_defined_attribute_types.h"
#include "mqtt_handler.hpp"
#include "timer.hpp"
#include "zpc_attribute_store.h"
#include "zpc_config.h"
#include "utils.hpp"
#include "s2_keystore.h"
#include "zwave_controller.h"
#include "zwave_controller_callbacks.h"
#include "zwave_network_management.h"
#include "zwave_utils.h"
#include "network_monitor_network_status.h"

#include <fmt/format.h>

#include <arpa/inet.h>
#include <map>
#include <sstream>
#include <nlohmann/json.hpp>
#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

using namespace smartstart;

namespace
{
    constexpr char LOG_TAG[] = "smartstart";

    std::string make_zpc_node_address_key(zwave_home_id_t home_id, zwave_node_id_t node_id)
    {
        return fmt::format("{:08X}-{:04X}", home_id, node_id);
    }

    constexpr char ZWAVE_LONG_RANGE_STRING_REPRESENTATION[]                           = "Z-Wave Long Range";
    constexpr char ZWAVE_STRING_REPRESENTATION[]                                      = "Z-Wave";
    const std::map<std::string, zwave_protocol_t, std::less<>> preferred_protocol_map = {{std::string(ZWAVE_CONFIG_REPRESENTATION), PROTOCOL_ZWAVE}, {std::string(ZWAVE_LONG_RANGE_CONFIG_REPRESENTATION), PROTOCOL_ZWAVE_LONG_RANGE}};
    const int PROTOCOL_DISCOVERY_COOLDOWN                                             = 4;

    using node_protocol_capabilities_t = struct {
            bool zwave;
            bool zwave_long_range;
    };

    enum class protocol_discover_state_t { DISCOVERED_ZERO, DISCOVERED_ONE, DISCOVERED_ALL };

    class ProtocolDiscoveryEntry
    {
        public:
            struct timer_handle_t timer {nullptr};
            node_protocol_capabilities_t capabilities;

            void add_capability(zwave_protocol_t protocol)
            {
                switch (protocol) {
                    case PROTOCOL_ZWAVE_LONG_RANGE:
                        capabilities.zwave_long_range = true;
                        break;
                    case PROTOCOL_ZWAVE:
                        capabilities.zwave = true;
                        break;
                    default:
                        break;
                }
            }

            bool has_capability(zwave_protocol_t protocol) const
            {
                switch (protocol) {
                    case PROTOCOL_ZWAVE_LONG_RANGE:
                        return capabilities.zwave_long_range;
                    case PROTOCOL_ZWAVE:
                        return capabilities.zwave;
                    default:
                        return false;
                }
            }

            ~ProtocolDiscoveryEntry()
            {
                timer_stop(&timer);
            }
    };

    std::mutex protocol_discovery_db_mutex;
    std::unordered_map<std::string, std::shared_ptr<ProtocolDiscoveryEntry>> protocol_discovery_db;

    bool is_homeid_in_dsk(zwave_dsk_t dsk, zwave_home_id_t home_id)
    {
        const zwave_home_id_t be_home_id = htonl(home_id);
        std::array<uint8_t, 4> buf;
        std::memcpy(buf.data(), &dsk[8], buf.size());
        buf[0] |= 0xC0;
        buf[3] &= 0xFE;
        return std::memcmp(buf.data(), &be_home_id, buf.size()) == 0;
    }

    std::vector<zwave_protocol_t> get_preferred_protocol_list_from_config()
    {
        std::vector<zwave_protocol_t> protocols;
        try {
            std::string protocols_string(zpc_get_config()->inclusion_protocol_preference);
            std::istringstream iss(protocols_string);
            for (std::string element; std::getline(iss, element, ',');) {
                if (preferred_protocol_map.contains(element)) {
                    protocols.push_back(preferred_protocol_map.at(element));
                }
            }
        } catch (const std::exception &exc) {
            sl_log_error(LOG_TAG, "Cannot parse configuration preferred protocols. Exception: %s", exc.what());
        }
        return protocols;
    }

    zwave_protocol_t get_preferred_inclusion_protocol_from_config(node_protocol_capabilities_t capabilities)
    {
        auto protocol_list = get_preferred_protocol_list_from_config();
        for (auto protocol: protocol_list) {
            if (protocol == PROTOCOL_ZWAVE && capabilities.zwave) {
                return protocol;
            }
            if (protocol == PROTOCOL_ZWAVE_LONG_RANGE && capabilities.zwave_long_range) {
                return protocol;
            }
        }
        return PROTOCOL_UNKNOWN;
    }

    zwave_protocol_t get_preferred_inclusion_protocol_from_list(const std::vector<std::string> &preferred_protocols, node_protocol_capabilities_t capabilities)
    {
        for (const auto &proto_str: preferred_protocols) {
            if (proto_str == ZWAVE_LONG_RANGE_STRING_REPRESENTATION && capabilities.zwave_long_range) {
                return PROTOCOL_ZWAVE_LONG_RANGE;
            }
            if (proto_str == ZWAVE_STRING_REPRESENTATION && capabilities.zwave) {
                return PROTOCOL_ZWAVE;
            }
        }
        return PROTOCOL_UNKNOWN;
    }

    bool has_several_protocol_candidates(const std::vector<std::string> &preferred_protocols)
    {
        if (preferred_protocols.empty()) {
            return get_preferred_protocol_list_from_config().size() > 1;
        }
        return preferred_protocols.size() > 1;
    }

    protocol_discover_state_t get_protocol_discovery_state(const std::string &dsk)
    {
        auto it = protocol_discovery_db.find(dsk);
        if (it == protocol_discovery_db.end()) {
            return protocol_discover_state_t::DISCOVERED_ZERO;
        }
        const auto *entry = it->second.get();
        if (entry->has_capability(PROTOCOL_ZWAVE_LONG_RANGE) && entry->has_capability(PROTOCOL_ZWAVE)) {
            return protocol_discover_state_t::DISCOVERED_ALL;
        }
        if (entry->has_capability(PROTOCOL_ZWAVE_LONG_RANGE) || entry->has_capability(PROTOCOL_ZWAVE)) {
            return protocol_discover_state_t::DISCOVERED_ONE;
        }
        return protocol_discover_state_t::DISCOVERED_ZERO;
    }

    bool is_dsk_already_included_in_current_network(const zwave_dsk_t &dsk_internal)
    {
        using namespace attribute_store;
        attribute home_id_node = get_zpc_network_node();
        attribute dsk_node;
        for (auto node_id_node: home_id_node.children(ATTRIBUTE_NODE_ID)) {
            dsk_node = node_id_node.child_by_type(ATTRIBUTE_S2_DSK, 0);
            if (!dsk_node.is_valid()) {
                continue;
            }
            std::vector<uint8_t> dsk_data;
            try {
                dsk_data = dsk_node.reported<std::vector<uint8_t>>();
            } catch (const std::invalid_argument &e) {
                sl_log_warning(LOG_TAG, "Failed to read DSK: %s", e.what());
                continue;
            }
            if (memcmp(dsk_data.data(), &dsk_internal[0], sizeof(dsk_internal)) == 0) {
                // Skip failed-inclusion ghosts (kex_fail != NONE). Same success
                // gate as zwave_smartstart_management_on_node_added — otherwise a
                // leftover NodeID after 6404 / self-destruct would strip the DSK
                // from the provisioning cache and block retry.
                attribute kex_fail_node = node_id_node.child_by_type(ATTRIBUTE_KEX_FAIL_TYPE, 0);
                if (kex_fail_node.is_valid()) {
                    try {
                        const auto kex_fail = static_cast<zwave_kex_fail_type_t>(kex_fail_node.reported<uint32_t>());
                        if (kex_fail != ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE) {
                            continue;
                        }
                    } catch (const std::invalid_argument &e) {
                        sl_log_warning(LOG_TAG, "Failed to read KEX fail type: %s", e.what());
                        continue;
                    }
                }

                zwave_node_id_t node_id_data = node_id_node.reported<uint16_t>();
                std::string const node_key   = make_zpc_node_address_key(zwave_network_management_get_home_id(), node_id_data);
                char dsk_str[DSK_STR_LEN];
                Utils::convert_dsk_to_dsk_str(dsk_internal, dsk_str, sizeof(dsk_str));
                sl_log_info(LOG_TAG, "DSK %s in provisioning list is already in network (node: %s).", dsk_str, node_key.c_str());
                Management::get_instance()->notify_node_added(std::string(dsk_str));
                return true;
            }
        }
        return false;
    }

    bool does_one_entry_need_inclusion()
    {
        bool has_dsk_awaiting_inclusion = false;
        zwave_dsk_t dsk_internal        = {0};
        const auto cache                = Management::get_instance()->get_cache();
        for (const auto &[_, value]: cache) {
            if (SL_STATUS_OK == Utils::convert_dsk_str_to_dsk(value.dsk.c_str(), dsk_internal)) {
                if (!is_dsk_already_included_in_current_network(dsk_internal)) {
                    has_dsk_awaiting_inclusion = true;
                }
            }
        }
        return has_dsk_awaiting_inclusion;
    }

    void zwave_smartstart_management_on_inclusion_request(zwave_home_id_t home_id, bool already_included, const zwave_node_info_t *node_info, zwave_protocol_t protocol)
    {
        const auto cache         = Management::get_instance()->get_cache();
        const Entry *matched     = nullptr;
        zwave_dsk_t dsk_internal = {0};
        for (const auto &pair: cache) {
            if (SL_STATUS_OK == Utils::convert_dsk_str_to_dsk(pair.second.dsk.c_str(), dsk_internal) && is_homeid_in_dsk(dsk_internal, home_id)) {
                matched = &pair.second;
                break;
            }
        }
        if (matched == nullptr) {
            sl_log_debug(LOG_TAG, "No match in SmartStart list for NWI HomeID %X", home_id);
            return;
        }
        if (already_included) {
            sl_log_info(LOG_TAG, "Received INIF from NWI HomeID %X.", home_id);
            return;
        }

        std::lock_guard<std::mutex> protocol_discovery_lock(protocol_discovery_db_mutex);

        if (has_several_protocol_candidates(matched->preferred_protocols)) {
            protocol_discover_state_t state = get_protocol_discovery_state(matched->dsk);
            switch (state) {
                case protocol_discover_state_t::DISCOVERED_ZERO: {
                    auto entry = std::make_shared<ProtocolDiscoveryEntry>();
                    entry->add_capability(protocol);
                    timer_set(&entry->timer, PROTOCOL_DISCOVERY_COOLDOWN * TIMER_SECOND, nullptr, 0);
                    protocol_discovery_db[matched->dsk] = entry;
                    sl_log_debug(LOG_TAG, "Discovered %s for NWI HomeID %X", zwave_get_protocol_name(protocol), home_id);
                    return;
                }
                case protocol_discover_state_t::DISCOVERED_ONE: {
                    if (!protocol_discovery_db[matched->dsk]->has_capability(protocol)) {
                        timer_restart(&protocol_discovery_db[matched->dsk]->timer);
                        protocol_discovery_db[matched->dsk]->add_capability(protocol);
                        sl_log_debug(LOG_TAG, "Discovered %s for NWI HomeID %X", zwave_get_protocol_name(protocol), home_id);
                        if (get_protocol_discovery_state(matched->dsk) == protocol_discover_state_t::DISCOVERED_ALL) {
                            goto done_protocol_discovery;
                        }
                        return;
                    }
                    if (timer_expired(&protocol_discovery_db[matched->dsk]->timer)) {
                        goto done_protocol_discovery;
                    }
                    sl_log_debug(LOG_TAG, "Re-discovered %s for NWI HomeID %X too early, ignoring.", zwave_get_protocol_name(protocol), home_id);
                    return;
                }
                case protocol_discover_state_t::DISCOVERED_ALL:
                    goto done_protocol_discovery;
                default:
                    sl_log_warning(LOG_TAG, "Invalid protocol discovery state");
                    return;
            }
done_protocol_discovery:
            sl_log_debug(LOG_TAG, "Protocol discovery for NWI Home ID %X done.", home_id);
        }

        zwave_protocol_t preferred_protocol = PROTOCOL_UNKNOWN;
        if (has_several_protocol_candidates(matched->preferred_protocols)) {
            node_protocol_capabilities_t node_capabilities = protocol_discovery_db[matched->dsk]->capabilities;
            if (matched->preferred_protocols.empty()) {
                preferred_protocol = get_preferred_inclusion_protocol_from_config(node_capabilities);
            } else {
                preferred_protocol = get_preferred_inclusion_protocol_from_list(matched->preferred_protocols, node_capabilities);
            }
        } else {
            if (matched->preferred_protocols.empty()) {
                preferred_protocol = get_preferred_protocol_list_from_config().at(0);
            } else {
                if (matched->preferred_protocols.at(0) == ZWAVE_LONG_RANGE_STRING_REPRESENTATION) {
                    preferred_protocol = PROTOCOL_ZWAVE_LONG_RANGE;
                } else if (matched->preferred_protocols.at(0) == ZWAVE_STRING_REPRESENTATION) {
                    preferred_protocol = PROTOCOL_ZWAVE;
                }
            }
        }
        if (preferred_protocol == PROTOCOL_UNKNOWN) {
            sl_log_info(LOG_TAG, "Preferred protocol not supported by NWI HomeID %X.", home_id);
            return;
        }
        if (protocol != preferred_protocol) {
            sl_log_debug(LOG_TAG, "SmartStart Prime on different protocol (%s) than preferred (%s), ignoring.", zwave_get_protocol_name(protocol), zwave_get_protocol_name(preferred_protocol));
            return;
        }
        // Defer secure-add while NM is busy, protocol commissioning is in progress,
        // or any end device is still being interviewed (avoids S2/TX contention).
        if (zwave_network_management_is_busy()) {
            sl_log_warning(LOG_TAG, "Ignoring SmartStart prime for NWI HomeID %X — network management busy", home_id);
            return;
        }
        if (network_monitor_is_end_device_inclusion_ongoing()) {
            sl_log_warning(LOG_TAG, "Ignoring SmartStart prime for NWI HomeID %X — end device inclusion still ongoing", home_id);
            return;
        }
        if (network_monitor_is_any_end_device_interviewing()) {
            sl_log_warning(LOG_TAG, "Ignoring SmartStart prime for NWI HomeID %X — end device interview in progress", home_id);
            return;
        }
        zwave_network_management_add_node_with_dsk(dsk_internal, KEY_CLASS_ALL, preferred_protocol);
    }

    void zwave_smartstart_management_on_node_added(sl_status_t status, const zwave_node_info_t *nif, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol)
    {
        (void)nif;
        (void)node_id;
        (void)granted_keys;
        (void)inclusion_protocol;

        char dsk_str[DSK_STR_LEN];
        Utils::convert_dsk_to_dsk_str(dsk, dsk_str, sizeof(dsk_str));

        // Always clear protocol-discovery state so a later NIF can rediscover.
        {
            std::lock_guard<std::mutex> lock(protocol_discovery_db_mutex);
            protocol_discovery_db.erase(std::string(dsk_str));
        }

        const bool success = (status == SL_STATUS_OK) && (kex_fail_type == ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE);
        if (success) {
            Management::get_instance()->notify_node_added(std::string(dsk_str));
            return;
        }

        // Keep the DSK so a later NIF can retry. Add mode was never cleared;
        // returning to NM_IDLE re-enables SmartStart listening.
        sl_log_info(LOG_TAG,
                    "Keeping SmartStart cache entry for DSK %s after security failure "
                    "(status=%d, kex_fail=%d).",
                    dsk_str,
                    static_cast<int>(status),
                    static_cast<int>(kex_fail_type));
    }

    zwave_controller_callbacks_t smartstart_callbacks = {
      .on_node_added                    = zwave_smartstart_management_on_node_added,
      .on_smart_start_inclusion_request = zwave_smartstart_management_on_inclusion_request,
    };

    void has_entries_awaiting_inclusion(bool value)
    {
        if (value) {
            bool activate_smart_start = true;
            if (!zwave_controller_is_reset_ongoing()) {
                activate_smart_start = does_one_entry_need_inclusion();
            }
            zwave_network_management_enable_smart_start_add_mode(activate_smart_start);
        } else {
            zwave_network_management_enable_smart_start_add_mode(false);
        }
    }
}  // namespace

namespace smartstart
{
    Management *Management::_instance;

    std::unordered_map<std::string, Entry> Management::get_cache() const
    {
        std::lock_guard<std::recursive_mutex> lock(_cache_mutex);
        return _smartstart_cache;
    }

    bool Management::has_entries_awaiting_inclusion() const
    {
        std::lock_guard<std::recursive_mutex> lock(_cache_mutex);
        return !_smartstart_cache.empty();
    }

    sl_status_t Management::notify_node_added(const std::string &dsk)
    {
        std::lock_guard<std::recursive_mutex> lock(_cache_mutex);
        auto it = _smartstart_cache.find(dsk);
        if (it == _smartstart_cache.end()) {
            sl_log_info(LOG_TAG, "Newly added node DSK (%s) not in SmartStart list.", dsk.c_str());
            return SL_STATUS_NOT_FOUND;
        }
        sl_log_debug(LOG_TAG, "Removing SmartStart cache entry for DSK %s after successful inclusion.", dsk.c_str());
        _smartstart_cache.erase(it);
        return SL_STATUS_OK;
    }

    sl_status_t Management::parse_smartstart_list_payload(const std::string &smartstart_list, std::unordered_map<std::string, Entry> &parsed, bool &has_entries_awaiting_inclusion)
    {
        try {
            nlohmann::json jsn = nlohmann::json::parse(smartstart_list);
            if (!jsn.contains("value")) {
                sl_log_warning(LOG_TAG, "No value for SmartStart list.");
                return SL_STATUS_FAIL;
            }
            nlohmann::json value = jsn["value"];
            for (auto &element: value) {
                try {
                    std::vector<std::string> preferred_protocols;
                    Entry entry = {element["DSK"]};
                    if (!element["PreferredProtocols"].is_null()) {
                        for (auto &prot: element["PreferredProtocols"]) {
                            preferred_protocols.push_back(prot.front());
                        }
                        entry.preferred_protocols = preferred_protocols;
                    }
                    parsed.try_emplace(entry.dsk, entry);
                    has_entries_awaiting_inclusion = true;
                } catch (std::exception &err) {
                    sl_log_warning(LOG_TAG, "Failed to parse SmartStart entry: %s", err.what());
                    return SL_STATUS_FAIL;
                }
            }
        } catch (std::exception &err) {
            sl_log_warning(LOG_TAG, "Failed to parse SmartStart list: %s", err.what());
            return SL_STATUS_FAIL;
        }
        return SL_STATUS_OK;
    }

    sl_status_t Management::update_smartstart_cache(const std::string &smartstart_list)
    {
        std::unordered_map<std::string, Entry> parsed;
        bool has_entries_awaiting_inclusion = false;
        sl_status_t status                  = parse_smartstart_list_payload(smartstart_list, parsed, has_entries_awaiting_inclusion);
        if (status != SL_STATUS_OK) {
            return status;
        }
        std::lock_guard<std::recursive_mutex> lock(_cache_mutex);
        _smartstart_cache = std::move(parsed);
        if (_notify_has_entries_awaiting_inclusion) {
            _notify_has_entries_awaiting_inclusion(has_entries_awaiting_inclusion);
        }
        return SL_STATUS_OK;
    }

    sl_status_t Management::add_to_smartstart_cache(const std::string &smartstart_list)
    {
        std::unordered_map<std::string, Entry> parsed;
        bool has_entries_awaiting_inclusion_in_payload = false;
        sl_status_t status                             = parse_smartstart_list_payload(smartstart_list, parsed, has_entries_awaiting_inclusion_in_payload);
        if (status != SL_STATUS_OK) {
            return status;
        }
        std::lock_guard<std::recursive_mutex> lock(_cache_mutex);
        bool added_any = false;
        for (auto &[dsk, entry]: parsed) {
            auto [it, inserted] = _smartstart_cache.try_emplace(dsk, std::move(entry));
            (void)it;
            if (inserted) {
                added_any = true;
            } else {
                sl_log_debug(LOG_TAG, "DSK %s already in SmartStart list, skipping.", dsk.c_str());
            }
        }
        // Re-evaluate from the full cache so we pick up entries that were already
        // awaiting inclusion before the add.
        if (added_any && _notify_has_entries_awaiting_inclusion) {
            _notify_has_entries_awaiting_inclusion(has_entries_awaiting_inclusion());
        }
        return SL_STATUS_OK;
    }

    sl_status_t Management::remove_from_smartstart_cache(const std::string &smartstart_remove_list)
    {
        // Parse the full payload up-front so a malformed entry doesn't leave the
        // cache half-mutated. The payload shape matches Update / Add ({"value": [{...}]});
        // only the DSK field is read, other fields are tolerated and ignored.
        std::vector<std::string> dsks_to_remove;
        try {
            nlohmann::json jsn = nlohmann::json::parse(smartstart_remove_list);
            if (!jsn.contains("value")) {
                sl_log_warning(LOG_TAG, "No value for SmartStart remove list.");
                return SL_STATUS_FAIL;
            }
            nlohmann::json value = jsn["value"];
            for (auto &element: value) {
                try {
                    dsks_to_remove.push_back(element.at("DSK").get<std::string>());
                } catch (std::exception &err) {
                    sl_log_warning(LOG_TAG, "Failed to parse DSK in SmartStart remove list: %s", err.what());
                    return SL_STATUS_FAIL;
                }
            }
        } catch (std::exception &err) {
            sl_log_warning(LOG_TAG, "Failed to parse SmartStart remove list: %s", err.what());
            return SL_STATUS_FAIL;
        }

        std::lock_guard<std::recursive_mutex> lock(_cache_mutex);
        bool removed_any = false;
        for (const auto &dsk: dsks_to_remove) {
            if (_smartstart_cache.erase(dsk) > 0) {
                sl_log_debug(LOG_TAG, "Removed SmartStart entry for DSK %s.", dsk.c_str());
                removed_any = true;
            } else {
                sl_log_debug(LOG_TAG, "DSK %s not in SmartStart list, skipping remove.", dsk.c_str());
            }
        }
        if (removed_any && _notify_has_entries_awaiting_inclusion) {
            _notify_has_entries_awaiting_inclusion(has_entries_awaiting_inclusion());
        }
        return SL_STATUS_OK;
    }

    sl_status_t Management::clear_smartstart_cache()
    {
        std::lock_guard<std::recursive_mutex> lock(_cache_mutex);
        sl_log_debug(LOG_TAG, "Clearing SmartStart provisioning list (%zu entries).", _smartstart_cache.size());
        _smartstart_cache.clear();
        if (_notify_has_entries_awaiting_inclusion) {
            _notify_has_entries_awaiting_inclusion(false);
        }
        return SL_STATUS_OK;
    }

    Management *Management::get_instance()
    {
        if (_instance == nullptr) {
            _instance = new Management();
        }
        return _instance;
    }

    sl_status_t Management::init(notification_function_t const &f)
    {
        if (f) {
            _notify_has_entries_awaiting_inclusion = f;
            return SL_STATUS_OK;
        }
        return SL_STATUS_FAIL;
    }

    sl_status_t Management::teardown()
    {
        return SL_STATUS_OK;
    }
}  // namespace smartstart

namespace zwave_component
{
    smartstart_handler *smartstart_handler::instance = nullptr;

    smartstart_handler::smartstart_handler() : threading("SmartStart Handler")
    {
        instance = this;
    }

    void smartstart_handler::run()
    {
        std::optional<smartstart_event_data> ev = event_queue.pop(10);
        if (ev.has_value()) {
            switch (ev.value().event) {
                case smartstart_event_t::LIST_UPDATE_EVENT:
                    Management::get_instance()->update_smartstart_cache(ev.value().data);
                    break;
                case smartstart_event_t::LIST_ADD_EVENT:
                    Management::get_instance()->add_to_smartstart_cache(ev.value().data);
                    break;
                case smartstart_event_t::LIST_REMOVE_EVENT:
                    Management::get_instance()->remove_from_smartstart_cache(ev.value().data);
                    break;
                case smartstart_event_t::LIST_CLEAR_EVENT:
                    Management::get_instance()->clear_smartstart_cache();
                    break;
                case smartstart_event_t::LIST_REQUEST_EVENT: {
                    nlohmann::json result;
                    result["value"] = nlohmann::json::array();
                    for (const auto &[_, entry]: Management::get_instance()->get_cache()) {
                        nlohmann::json item;
                        item["DSK"]                = entry.dsk;
                        item["PreferredProtocols"] = entry.preferred_protocols;
                        result["value"].push_back(std::move(item));
                    }
                    zwave_command_class::SmartStartMqttApi::publish_smartstart_list(result.dump());
                    break;
                }
                default:
                    sl_log_warning(LOG_TAG, "Unhandled event %d", static_cast<int>(ev.value().event));
                    break;
            }
        }

        if (should_stop()) {
            return;
        }
    }

    smartstart_handler::~smartstart_handler()
    {
        if (instance == this) {
            instance = nullptr;
        }
    }

    sl_status_t smartstart_handler::initialize()
    {
        zwave_controller_register_callbacks(&smartstart_callbacks);
        Management::get_instance()->init(has_entries_awaiting_inclusion);
        // Set callback to route MQTT API updates through the thread-safe event queue
        smartstart_mqtt_api_instance.set_cache_update_callback([this](const std::string &message) { event_queue.push({smartstart_event_t::LIST_UPDATE_EVENT, message}); });
        smartstart_mqtt_api_instance.set_cache_add_callback([this](const std::string &message) { event_queue.push({smartstart_event_t::LIST_ADD_EVENT, message}); });
        smartstart_mqtt_api_instance.set_cache_remove_callback([this](const std::string &message) { event_queue.push({smartstart_event_t::LIST_REMOVE_EVENT, message}); });
        smartstart_mqtt_api_instance.set_cache_clear_callback([this]() { event_queue.push({smartstart_event_t::LIST_CLEAR_EVENT, ""}); });
        smartstart_mqtt_api_instance.set_list_request_callback([this]() { event_queue.push({smartstart_event_t::LIST_REQUEST_EVENT, ""}); });
        smartstart_mqtt_api_instance.setup_mqtt_api();
        return SL_STATUS_OK;
    }

    int smartstart_handler::shutdown()
    {
        stop();
        return 0;
    }

    std::string smartstart_handler::name() const
    {
        return "SmartStart Handler";
    }
}  // namespace zwave_component

extern "C" {

bool find_dsk_obfuscated_bytes_from_smart_start_list(zwave_dsk_t dsk, uint8_t obfuscated_bytes)
{
    zwave_dsk_t dsk_internal = {0};
    for (const auto &[key, value]: Management::get_instance()->get_cache()) {
        if (SL_STATUS_OK == Utils::convert_dsk_str_to_dsk(value.dsk.c_str(), dsk_internal)) {
            if (memcmp(dsk + obfuscated_bytes, dsk_internal + obfuscated_bytes, sizeof(zwave_dsk_t) - obfuscated_bytes) == 0) {
                memcpy(dsk, dsk_internal, obfuscated_bytes);
                return true;
            }
        }
    }
    return false;
}

}  // extern "C"
