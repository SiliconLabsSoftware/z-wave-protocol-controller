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

#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>
#include <vector>

// Base class
#include "command_class_node_naming.hpp"

#include "command_class_node_naming_attribute_store.hpp"
#include "command_class_node_naming_constants.hpp"
#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_node_naming_attribute_store";

    using command_class_node_naming_constants::char_presentation_extended_ascii;
    using command_class_node_naming_constants::char_presentation_key;
    using command_class_node_naming_constants::char_presentation_standard_ascii;
    using command_class_node_naming_constants::char_presentation_utf16;
    using command_class_node_naming_constants::character_field_max_length;
    using command_class_node_naming_constants::node_location_char_key;
    using command_class_node_naming_constants::node_name_char_key;

    command_class_node_naming_attribute_store::command_class_node_naming_attribute_store() {}

    sl_status_t command_class_node_naming_attribute_store::on_node_naming_node_name_report_received_store(attribute_store::attribute endpoint_node, command_class_node_naming_attribute_map_t attribute_map)
    {
        const auto char_presentation = get_value_or_default<uint8_t>(attribute_map, char_presentation_key, char_presentation_standard_ascii);
        const auto characters        = get_value_or_default<std::vector<uint8_t>>(attribute_map, node_name_char_key, {});
        return store_node_name(endpoint_node, char_presentation, characters);
    }

    sl_status_t command_class_node_naming_attribute_store::on_node_naming_node_location_report_received_store(attribute_store::attribute endpoint_node, command_class_node_naming_attribute_map_t attribute_map)
    {
        const auto char_presentation = get_value_or_default<uint8_t>(attribute_map, char_presentation_key, char_presentation_standard_ascii);
        const auto characters        = get_value_or_default<std::vector<uint8_t>>(attribute_map, node_location_char_key, {});
        return store_node_location(endpoint_node, char_presentation, characters);
    }

    bool command_class_node_naming_attribute_store::is_supported_char_presentation(uint8_t char_presentation)
    {
        return char_presentation == char_presentation_standard_ascii || char_presentation == char_presentation_extended_ascii || char_presentation == char_presentation_utf16;
    }

    sl_status_t command_class_node_naming_attribute_store::validate_naming_characters(uint8_t char_presentation, const std::vector<uint8_t> &characters)
    {
        if (!is_supported_char_presentation(char_presentation)) {
            sl_log_warning(LOG_TAG.data(), "Unsupported Char Presentation %u", char_presentation);
            return SL_STATUS_FAIL;
        }
        if (characters.size() > character_field_max_length) {
            sl_log_warning(LOG_TAG.data(), "Character field longer than %u bytes", character_field_max_length);
            return SL_STATUS_FAIL;
        }
        return SL_STATUS_OK;
    }

    sl_status_t command_class_node_naming_attribute_store::store_naming_value(attribute_store::attribute endpoint_node,
                                                                              attribute_store_type_t report_group_type,
                                                                              attribute_store_type_t char_presentation_type,
                                                                              attribute_store_type_t characters_type,
                                                                              uint8_t char_presentation,
                                                                              const std::vector<uint8_t> &characters)
    {
        if (!endpoint_node.is_valid()) {
            return SL_STATUS_FAIL;
        }

        auto report_group           = endpoint_node.emplace_node(report_group_type);
        auto char_presentation_node = report_group.emplace_node(char_presentation_type);
        auto characters_node        = report_group.emplace_node(characters_type);
        char_presentation_node.set_reported(char_presentation);
        characters_node.set_reported(characters);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_node_naming_attribute_store::store_node_name(attribute_store::attribute endpoint_node, uint8_t char_presentation, const std::vector<uint8_t> &characters)
    {
        const sl_status_t status = validate_naming_characters(char_presentation, characters);
        if (status != SL_STATUS_OK) {
            return status;
        }

        return store_naming_value(endpoint_node,
                                  static_cast<attribute_store_type_t>(node_naming_node_name_report_group_attributes_t::NODE_NAMING_NODE_NAME_REPORT_GROUP),
                                  static_cast<attribute_store_type_t>(node_naming_node_name_report_group_attributes_t::char__presentation),
                                  static_cast<attribute_store_type_t>(node_naming_node_name_report_group_attributes_t::node_name_char),
                                  char_presentation,
                                  characters);
    }

    sl_status_t command_class_node_naming_attribute_store::store_node_location(attribute_store::attribute endpoint_node, uint8_t char_presentation, const std::vector<uint8_t> &characters)
    {
        const sl_status_t status = validate_naming_characters(char_presentation, characters);
        if (status != SL_STATUS_OK) {
            return status;
        }

        return store_naming_value(endpoint_node,
                                  static_cast<attribute_store_type_t>(node_naming_node_location_report_group_attributes_t::NODE_NAMING_NODE_LOCATION_REPORT_GROUP),
                                  static_cast<attribute_store_type_t>(node_naming_node_location_report_group_attributes_t::char__presentation),
                                  static_cast<attribute_store_type_t>(node_naming_node_location_report_group_attributes_t::node_location_char),
                                  char_presentation,
                                  characters);
    }

}  // namespace zwave_command_class
