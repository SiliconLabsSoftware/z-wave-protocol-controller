
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

// Base class
#include "command_class_association.hpp"

#include "command_class_association_attribute_store.hpp"
#include "command_class_association_events.hpp"
#include "command_class_association_types.hpp"
#include "component_connector.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_association_attribute_store";

    command_class_association_attribute_store::command_class_association_attribute_store() {}

    sl_status_t command_class_association_attribute_store::on_association_groupings_report_received_store(attribute_store::attribute endpoint_node, command_class_association_attribute_map_t attribute_map)
    {
        auto group_node               = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(command_class_association_types::association_groupings_report_group_attributes_t::ASSOCIATION_GROUPINGS_REPORT_GROUP));
        auto supported_groupings_node = group_node.emplace_node(static_cast<attribute_store_type_t>(command_class_association_types::association_groupings_report_group_attributes_t::supported_groupings));

        command_class_association_types::association_groupings_report_supported_groupings_t supported_groupings = 0;
        supported_groupings                                                                                     = get_value_or_default(attribute_map, "supported_groupings", supported_groupings);
        supported_groupings_node.set_reported<command_class_association_types::association_groupings_report_supported_groupings_t>(supported_groupings);

        component_connector_association_groupings_get_payload_t payload;
        payload.endpoint_node = endpoint_node;
        component_connector connector;
        connector.fire_event(static_cast<uint32_t>(command_class_association_events_t::COMMAND_CLASS_ASSOCIATION_GROUPINGS_REPORT), payload);
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_attribute_store::on_association_report_received_store(attribute_store::attribute endpoint_node, command_class_association_attribute_map_t attribute_map)
    {
        return SL_STATUS_OK;
    }

    sl_status_t command_class_association_attribute_store::on_association_specific_group_report_received_store(attribute_store::attribute endpoint_node, command_class_association_attribute_map_t attribute_map)
    {
        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class