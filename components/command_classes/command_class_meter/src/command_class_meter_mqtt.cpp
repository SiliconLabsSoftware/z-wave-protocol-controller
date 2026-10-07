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

#include "command_class_meter_mqtt.hpp"
#include "command_class_meter_attribute_store.hpp"
#include "command_class_meter_constants.hpp"

#include "log.h"
#include "zwave_command_class_mqtt_utils.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_meter_mqtt";

    command_class_meter_mqtt::command_class_meter_mqtt()
    {
        mqtt_callback_map.insert({"MeterGet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                      this->mqtt_on_meter_get_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MeterReset", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                      this->mqtt_on_meter_reset_command(endpoint_node, payload);
                                  }});
        mqtt_callback_map.insert({"MeterSupportedGet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                      this->mqtt_on_meter_supported_get_command(endpoint_node, payload);
                                  }});

        mqtt_register_command_handler();
    }

    sl_status_t command_class_meter_mqtt::mqtt_on_meter_supported_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        (void)payload;
        const uint8_t version = endpoint_supported_version(endpoint_node);
        if (version < 2) {
            sl_log_warning(LOG_TAG.data(), "MeterSupportedGet requires version 2 or newer");
            return SL_STATUS_FAIL;
        }
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_supported_get_group_attributes_t::METER_SUPPORTED_GET_GROUP));
        command_class_meter_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_meter_mqtt::mqtt_on_meter_get_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        const uint8_t version = endpoint_supported_version(endpoint_node);
        auto group_node       = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));

        if (version < 2) {
            command_class_meter_core::start_group_resolution(group_node);
            return SL_STATUS_OK;
        }

        uint8_t scale     = 0;
        uint8_t rate_type = command_class_meter_constants::RATE_TYPE_DEFAULT;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("scale", scale);
        if (version >= 4) {
            parser.parse("rate_type", rate_type);
        }
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }

        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).set_desired(scale);
        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).set_desired(rate_type);
        command_class_meter_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_meter_mqtt::mqtt_on_meter_reset_command(attribute_store::attribute &endpoint_node, std::string payload)
    {
        const uint8_t version = endpoint_supported_version(endpoint_node);
        if (version < 2) {
            sl_log_warning(LOG_TAG.data(), "MeterReset requires version 2 or newer");
            return SL_STATUS_FAIL;
        }

        if (!command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint_node)) {
            sl_log_warning(LOG_TAG.data(), "MeterReset refused: node did not advertise Meter Reset support");
            return SL_STATUS_FAIL;
        }

        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP));

        if (version < 6) {
            command_class_meter_core::start_group_resolution(group_node);
            return SL_STATUS_OK;
        }

        uint8_t meter_type = 0;
        uint8_t rate_type  = 0;
        uint8_t scale      = 0;
        uint8_t precision  = 0;
        std::vector<uint8_t> meter_value;

        mqtt_payload_parser parser {payload, LOG_TAG.data()};
        parser.parse("meter_type", meter_type).parse("rate_type", rate_type).parse("scale", scale).parse("precision", precision).parse("meter_value", meter_value);
        if (parser.status() != SL_STATUS_OK) {
            return parser.status();
        }
        if (meter_value.empty() || meter_value.size() > 4) {
            sl_log_warning(LOG_TAG.data(), "MeterReset v6: meter_value size must be 1..4 bytes");
            return SL_STATUS_INVALID_PARAMETER;
        }

        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::meter_type)).set_desired(meter_type);
        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::rate_type)).set_desired(rate_type);
        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::scale_bits_10)).set_desired(scale);
        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::precision)).set_desired(precision);
        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::size)).set_desired(static_cast<uint8_t>(meter_value.size()));
        group_node.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::meter_value)).set_desired(meter_value);
        command_class_meter_core::start_group_resolution(group_node);
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class
