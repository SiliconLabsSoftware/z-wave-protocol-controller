/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <string_view>
#include <vector>

#include "command_class_configuration.hpp"
#include "command_class_configuration_constants.hpp"
#include "command_class_configuration_types.hpp"

#include "zpc_mqtt.hpp"
#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_configuration_mqtt";

    using namespace command_class_configuration_types;
    using namespace command_class_configuration_constants;

    command_class_configuration_mqtt::command_class_configuration_mqtt()
    {
        mqtt_callback_map.insert({"ConfigurationBulkGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_bulk_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationBulkSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationSet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_set_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationNameGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_name_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationInfoGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_info_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationPropertiesGet", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_properties_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationDefaultReset", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_default_reset_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationPropertiesDiscover", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_properties_discover_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"ConfigurationParameterScan", [](attribute_store::attribute &endpoint_node, std::string payload) {
                                      command_class_configuration_mqtt::mqtt_on_configuration_parameter_scan_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_bulk_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t parameter_offset    = 0;
        uint8_t number_of_parameters = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("parameter_offset", parameter_offset).parse("number_of_parameters", number_of_parameters);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (number_of_parameters == 0) {
            sl_log_warning(LOG_TAG.data(), "Configuration Bulk Get requires number_of_parameters > 0");
            return SL_STATUS_FAIL;
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::CONFIGURATION_BULK_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::parameter_offset)).set_desired(parameter_offset);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::number_of_parameters)).set_desired(number_of_parameters);
        command_class_configuration_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t parameter_offset    = 0;
        uint8_t number_of_parameters = 0;
        uint8_t size                 = 0;
        uint8_t handshake            = 0;
        uint8_t default_flag         = 0;
        configuration_bulk_set_vg_t vg;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("parameter_offset", parameter_offset).parse("number_of_parameters", number_of_parameters);
        auto properties1 = parser.parse_nested("properties1");
        properties1.parse("size", size).parse_optional("handshake", handshake).parse_optional("default_flag", default_flag);
        if (default_flag == 0) {
            properties1.parse_optional("default", default_flag);
        }
        for (auto &&[elem, item]: parser.parse_array("vg", vg)) {
            elem.parse("parameter", item.parameter);
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        if (number_of_parameters == 0 || !is_valid_size(size)) {
            sl_log_warning(LOG_TAG.data(), "Configuration Bulk Set missing valid size or number_of_parameters");
            return SL_STATUS_FAIL;
        }

        if (!command_class_configuration::bulk_set_allowed(endpoint_node, parameter_offset, number_of_parameters)) {
            return SL_STATUS_FAIL;
        }

        std::vector<uint8_t> flat_values;
        if (default_flag == 0) {
            if (vg.size() != number_of_parameters) {
                sl_log_warning(LOG_TAG.data(), "Configuration Bulk Set vg length %zu does not match number_of_parameters %u", vg.size(), number_of_parameters);
                return SL_STATUS_FAIL;
            }
            for (const auto &item: vg) {
                if (item.parameter.size() != size) {
                    sl_log_warning(LOG_TAG.data(), "Configuration Bulk Set parameter value length %zu does not match size %u", item.parameter.size(), size);
                    return SL_STATUS_FAIL;
                }
                flat_values.insert(flat_values.end(), item.parameter.begin(), item.parameter.end());
            }
        } else {
            flat_values.assign(static_cast<size_t>(number_of_parameters) * size, 0);
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::CONFIGURATION_BULK_SET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(parameter_offset);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(number_of_parameters);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(size);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired(handshake);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired(default_flag);
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg)).set_desired(flat_values);
        command_class_configuration_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t parameter_number = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("parameter_number", parameter_number);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        command_class_configuration::request_configuration_get(endpoint_node, parameter_number);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_set_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint8_t parameter_number = 0;
        uint8_t size             = 0;
        uint8_t default_flag     = 0;
        int64_t value            = 0;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("parameter_number", parameter_number);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        mqtt_payload_parser size_parser {payload, LOG_TAG.data()};
        size_parser.parse("size", size);
        if (size_parser.status() != SL_STATUS_OK || !is_valid_size(size)) {
            auto parameter_node = endpoint_node.child_by_type_and_value(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID), static_cast<uint16_t>(parameter_number));
            if (parameter_node.is_valid()) {
                auto size_node = parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size));
                if (size_node.is_valid() && size_node.reported_exists()) {
                    size = size_node.reported<uint8_t>();
                }
            }
        }
        if (!is_valid_size(size)) {
            sl_log_warning(LOG_TAG.data(), "Configuration Set missing valid size for parameter %u", parameter_number);
            return SL_STATUS_FAIL;
        }

        mqtt_payload_parser flag_parser {payload, LOG_TAG.data()};
        flag_parser.parse("default_flag", default_flag);
        if (flag_parser.status() != SL_STATUS_OK) {
            default_flag = 0;
        }

        if (default_flag == 0) {
            mqtt_payload_parser value_parser {payload, LOG_TAG.data()};
            value_parser.parse("value", value);
            if (value_parser.status() != SL_STATUS_OK) {
                return value_parser.status();
            }
        }

        command_class_configuration::request_configuration_set(endpoint_node, parameter_number, size, value, default_flag != 0);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_name_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t parameter_number = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("parameter_number", parameter_number);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::CONFIGURATION_NAME_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::parameter_number)).set_desired(parameter_number);
        command_class_configuration_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_info_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t parameter_number = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("parameter_number", parameter_number);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::CONFIGURATION_INFO_GET_GROUP));
        group_node.emplace_node(static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::parameter_number)).set_desired(parameter_number);
        command_class_configuration_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_properties_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        uint16_t parameter_number = 0;
        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("parameter_number", parameter_number);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        command_class_configuration::request_properties_get(endpoint_node, parameter_number);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_default_reset_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(configuration_default_reset_group_attributes_t::CONFIGURATION_DEFAULT_RESET_GROUP));
        command_class_configuration_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_properties_discover_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        command_class_configuration::start_properties_walk(endpoint_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_configuration_mqtt::mqtt_on_configuration_parameter_scan_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        command_class_configuration::start_parameter_scan(endpoint_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
