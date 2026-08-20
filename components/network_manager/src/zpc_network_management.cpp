/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "zpc_nm_neighbor_discovery.h"

// ZPC includes
#include "utils.hpp"
#include "zwave_controller.h"
#include "zwave_controller_callbacks.h"
#include "zwave_network_management.h"
#include "timer.hpp"
#include "network_management_handler.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>

#define LOG_TAG "zpc_network_management"

/// The DSK reported by the node during the S2 inclusion (used by stdin and MQTT DSK/Accept)
uint8_t node_reported_dsk[16] = {0};

namespace
{
    // Case-insensitive less comparator for string-like types.
    struct less_nocase {
            template<typename T> bool operator()(const T &a, const T &b) const
            {
                return std::lexicographical_compare(std::begin(a), std::end(a), std::begin(b), std::end(b), [](char ca, char cb) { return std::tolower(static_cast<unsigned char>(ca)) < std::tolower(static_cast<unsigned char>(cb)); });
            }
    };
}  // namespace

namespace zwave_component
{
    bool network_management_handler::allow_multiple_inclusions = false;
    zwave_home_id_t network_management_handler::zpc_home_id    = 0;
    zwave_node_id_t network_management_handler::zpc_node_id    = 0;

    constexpr std::string_view NM_TOPIC_IDLE_STR                = "idle";
    constexpr std::string_view NM_TOPIC_ADD_NODE_STR            = "add node";
    constexpr std::string_view NM_TOPIC_REMOVE_NODE_STR         = "remove node";
    constexpr std::string_view NM_TOPIC_JOIN_NETWORK_STR        = "join network";
    constexpr std::string_view NM_TOPIC_LEAVE_NETWORK_STR       = "leave network";
    constexpr std::string_view NM_TOPIC_NETWORK_REPAIR_STR      = "network repair";
    constexpr std::string_view NM_TOPIC_NETWORK_UPDATE_STR      = "network update";
    constexpr std::string_view NM_TOPIC_RESET_STR               = "reset";
    constexpr std::string_view NM_TOPIC_TEMPORARILY_OFFLINE_STR = "idle";
    constexpr std::string_view NM_TOPIC_SCAN_MODE_STR           = "scan mode";

    static const std::multimap<std::string_view, ucl_network_management_state_t, less_nocase> network_management_state_map = {
      {NM_TOPIC_IDLE_STR, NM_TOPIC_IDLE},
      {NM_TOPIC_ADD_NODE_STR, NM_TOPIC_ADD_NODE},
      {NM_TOPIC_REMOVE_NODE_STR, NM_TOPIC_REMOVE_NODE},
      {NM_TOPIC_JOIN_NETWORK_STR, NM_TOPIC_JOIN_NETWORK},
      {NM_TOPIC_LEAVE_NETWORK_STR, NM_TOPIC_LEAVE_NETWORK},
      {NM_TOPIC_NETWORK_REPAIR_STR, NM_TOPIC_NETWORK_REPAIR},
      {NM_TOPIC_NETWORK_UPDATE_STR, NM_TOPIC_NETWORK_UPDATE},
      {NM_TOPIC_RESET_STR, NM_TOPIC_RESET},
      {NM_TOPIC_TEMPORARILY_OFFLINE_STR, NM_TOPIC_TEMPORARILY_OFFLINE},
      {NM_TOPIC_SCAN_MODE_STR, NM_TOPIC_SCAN_MODE},
    };

    // Static instance pointer for accessing instance from static wrapper
    network_management_handler *network_management_handler::instance_ptr = nullptr;

    const zwave_controller_callbacks_t network_management_handler::ucl_network_management_callbacks = {
      .on_state_updated          = &network_management_handler::network_management_on_state_updated_static,
      .on_node_added             = &network_management_handler::network_management_on_node_added_static,
      .on_network_address_update = &network_management_handler::network_management_on_network_address_update_static,
      .on_keys_report            = &network_management_handler::network_management_on_keys_report_static,
      .on_dsk_report             = &network_management_handler::network_management_on_dsk_report_static,
    };

    void network_management_handler::network_management_on_state_updated_static(zwave_network_management_state_t nm_state)
    {
        // Static wrapper function that forwards to the instance method
        if (network_management_handler::instance_ptr != nullptr) {
            zwave_component::network_management_handler::network_management_on_state_updated(nm_state);
        }
    }

