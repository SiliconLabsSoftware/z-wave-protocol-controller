/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "zwave_command_class_base.h"

// ZPC
#include "log.h"

// ZPC
// Attribute
#include "attribute_store_defined_attribute_types.h"
#include "attribute_callbacks.hpp"
#include "attribute_store_helper.h"
#include "zpc_attribute_store_network_helper.h"
#include "device_interviewer_attribute_store.hpp"

// Component Connector
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"

// S2/S0/MC endpoint-specific CC lists
#include "command_class_security_2_types.hpp"
#include "command_class_security_types.hpp"
#include "command_class_multi_channel_generated_types.hpp"
#include "zwave_command_class_utils.hpp"
#include "ZW_classcmd.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace zwave_command_class
{
    constexpr char LOG_TAG[] = "zwave_command_class_base";

    std::map<int, int> zwave_command_class_base::supported_command_class_versions = {};
    static std::once_flag cc_interview_action_handler_registered;

    zwave_command_class_base::zwave_command_class_base(command_class_properties cc_properties, const std::vector<attribute_schema_t> &attributes, const std::string &mqtt_cc_name) :
      properties(cc_properties), mqtt_command_class_namespace(mqtt_cc_name), m_frame_generator(cc_properties.command_class_id)
    {
        sl_status_t status = SL_STATUS_OK;

        status = register_attribute_types(attributes);
        if (status != SL_STATUS_OK) {
            sl_log_critical(LOG_TAG, "Failed to register attributes for command class 0x%.2x", cc_properties.command_class_id);
        }

        zwave_command_class_base::supported_command_class_versions[cc_properties.command_class_id] = cc_properties.supported_version;

        // Connect to interview done event instead of attribute_store callback
        component_connector connector;
        connector.connect_typed<component_connector_common_events_t, component_connector_interview_done_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE,
                                                                                                                   [this](const component_connector_interview_done_payload_t &payload) { return this->interview(payload.endpoint_node); });

        std::call_once(cc_interview_action_handler_registered, [&connector] {
            connector.connect_typed<component_connector_common_events_t, component_connector_cc_interview_action_payload_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_CC_INTERVIEW_ACTION_REQUESTED, [](const component_connector_cc_interview_action_payload_t &payload) {
                attribute_store::attribute endpoint(payload.endpoint_node);
                if (!endpoint.is_valid()) {
                    return SL_STATUS_FAIL;
                }
                switch (payload.action) {
                    case component_connector_cc_interview_action_t::seed:
                        if (payload.command_classes.empty()) {
                            zwave_command_class_base::seed_cc_interview_state(endpoint);
                        } else {
                            zwave_command_class_base::seed_cc_interview_state(endpoint, payload.command_classes);
                        }
                        return SL_STATUS_OK;
                    case component_connector_cc_interview_action_t::check:
                        zwave_command_class_base::check_cc_interview_state(endpoint);
                        return SL_STATUS_OK;
                    case component_connector_cc_interview_action_t::cancel:
                        if (!zwave_command_class_base::cancel_cc_interview_state(endpoint)) {
                            component_connector connector;
                            component_connector_interview_done_payload_t failure {.endpoint_node = endpoint, .status = SL_STATUS_FAIL};
                            connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED), failure);
                        }
                        return SL_STATUS_OK;
                }
                return SL_STATUS_FAIL;
            });
        });
    }

    sl_status_t zwave_command_class_base::register_attribute_types(const attribute_list_registration_t &attributes)
    {
        sl_status_t status = SL_STATUS_OK;
        for (auto const &a: attributes) {
            status = attribute_store_register_type(a.type, a.name, a.parent_type, a.storage_type);
            if (status != SL_STATUS_OK) {
                sl_log_critical(LOG_TAG, "Failed to register user attribute %s for command class 0x%.2x", a.name);
            }
        }

        return status;
    }

    sl_status_t zwave_command_class_base::control_handler([[maybe_unused]] const zwave_controller_connection_info_t *connection_info, [[maybe_unused]] const uint8_t *frame_data, [[maybe_unused]] uint16_t frame_length)
    {
        return SL_STATUS_NOT_SUPPORTED;
    }
    sl_status_t zwave_command_class_base::support_handler([[maybe_unused]] const zwave_controller_connection_info_t *connection_info, [[maybe_unused]] const uint8_t *frame_data, [[maybe_unused]] uint16_t frame_length)
    {
        return SL_STATUS_NOT_SUPPORTED;
    }

    bool zwave_command_class_base::has_control_handler() const
    {
        return false;
    }
    bool zwave_command_class_base::has_support_handler() const
    {
        return false;
    }

    zwave_command_class_t zwave_command_class_base::id() const
    {
        return properties.command_class_id;
    }

    uint8_t zwave_command_class_base::supported_version() const
    {
        return properties.supported_version;
    }

    zwave_controller_encapsulation_scheme_t zwave_command_class_base::supported_handler_minimal_scheme() const
    {
        return properties.supported_handler_minimal_scheme;
    }

    std::string zwave_command_class_base::display_name() const
    {
        return properties.command_class_name;
    }

    std::string zwave_command_class_base::comments() const
    {
        return properties.comments;
    }

    bool zwave_command_class_base::manual_security_validation() const
    {
        return properties.manual_security_validation;
    }

    uint8_t zwave_command_class_base::endpoint_supported_version(const attribute_store::attribute &node) const
    {
        uint8_t version = 0;

        auto version_node = node.child_by_type(ZWAVE_CC_VERSION_ATTRIBUTE(properties.command_class_id));

        if (version_node.reported_exists()) {
            version = version_node.reported<uint8_t>();
        }

        return version;
    }

    sl_status_t zwave_command_class_base::validate_command_version(attribute_store::attribute group_node, uint8_t command, uint8_t min_version) const
    {
        for (auto node = group_node; node.is_valid(); node = node.parent()) {
            if (node.type() == ATTRIBUTE_ENDPOINT_ID) {
                zwave_node_id_t node_id         = 0;
                zwave_endpoint_id_t endpoint_id = 0;
                attribute_store_network_helper_get_zwave_ids_from_node(node, &node_id, &endpoint_id);

                uint8_t supported_version = endpoint_supported_version(node);
                const auto endpoint_0     = attribute_store::attribute(attribute_store_get_endpoint_0_node(node.parent()));
                if (supported_version == 0) {
                    if (endpoint_0.is_valid()) {
                        supported_version = endpoint_supported_version(endpoint_0);
                    }
                }

                bool supported_on_endpoint = endpoint_supports_command_class(node);

                // Security Commands Supported Get is how endpoint-specific secure
                // capabilities are discovered. Security itself resides on the root,
                // so this bootstrap command is allowed when the root advertises it.
                const bool security_capability_get = (properties.command_class_id == COMMAND_CLASS_SECURITY && command == SECURITY_COMMANDS_SUPPORTED_GET) || (properties.command_class_id == COMMAND_CLASS_SECURITY_2 && command == SECURITY_2_COMMANDS_SUPPORTED_GET);
                if (!supported_on_endpoint && endpoint_id != 0 && security_capability_get && endpoint_0.is_valid()) {
                    supported_on_endpoint = endpoint_supports_command_class(endpoint_0);
                }

                // Forced interviews, such as Basic, are never advertised in the NIF.
                // Allow the Basic Get probe while version is still unknown (0). Other
                // commands (e.g. Basic Set) wait until a Basic Report promotes the
                // version above 0 (CL:0020.01.21.02.2).
                if (!supported_on_endpoint && force_interview_for_cc && (supported_version > 0 || command == BASIC_GET)) {
                    supported_on_endpoint = true;
                }

                // An advertised CC supports at least its mandatory v1 commands.
                // Keep this as a local lower bound: the attribute store remains 0
                // until Version CC reports the exact version.
                if (supported_on_endpoint && supported_version == 0) {
                    supported_version = 1;
                }

                if (supported_on_endpoint && supported_version >= min_version) {
                    return SL_STATUS_OK;
                }

                const auto status = attribute_store_set_reported_as_desired(group_node);
                if (status != SL_STATUS_OK) {
                    return status;
                }

                sl_log_warning(LOG_TAG, "Ignoring unsupported command 0x%02X for command class 0x%02X on endpoint %u: advertised=%u, node version %u, required version %u", command, properties.command_class_id, endpoint_id, supported_on_endpoint, supported_version, min_version);
                // The resolver interprets ALREADY_EXISTS as a successful
                // no-frame completion. The group was settled above, so this
                // prevents it from being retried.
                return SL_STATUS_ALREADY_EXISTS;
            }
        }

        const auto status = attribute_store_set_reported_as_desired(group_node);
        if (status != SL_STATUS_OK) {
            return status;
        }

        sl_log_warning(LOG_TAG, "Ignoring command 0x%02X for command class 0x%02X: resolver group has no endpoint", command, properties.command_class_id);
        // See above: this completes the resolver group without transmitting.
        return SL_STATUS_ALREADY_EXISTS;
    }

    bool zwave_command_class_base::endpoint_supports_command_class(const attribute_store::attribute &endpoint_node) const
    {
        using s2_t           = command_class_security_2_types::security_2_commands_supported_report_group_attributes_t;
        using s0_t           = command_class_security_types::security_commands_supported_report_group_attributes_t;
        using mc_t           = command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t;
        const uint16_t cc_id = static_cast<uint16_t>(properties.command_class_id);

        const auto check_node = [cc_id](const attribute_store::attribute &cc_node) {
            if (!cc_node.is_valid() || !cc_node.reported_exists()) {
                return false;
            }
            try {
                const auto list = cc_node.reported<std::vector<uint8_t>>();

                std::vector<uint8_t> normal_command_classes    = command_class_utils::get_normal_command_classes(list);
                std::vector<uint16_t> extended_command_classes = command_class_utils::get_extended_command_classes(list);

                if (std::find(normal_command_classes.begin(), normal_command_classes.end(), cc_id) != normal_command_classes.end()) {
                    return true;
                }
                if (std::find(extended_command_classes.begin(), extended_command_classes.end(), cc_id) != extended_command_classes.end()) {
                    return true;
                }

                return false;
            } catch (...) {
                return false;
            }
        };

        const auto check_group = [&endpoint_node, &check_node](attribute_store_type_t group_type, attribute_store_type_t list_type) {
            const auto group = endpoint_node.child_by_type(group_type);
            return group.is_valid() && check_node(group.child_by_type(list_type));
        };

        using nif_t = node_information_group_attributes_t;

        return check_group(static_cast<attribute_store_type_t>(nif_t::NODE_INFORMATION_GROUP), static_cast<attribute_store_type_t>(nif_t::command_class_list)) || check_node(endpoint_node.child_by_type(ATTRIBUTE_ZWAVE_NIF)) || check_node(endpoint_node.child_by_type(ATTRIBUTE_ZWAVE_SECURE_NIF))
               || check_group(static_cast<attribute_store_type_t>(s2_t::SECURITY_2_COMMANDS_SUPPORTED_REPORT_GROUP), static_cast<attribute_store_type_t>(s2_t::command_class))
               || check_group(static_cast<attribute_store_type_t>(s0_t::SECURITY_COMMANDS_SUPPORTED_REPORT_GROUP), static_cast<attribute_store_type_t>(s0_t::command_class_support))
               || check_group(static_cast<attribute_store_type_t>(mc_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP), static_cast<attribute_store_type_t>(mc_t::command_class));
    }

    bool zwave_command_class_base::is_root_of_multi_endpoint_device(const attribute_store::attribute &endpoint_node)
    {
        zwave_node_id_t node_id         = 0;
        zwave_endpoint_id_t endpoint_id = 0;
        if (attribute_store_network_helper_get_zwave_ids_from_node(endpoint_node, &node_id, &endpoint_id) != SL_STATUS_OK) {
            return false;
        }
        if (endpoint_id != 0) {
            return false;
        }

        return std::ranges::any_of(endpoint_node.parent().children(ATTRIBUTE_ENDPOINT_ID), [](const attribute_store::attribute &ep) { return ep.reported_exists() && ep.reported<uint8_t>() > 0; });
    }

    // interview() early-return paths: clear a seeded row without on_interview().
    static void mark_seeded_cc_interview_done(attribute_store::attribute endpoint, zwave_command_class_t cc_id)
    {
        zwave_command_class_base::set_cc_interview_state(endpoint, cc_id, zwave_command_class_base::cc_interview_state::done);
    }

    sl_status_t zwave_command_class_base::interview(attribute_store_node_t endpoint_node)
    {
        attribute_store::attribute endpoint(endpoint_node);

        uint8_t supporting_node_version = endpoint_supported_version(endpoint);
        const bool supported            = endpoint_supports_command_class(endpoint);

        if (supporting_node_version == 0 && !supported && !force_interview_for_cc) {
            mark_seeded_cc_interview_done(endpoint, properties.command_class_id);
            return SL_STATUS_OK;
        }

        if (supporting_node_version == 0) {
            zwave_node_id_t node_id         = 0;
            zwave_endpoint_id_t endpoint_id = 0;
            attribute_store_network_helper_get_zwave_ids_from_node(endpoint, &node_id, &endpoint_id);
            if (endpoint_id != 0) {
                auto ep0 = attribute_store::attribute(attribute_store_get_endpoint_0_node(endpoint.parent()));
                if (ep0.is_valid()) {
                    supporting_node_version = endpoint_supported_version(ep0);
                }
            }
        }

        if (is_root_of_multi_endpoint_device(endpoint)) {
            for (const auto &sibling: endpoint.parent().children(ATTRIBUTE_ENDPOINT_ID)) {
                if (sibling.reported_exists() && sibling.reported<uint8_t>() != 0 && endpoint_supports_command_class(sibling)) {
                    mark_seeded_cc_interview_done(endpoint, properties.command_class_id);
                    return SL_STATUS_OK;
                }
            }
        }

        const uint8_t version_for_callback = (supporting_node_version != 0) ? supporting_node_version : 1;
        m_interview_resolution_options     = {.retry_count = 5};
        m_interview_endpoint               = endpoint;
        this->on_interview(endpoint, version_for_callback);
        return SL_STATUS_OK;
    }

    const group_resolution_options &zwave_command_class_base::interview_resolution_options() const
    {
        return m_interview_resolution_options;
    }

    void zwave_command_class_base::mqtt_command_handler(attribute_store::attribute endpoint_node, const std::string &command_name, const std::string &payload)
    {
        try {
            mqtt_callback_map.at(command_name)(endpoint_node, payload);
        } catch (const std::out_of_range &e) {
            sl_log_error(LOG_TAG, "Unknown mqtt topic received: %s", command_name.c_str());
        }
    }

    void zwave_command_class_base::mqtt_publish_supported_commands(const attribute_store::attribute &endpoint_node) {}

    void zwave_command_class_base::mqtt_register() {}

    const std::string &zwave_command_class_base::mqtt_class_namespace() const
    {
        return mqtt_command_class_namespace;
    }

    void zwave_command_class_base::mqtt_register_command_handler(void)
    {
        // Commands registration
        zpc_mqtt::register_command(this->mqtt_command_class_namespace, [this](attribute_store::attribute endpoint_node, const std::string &command_name, const std::string &payload) { this->mqtt_command_handler(endpoint_node, command_name, payload); });
    }

    void zwave_command_class_base::on_interview([[maybe_unused]] attribute_store::attribute endpoint_node, [[maybe_unused]] uint8_t supported_version)
    {
        set_cc_interview_state(cc_interview_state::done);
    }

    static attribute_store::attribute cc_interview_published_group(const attribute_store::attribute &endpoint)
    {
        auto device = endpoint.parent();
        auto ep0    = device.emplace_node(ATTRIBUTE_ENDPOINT_ID, 0);
        return ep0.emplace_node(ATTRIBUTE_CC_INTERVIEW_ONGOING_GROUP);
    }

    void zwave_command_class_base::seed_cc_interview_state(attribute_store::attribute endpoint, const std::vector<uint8_t> &command_classes)
    {
        auto group = endpoint.emplace_node(ATTRIBUTE_CC_INTERVIEW_ONGOING_GROUP);
        // A reported value on the empty ep0 group is the device-wide "published"
        // latch. A new interview must clear it before any CC can complete.
        attribute_store_undefine_reported(cc_interview_published_group(endpoint));

        std::vector<uint16_t> ids;
        const auto normal   = command_class_utils::get_normal_command_classes(command_classes);
        const auto extended = command_class_utils::get_extended_command_classes(command_classes);
        ids.insert(ids.end(), normal.begin(), normal.end());
        ids.insert(ids.end(), extended.begin(), extended.end());
        // Basic is never advertised. Probe it only on the root: Version Command
        // Class Get is always sent to ep0, so a Basic row on a non-root endpoint
        // would stay ongoing forever.
        zwave_node_id_t node_id         = 0;
        zwave_endpoint_id_t endpoint_id = 0;
        if (attribute_store_network_helper_get_zwave_ids_from_node(endpoint, &node_id, &endpoint_id) == SL_STATUS_OK && endpoint_id == 0) {
            ids.push_back(COMMAND_CLASS_BASIC);
        }
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        for (const auto id: ids) {
            if (id == 0) {
                continue;
            }
            // Only seed CCs that inherit zwave_command_class_base and therefore
            // receive INTERVIEW_DONE. Older CCs (Association, Version, Transport
            // Service, ...) never mark the latch and would stall Completed forever.
            if (!supported_command_class_versions.contains(static_cast<int>(id))) {
                continue;
            }
            auto cc = group.emplace_node(ATTRIBUTE_CC_INTERVIEW_COMMAND_CLASS, id);
            cc.emplace_node(ATTRIBUTE_CC_INTERVIEW_STATE).set_reported<uint8_t>(static_cast<uint8_t>(cc_interview_state::ongoing));
        }
    }

    void zwave_command_class_base::seed_cc_interview_state(attribute_store::attribute endpoint)
    {
        using s2_t = command_class_security_2_types::security_2_commands_supported_report_group_attributes_t;
        using s0_t = command_class_security_types::security_commands_supported_report_group_attributes_t;
        using mc_t = command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t;
        std::vector<uint8_t> command_classes;
        const auto append = [&endpoint, &command_classes](attribute_store_type_t group_type, attribute_store_type_t list_type) {
            auto group = endpoint.child_by_type(group_type);
            if (!group.is_valid()) {
                return;
            }
            auto list = group.child_by_type(list_type);
            if (list.is_valid() && list.reported_exists()) {
                const auto values = list.reported<std::vector<uint8_t>>();
                command_classes.insert(command_classes.end(), values.begin(), values.end());
            }
        };
        append(static_cast<attribute_store_type_t>(s2_t::SECURITY_2_COMMANDS_SUPPORTED_REPORT_GROUP), static_cast<attribute_store_type_t>(s2_t::command_class));
        append(static_cast<attribute_store_type_t>(s0_t::SECURITY_COMMANDS_SUPPORTED_REPORT_GROUP), static_cast<attribute_store_type_t>(s0_t::command_class_support));
        append(static_cast<attribute_store_type_t>(mc_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP), static_cast<attribute_store_type_t>(mc_t::command_class));
        seed_cc_interview_state(endpoint, command_classes);
    }

    void zwave_command_class_base::set_cc_interview_state(cc_interview_state state)
    {
        if (m_interview_endpoint.is_valid()) {
            set_cc_interview_state(m_interview_endpoint, properties.command_class_id, state);
        }
    }

    void zwave_command_class_base::set_cc_interview_state(attribute_store::attribute endpoint, zwave_command_class_t cc_id, cc_interview_state state)
    {
        // Only touch a seeded ongoing row. After FULLY_RESOLVED (or if this CC was
        // never part of the latch), late reports / MQTT must be a no-op.
        auto group = endpoint.child_by_type(ATTRIBUTE_CC_INTERVIEW_ONGOING_GROUP);
        if (!group.is_valid()) {
            return;
        }
        auto cc = group.child_by_type_and_value(ATTRIBUTE_CC_INTERVIEW_COMMAND_CLASS, static_cast<uint16_t>(cc_id));
        if (!cc.is_valid()) {
            return;
        }
        auto state_node = cc.child_by_type(ATTRIBUTE_CC_INTERVIEW_STATE);
        if (!state_node.is_valid() || !state_node.reported_exists() || state_node.reported<uint8_t>() != static_cast<uint8_t>(cc_interview_state::ongoing)) {
            return;
        }
        state_node.set_reported<uint8_t>(static_cast<uint8_t>(state));
        if (state != cc_interview_state::ongoing) {
            check_cc_interview_state(endpoint);
        }
    }

    void zwave_command_class_base::check_cc_interview_state(attribute_store::attribute endpoint)
    {
        auto device = endpoint.parent();
        if (!device.is_valid()) {
            return;
        }
        auto published_group = cc_interview_published_group(endpoint);
        if (published_group.reported_exists()) {
            return;
        }
        for (const auto &ep: device.children(ATTRIBUTE_ENDPOINT_ID)) {
            auto group = ep.child_by_type(ATTRIBUTE_CC_INTERVIEW_ONGOING_GROUP);
            if (!group.is_valid()) {
                continue;
            }
            for (const auto &cc: group.children(ATTRIBUTE_CC_INTERVIEW_COMMAND_CLASS)) {
                auto state = cc.child_by_type(ATTRIBUTE_CC_INTERVIEW_STATE);
                if (state.is_valid() && state.reported_exists() && state.reported<uint8_t>() == static_cast<uint8_t>(cc_interview_state::ongoing)) {
                    return;
                }
            }
        }
        // Empty storage provides an atomic store-owned latch without adding a
        // fourth schema type. It prevents late reports from publishing success twice.
        attribute_store_set_reported(published_group, nullptr, 0);
        component_connector connector;
        for (const auto &ep: device.children(ATTRIBUTE_ENDPOINT_ID)) {
            component_connector_interview_done_payload_t payload {.endpoint_node = ep, .status = SL_STATUS_OK};
            connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED), payload);
        }
    }

    bool zwave_command_class_base::cancel_cc_interview_state(attribute_store::attribute endpoint)
    {
        auto device = endpoint.parent();
        if (!device.is_valid()) {
            return false;
        }
        auto published_group = cc_interview_published_group(endpoint);
        if (published_group.reported_exists()) {
            return false;
        }
        bool found = false;
        for (const auto &ep: device.children(ATTRIBUTE_ENDPOINT_ID)) {
            auto group = ep.child_by_type(ATTRIBUTE_CC_INTERVIEW_ONGOING_GROUP);
            if (!group.is_valid()) {
                continue;
            }
            found = true;
            for (const auto &cc: group.children(ATTRIBUTE_CC_INTERVIEW_COMMAND_CLASS)) {
                auto state = cc.child_by_type(ATTRIBUTE_CC_INTERVIEW_STATE);
                if (state.is_valid() && state.reported_exists() && state.reported<uint8_t>() == static_cast<uint8_t>(cc_interview_state::ongoing)) {
                    state.set_reported<uint8_t>(static_cast<uint8_t>(cc_interview_state::cancelled));
                }
            }
        }
        if (!found) {
            return false;
        }
        attribute_store_set_reported(published_group, nullptr, 0);
        component_connector connector;
        for (const auto &ep: device.children(ATTRIBUTE_ENDPOINT_ID)) {
            component_connector_interview_done_payload_t payload {.endpoint_node = ep, .status = SL_STATUS_FAIL};
            connector.fire_event(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_FULLY_RESOLVED), payload);
        }
        return true;
    }

    bool zwave_command_class_base::is_supported_on_node(attribute_store::attribute endpoint_node) const
    {
        return endpoint_supported_version(endpoint_node) > 0;
    }

}  // namespace zwave_command_class