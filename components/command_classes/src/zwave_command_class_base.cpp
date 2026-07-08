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
#include "zpc_attribute_store_network_helper.h"

// Component Connector
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"

// S2/S0/MC endpoint-specific CC lists
#include "command_class_s2_types.hpp"
#include "command_class_s0_types.hpp"
#include "command_class_multi_channel_generated_types.hpp"
#include "zwave_command_class_utils.hpp"

#include <algorithm>
#include <vector>

namespace zwave_command_class
{
    constexpr char LOG_TAG[] = "zwave_command_class_base";

    std::map<int, int> zwave_command_class_base::supported_command_class_versions = {};

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

    bool zwave_command_class_base::endpoint_supports_command_class(const attribute_store::attribute &endpoint_node) const
    {
        using s2_t           = command_class_s2_types::s2_commands_supported_report_group_attributes_t;
        using s0_t           = command_class_s0_types::s0_commands_supported_report_group_attributes_t;
        using mc_t           = command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t;
        const uint16_t cc_id = static_cast<uint16_t>(properties.command_class_id);

        const auto check = [&endpoint_node, cc_id](attribute_store_type_t group_type, attribute_store_type_t list_type) {
            auto grp = endpoint_node.child_by_type(group_type);
            if (!grp.is_valid()) {
                return false;
            }
            auto cc_node = grp.child_by_type(list_type);
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

        return check(static_cast<attribute_store_type_t>(s2_t::S2_COMMANDS_SUPPORTED_REPORT_GROUP), static_cast<attribute_store_type_t>(s2_t::command_class))
               || check(static_cast<attribute_store_type_t>(s0_t::S0_COMMANDS_SUPPORTED_REPORT_GROUP), static_cast<attribute_store_type_t>(s0_t::command_class))
               || check(static_cast<attribute_store_type_t>(mc_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP), static_cast<attribute_store_type_t>(mc_t::command_class));
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

    sl_status_t zwave_command_class_base::interview(attribute_store_node_t endpoint_node)
    {
        attribute_store::attribute endpoint(endpoint_node);

        uint8_t supporting_node_version = endpoint_supported_version(endpoint);
        const bool supported            = endpoint_supports_command_class(endpoint);

        if (supporting_node_version == 0 && !supported && !force_interview_for_cc) {
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
                    return SL_STATUS_OK;
                }
            }
        }

        const uint8_t version_for_callback = (supporting_node_version != 0) ? supporting_node_version : 1;
        m_interview_resolution_options     = {.retry_count = 5};
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

    void zwave_command_class_base::on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) {}

    bool zwave_command_class_base::is_supported_on_node(attribute_store::attribute endpoint_node) const
    {
        return endpoint_supported_version(endpoint_node) > 0;
    }

}  // namespace zwave_command_class