    void network_management_handler::network_management_on_state_updated(zwave_network_management_state_t nm_state_in)
    {
        ucl_network_management_state_t nm_state = NM_TOPIC_LAST;
        switch (nm_state_in) {
            case NM_IDLE:
                nm_state = NM_TOPIC_IDLE;
                break;
            case NM_WAITING_FOR_ADD:
                nm_state = NM_TOPIC_ADD_NODE;
                break;
            case NM_NODE_FOUND:
            case NM_WAIT_FOR_PROTOCOL:
            case NM_WAIT_FOR_SECURE_ADD:
            case NM_PREPARE_SUC_INCLISION:
            case NM_WAIT_FOR_SUC_INCLUSION:
                return;  // ignore above states
            case NM_PROXY_INCLUSION_WAIT_NIF:
                nm_state = NM_TOPIC_NODE_INTERVIEW;
                break;
            case NM_SET_DEFAULT:
                nm_state = NM_TOPIC_RESET;
                break;
            case NM_LEARN_MODE:
            case NM_LEARN_MODE_STARTED:
            case NM_WAIT_FOR_SECURE_LEARN:
                nm_state = NM_TOPIC_JOIN_NETWORK;
                break;
            case NM_WAITING_FOR_NODE_REMOVAL:
                nm_state = NM_TOPIC_REMOVE_NODE;
                break;
            case NM_WAITING_FOR_FAILED_NODE_REMOVAL:
            case NM_REPLACE_FAILED_REQ:
            case NM_SEND_NOP:
            case NM_WAIT_FOR_TX_TO_SELF_DESTRUCT:
            case NM_WAIT_FOR_SELF_DESTRUCT_REMOVAL:
            case NM_FAILED_NODE_REMOVE:
                nm_state = NM_TOPIC_REMOVE_NODE;
                break;
            case NM_ASSIGNING_RETURN_ROUTE:
            case NM_NEIGHBOR_DISCOVERY_ONGOING:
                nm_state = NM_TOPIC_NETWORK_REPAIR;
                break;
            default:
                sl_log_warning(LOG_TAG, "Unhandled Network Management state: %d", nm_state_in);
                break;
        }

        if (nm_state != NM_TOPIC_LAST) {
            network_management_handler::state_topic_update(NetworkManagementStateData(nm_state));
        }
    }

    void network_management_handler::network_management_on_keys_report_static(bool csa, zwave_keyset_t keys)
    {
        network_management_on_keys_report(csa, keys);
    }

    void network_management_handler::network_management_on_keys_report(bool csa, zwave_keyset_t keys)
    {
        zwave_command_class::NetworkManagementMqttApi::publish_requested_keys(csa, keys);
    }

    void network_management_handler::network_management_on_dsk_report_static(uint8_t input_length, zwave_dsk_t dsk, zwave_keyset_t keys)
    {
        network_management_on_dsk_report(input_length, dsk, keys);
    }

    void network_management_handler::network_management_on_dsk_report(uint8_t input_length, zwave_dsk_t dsk, zwave_keyset_t keys)
    {
        if ((((keys & ZWAVE_CONTROLLER_S2_ACCESS_KEY) != 0) || ((keys & ZWAVE_CONTROLLER_S2_AUTHENTICATED_KEY) != 0)) && (input_length > 0)) {
            char dsk_str[DSK_STR_LEN];
            // Input validation of 'input_length'
            if (input_length % 2 != 0) {
                sl_log_info(LOG_TAG,
                            "Invalid DSK input length (%d). The number of missing bytes "
                            "for the DSK input must be an even number. Please try again",
                            input_length);
                return;
            }
            // More input validation of 'input_length'
            if (input_length > ZWAVE_DSK_LENGTH) {
                sl_log_info(LOG_TAG,
                            "Invalid DSK input. You have entered too much data (%d), "
                            "more than the length of a DSK (%d). Please try again",
                            input_length,
                            ZWAVE_DSK_LENGTH);

                return;
            }
            if (Utils::convert_dsk_to_dsk_str(dsk, dsk_str, sizeof(dsk_str)) != SL_STATUS_OK) {
                sl_log_info(LOG_TAG, "Failed to convert DSK to string");
                return;
            }
            // Blank out 'input_length' bytes with 'x'
            for (unsigned int i = 0; i < input_length / 2; i++) {
                // Get the position of 'I' in the following sequence: Ixxxx-Ixxxx-Ixxxx-...
                unsigned int index = (i * 5) + i;
                for (unsigned int j = 0; j < 5; j++) {
                    dsk_str[index + j] = 'x';
                }
            }

            memcpy(node_reported_dsk, dsk, sizeof(zwave_dsk_t));
            zwave_command_class::NetworkManagementMqttApi::publish_requested_dsk(input_length, std::string(dsk_str));
            network_management_handler::state_topic_update(NetworkManagementStateData(NM_TOPIC_ADD_NODE, {{"ProvisioningMode", "ZWaveDSK"}, {"SecurityCode", std::string(dsk_str)}}, {"SecurityCode", "UserAccept", "AllowMultipleInclusions"}));
        } else {
            // If the highest requested key is unauthenticated, we accept the received DSK directly
            // since the inclusion does not need authentication.
            zwave_network_management_dsk_set(dsk);
        }
    }

