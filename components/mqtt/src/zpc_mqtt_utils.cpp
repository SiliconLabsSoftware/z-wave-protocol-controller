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
// Base class
#include "zpc_mqtt_utils.hpp"
#include "zpc_mqtt_definitions.hpp"

// Format
#include "fmt/format.h"

// Cpp
#include <regex>

// ZPC
#include "attribute_store_defined_attribute_types.h"  // ATTRIBUTE_HOME_ID, ATTRIBUTE_NODE_ID, ATTRIBUTE_ENDPOINT_ID
#include "log.h"                                      // sl_log

// Zpc
#include "zwave_node_id_definitions.h"  // zwave_home_id_t, zwave_node_id_t, zwave_endpoint_id_t
namespace zpc_mqtt
{
    namespace utils
    {

        constexpr char LOG_TAG[] = "zpc_mqtt";

        zwave_home_id_t str_to_home_id(const std::string &str)
        {
            zwave_home_id_t home_id;
            std::stringstream ss;
            ss << std::hex << str;
            ss >> home_id;
            return home_id;
        }

        zwave_node_id_t str_to_node_id(const std::string &str)
        {
            zwave_node_id_t node_id;
            std::stringstream ss;
            ss << std::hex << str;
            ss >> node_id;

            return node_id;
        }

        zwave_endpoint_id_t str_to_endpoint_id(const std::string &str)
        {
            // Avoid mismatch between uint8_t and char
            unsigned int endpoint_id;
            std::stringstream ss;
            ss << str;
            ss >> endpoint_id;

            return static_cast<zwave_endpoint_id_t>(endpoint_id);
        }

        attribute_store::attribute get_endpoint_node_from_topic(const std::string &topic)
        {
            std::regex rgx(MQTT_ROOT_NAME + "/([A-F0-9]+)/([A-F0-9]+)/ep(\\d+)");
            std::smatch match;
            attribute_store::attribute empty_attribute;

            try {
                if (std::regex_search(topic.begin(), topic.end(), match, rgx)) {
                    if (match.size() != 4) {
                        sl_log_warning(LOG_TAG, "Incorrect match size : %d (expected 4)", match.size());
                        return empty_attribute;
                    }
                    // Home ID
                    const std::string &home_id_str = match[1].str();
                    const zwave_home_id_t home_id  = str_to_home_id(home_id_str);
                    // Node ID
                    const std::string &node_id_str = match[2].str();
                    const zwave_node_id_t node_id  = str_to_node_id(node_id_str);
                    // Endpoint ID
                    const std::string &endpoint_id_str    = match[3].str();
                    const zwave_endpoint_id_t endpoint_id = str_to_endpoint_id(endpoint_id_str);

                    const auto root_node = attribute_store::attribute::root();

                    auto home_id_node = root_node.child_by_type_and_value(ATTRIBUTE_HOME_ID, home_id);

                    if (!home_id_node.is_valid()) {
                        sl_log_warning(LOG_TAG, "Home ID %d not found in the attribute store", home_id);
                        return empty_attribute;
                    }

                    auto node_id_node = home_id_node.child_by_type_and_value(ATTRIBUTE_NODE_ID, node_id);

                    if (!node_id_node.is_valid()) {
                        sl_log_warning(LOG_TAG, "Node ID %d not found in the attribute store", node_id);
                        return empty_attribute;
                    }

                    auto endpoint_node = node_id_node.child_by_type_and_value(ATTRIBUTE_ENDPOINT_ID, endpoint_id);

                    if (!endpoint_node.is_valid()) {
                        sl_log_warning(LOG_TAG, "Endpoint ID %d not found in the attribute store", endpoint_id);
                        return empty_attribute;
                    }

                    return endpoint_node;
                }
            } catch (const std::exception &e) {
                sl_log_warning(LOG_TAG, "Error in get_endpoint_node_from_topic: %s", e.what());
            }
            sl_log_debug(LOG_TAG, "No match found for topic %s", topic.c_str());
            return empty_attribute;
        }

        zwave_command_info get_command_info_from_topic(const std::string &topic)
        {
            std::regex rgx("/([0-9A-Z]+)/" + MQTT_COMMAND_KEYWORD + "/(.+)", std::regex::icase);
            std::smatch match;
            zwave_command_info command_info;

            if (std::regex_search(topic.begin(), topic.end(), match, rgx)) {
                if (match.size() != 3) {
                    sl_log_warning(LOG_TAG, "Incorrect match size : %d (expected 3)", match.size());
                    return command_info;
                }

                command_info.command_class_name = match[1].str();
                command_info.command_name       = match[2].str();
            }

            return command_info;
        }

        std::string get_base_topic_from_attribute(const attribute_store::attribute &attribute)
        {
            std::string base_topic;
            try {
                auto home_id = attribute.first_parent_or_self(ATTRIBUTE_HOME_ID).reported<zwave_home_id_t>();
                auto node_id = attribute.first_parent_or_self(ATTRIBUTE_NODE_ID).reported<zwave_node_id_t>();

                auto endpoint_attribute = attribute.first_parent_or_self(ATTRIBUTE_ENDPOINT_ID);
                if (endpoint_attribute.is_valid()) {
                    auto endpoint_id = endpoint_attribute.reported<zwave_endpoint_id_t>();
                    base_topic       = fmt::format(MQTT_BASE_TOPIC_WITH_ENDPOINT, fmt::arg("home_id", home_id), fmt::arg("node_id", node_id), fmt::arg("endpoint_id", endpoint_id));
                } else {
                    base_topic = fmt::format(MQTT_BASE_TOPIC, fmt::arg("home_id", home_id), fmt::arg("node_id", node_id));
                }
            } catch (const std::exception &e) {
                sl_log_error(LOG_TAG, "Error while formatting base topic: %s", e.what());
            }
            return base_topic;
        }

    }  // namespace utils
}  // namespace zpc_mqtt