    void network_management_handler::network_management_on_node_added_static(sl_status_t status, const zwave_node_info_t *nif, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol)
    {
        // Static wrapper function that forwards to the instance method
        if (network_management_handler::instance_ptr != nullptr) {
            zwave_component::network_management_handler::network_management_on_node_added(status, nif, node_id, dsk, granted_keys, kex_fail_type, inclusion_protocol);
        }
    }

    void network_management_handler::network_management_on_node_added(sl_status_t status, const zwave_node_info_t *nif, zwave_node_id_t node_id, const zwave_dsk_t dsk, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type, zwave_protocol_t inclusion_protocol)
    {
        if (!zwave_network_management_is_busy()) {
            network_management_handler::state_topic_update(NetworkManagementStateData(NM_TOPIC_IDLE));
        }
    }

    void network_management_handler::network_management_on_network_address_update_static(zwave_home_id_t home_id, zwave_node_id_t node_id)
    {
        // Static wrapper function that forwards to the instance method
        if (network_management_handler::instance_ptr != nullptr) {
            zwave_component::network_management_handler::network_management_on_network_address_update(home_id, node_id);
        }
    }

    void network_management_handler::network_management_on_network_address_update(zwave_home_id_t home_id, zwave_node_id_t node_id)
    {
        zpc_home_id = home_id;
        zpc_node_id = node_id;
    }

    nlohmann::json network_management_handler::get_case_insensitive_json(const nlohmann::json &jsn, const std::string &key)
    {
        if (!jsn.is_object()) {
            return nlohmann::json();
        }
        for (auto it = jsn.begin(); it != jsn.end(); ++it) {
            const std::string &k = it.key();
            if (k.size() == key.size() && std::equal(k.begin(), k.end(), key.begin(), [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
                return it.value();
            }
        }
        return nlohmann::json();
    }

    sl_status_t network_management_handler::write_topic_received(const std::string &message)
    {
        nlohmann::json jsn;

        try {
            jsn = nlohmann::json::parse(message);
        } catch (const nlohmann::json::exception &err) {
            sl_log_debug(LOG_TAG, "Failed to parse JSON message: '%s', error: %s", message.c_str(), err.what());
            return SL_STATUS_FAIL;
        }

        try {
            std::string state_msg = get_case_insensitive_json(jsn, std::string("State"));
            sl_log_debug(LOG_TAG, "State: '%s' received", state_msg.c_str());

            auto state = network_management_state_map.find(state_msg);
            if (state == network_management_state_map.end()) {
                sl_log_debug(LOG_TAG, "Invalid state: %s, ignoring command.", state_msg.c_str());
                return SL_STATUS_FAIL;
            }

            switch (state->second) {
                case NM_TOPIC_IDLE:
                    allow_multiple_inclusions = false;
                    zwave_network_management_abort();
                    break;
                case NM_TOPIC_ADD_NODE: {
                    nlohmann::json jsn_state_param = get_case_insensitive_json(jsn, std::string("StateParameters"));

                    if (jsn_state_param.is_null()) {  // User have not supplied StateParameters, assuming add node
                        zwave_network_management_add_node();
                        break;
                    }

                    // User have supplied StateParameters
                    nlohmann::json jsn_allow_mult_inc = get_case_insensitive_json(jsn_state_param, std::string("AllowMultipleInclusions"));
                    nlohmann::json jsn_user_accept    = get_case_insensitive_json(jsn_state_param, std::string("UserAccept"));
                    nlohmann::json jsn_secur_code     = get_case_insensitive_json(jsn_state_param, std::string("SecurityCode"));

                    if (!jsn_allow_mult_inc.is_null()) {
                        allow_multiple_inclusions = jsn_allow_mult_inc;
                    }

                    if (jsn_user_accept.is_null()) {  // UserAccept have not been supplied
                        zwave_network_management_add_node();
                        break;
                    }

                    // User have supplied UserAccepted
                    bool user_accept = jsn_user_accept;

                    if (user_accept) {  // UserAccepted is true
                        sl_log_debug(LOG_TAG, "User accepted node add");

                        if (jsn_secur_code.is_null()) {  // UserAccepted is true, but SecurityCode is not supplied
                            sl_log_debug(LOG_TAG,
                                         "\"State\": \"add node\" is missing \"StateParameters\" "
                                         "\"SecurityCode\", discarding command: %s",
                                         message.c_str());
                            break;
                        }

                        // User have supplied SecurityCode
                        std::string dsk_str = jsn_secur_code;
                        zwave_dsk_t zwave_dsk;

                        if (SL_STATUS_OK == Utils::convert_dsk_str_to_dsk(dsk_str.c_str(), zwave_dsk)) {
                            zwave_network_management_dsk_set(zwave_dsk);
                        } else {  // Failed to parse dsk
                            sl_log_debug(LOG_TAG, "Failed to parse DSK: '%s'", dsk_str.c_str());
                        }
                    } else {  // UserAccepted is false
                        sl_log_debug(LOG_TAG, "User rejected node add operation (UserAccepted = false).");
                        zwave_network_management_abort();
                    }
                } break;
                case NM_TOPIC_REMOVE_NODE:
                    zwave_network_management_remove_node();
                    break;
                case NM_TOPIC_JOIN_NETWORK:
                    break;
                case NM_TOPIC_LEAVE_NETWORK:
                    break;
                case NM_TOPIC_NETWORK_REPAIR:
                    break;
                case NM_TOPIC_NETWORK_UPDATE:
                    break;
                case NM_TOPIC_RESET:
                    zwave_controller_reset();
                    if (zwave_controller_is_reset_ongoing()) {
                        // Publish immediately that we initiated reset
                        state_topic_update(NetworkManagementStateData(NM_TOPIC_RESET));
                    }
                    break;
                case NM_TOPIC_TEMPORARILY_OFFLINE:
                    break;
                case NM_TOPIC_SCAN_MODE:
                    break;
                case NM_TOPIC_LAST:
                    break;
                default:
                    sl_log_warning(LOG_TAG, "Unhandled Network Management Topic: %d", state->second);
                    break;
            }
        } catch (const nlohmann::json::exception &err) {
            sl_log_error(LOG_TAG, "JSON payload invalid %s, Error: %s", message.c_str(), err.what());
        }
        return SL_STATUS_OK;
    }

    nlohmann::json network_management_handler::get_supported_states(ucl_network_management_state_t state)
    {
        switch (state) {
            case NM_TOPIC_IDLE:
                return nlohmann::json::array({NM_TOPIC_ADD_NODE_STR, NM_TOPIC_REMOVE_NODE_STR, NM_TOPIC_RESET_STR});

            case NM_TOPIC_ADD_NODE:
                return nlohmann::json::array({NM_TOPIC_IDLE_STR});
            case NM_TOPIC_REMOVE_NODE:
                return nlohmann::json::array({NM_TOPIC_IDLE_STR});

            case NM_TOPIC_REMOVE_FAILED_NODE:
                return nlohmann::json::array({NM_TOPIC_IDLE_STR});
            case NM_TOPIC_JOIN_NETWORK:
                return nlohmann::json::array({NM_TOPIC_IDLE_STR});
            case NM_TOPIC_LEAVE_NETWORK:
                break;
            case NM_TOPIC_NODE_INTERVIEW:
                break;
            case NM_TOPIC_NETWORK_REPAIR:
                return nlohmann::json::array({NM_TOPIC_IDLE_STR});
            case NM_TOPIC_NETWORK_UPDATE:
                break;
            case NM_TOPIC_RESET:
                break;
            case NM_TOPIC_TEMPORARILY_OFFLINE:
                break;
            case NM_TOPIC_SCAN_MODE:
                return nlohmann::json::array({NM_TOPIC_IDLE_STR});
            default:
                break;
        }
        return nlohmann::json::array();
    }

    sl_status_t network_management_handler::state_topic_update(const NetworkManagementStateData &state_data)
    {
        // According to spec, when AllowMultipleInclusions is true, it shall remain
        // in add node when first add node is finished. It shall remain in this state,
        // until a "State": "idle" is received on the MQTT interface.
        if (allow_multiple_inclusions && state_data.state == NM_TOPIC_IDLE) {
            sl_log_debug(LOG_TAG,
                         "allow_multiple_inclusions is true, and state is 'idle', "
                         "triggering add_node again");
            zwave_network_management_add_node();
            return SL_STATUS_OK;
        }
        // find key in map where value match network management state (reverse lookup)
        for (auto it = network_management_state_map.begin(); it != network_management_state_map.end(); ++it) {
            if (it->second == state_data.state) {
                nlohmann::json root;
                // State
                root["State"] = it->first;

                // SupportedStateList
                root["SupportedStateList"] = get_supported_states(it->second);

                // StateParameters
                if (!state_data.state_parameters.empty()) {
                    root["StateParameters"] = state_data.state_parameters;
                }

                // RequestedStateParamters
                if (!state_data.requested_state_parameters.empty()) {
                    root["RequestedStateParameters"] = state_data.requested_state_parameters;
                }

                return SL_STATUS_OK;
            }
        }
        sl_log_error(LOG_TAG, "Invalid topic state: %d", state_data.state);
        return SL_STATUS_FAIL;
    }

    void network_management_handler::network_management_init(void)
    {
        assert("network_management_state_map doesn't contain all elements from "
               "ucl_network_management_state_t"
               && network_management_state_map.size() != NM_TOPIC_LAST - 1);
        zwave_controller_register_callbacks(&network_management_handler::ucl_network_management_callbacks);

        zpc_home_id = zwave_network_management_get_home_id();
        zpc_node_id = zwave_network_management_get_node_id();
        network_management_handler::network_management_on_state_updated_static(zwave_network_management_get_state());
        /// intialize network management node neighbor discovery submodule
        ucl_nm_neighbor_discovery_init();
    }

    void network_management_handler::network_management_exit(void)
    {
        NetworkManagementStateData nm_state_data(NM_TOPIC_TEMPORARILY_OFFLINE);
        state_topic_update(nm_state_data);
    }

    void network_management_handler::network_management_mqtt_callback_static(const std::string &topic, const std::string &message)
    {
        // Static wrapper function that can be used as a function pointer
        // It forwards the call to the instance method
        if (instance_ptr != nullptr) {
            instance_ptr->network_management_mqtt_callback(topic, message);
        } else {
            sl_log_warning(LOG_TAG, "MQTT callback called but instance_ptr is null");
        }
    }

    void network_management_handler::network_management_mqtt_callback(const std::string &topic, const std::string &message)
    {
        sl_log_debug(LOG_TAG, "Network Manager received Topic: %s state: %s\n", topic.c_str(), message.c_str());
        if (message.empty()) {
            return;
        }
        network_management_event_data ev;
        ev.event = network_management_event_t::MQTT_CB_EVENT;
        ev.data  = std::make_any<std::string>(message);
        network_management_handler::event_queue.push(ev);
    }

    network_management_handler::network_management_handler() : threading("Network Management Handler")
    {
        instance_ptr = this;
        network_management_init();
        sl_log_debug(LOG_TAG, "Network Management Handler initialized");
    }

    void network_management_handler::run()
    {
        std::optional<network_management_event_data> ev = network_management_handler::event_queue.pop(10);
        if (ev.has_value()) {
            switch (ev.value().event) {
                case network_management_event_t::MQTT_CB_EVENT: {
                    const std::string &message = std::any_cast<const std::string &>(ev.value().data);
                    network_management_handler::write_topic_received(message);
                } break;

                default:
                    sl_log_warning(LOG_TAG, "Unhandled event %d", ev.value().event);
                    break;
            }
        }

        // Check if we should stop before attempting to read
        if (should_stop()) {
            return;
        }
    }

    network_management_handler::~network_management_handler()
    {
        network_management_exit();
        instance_ptr = nullptr;
        sl_log_debug(LOG_TAG, "Network Management Handler exited");
    }

    sl_status_t network_management_handler::initialize()
    {
        network_management_mqtt_api_instance.setup_mqtt_api();
        return SL_STATUS_OK;
    }

    int network_management_handler::shutdown()
    {
        stop();
        return 0;
    }

    std::string network_management_handler::name() const
    {
        return "Network Management Handler";
    }
}  // namespace zwave_component
