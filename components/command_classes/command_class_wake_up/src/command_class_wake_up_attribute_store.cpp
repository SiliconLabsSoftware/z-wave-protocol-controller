
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

#include <cstdint>
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_wake_up.hpp"

#include "command_class_wake_up_attribute_store.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_wake_up_attribute_store";

    command_class_wake_up_attribute_store::command_class_wake_up_attribute_store() {}

    sl_status_t command_class_wake_up_attribute_store::on_wake_up_interval_capabilities_report_received_store(attribute_store::attribute endpoint_node, command_class_wake_up_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_report_group_attributes_t::WAKE_UP_INTERVAL_CAPABILITIES_REPORT_GROUP));

        wake_up_interval_capabilities_report_minimum_wake_up_interval_seconds_t minimum_wake_up_interval_seconds = 0;
        minimum_wake_up_interval_seconds                                                                         = get_value_or_default(attribute_map, "minimum_wake_up_interval_seconds", minimum_wake_up_interval_seconds);
        auto minimum_wake_up_interval_seconds_node                                                               = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_report_group_attributes_t::minimum_wake_up_interval_seconds));
        minimum_wake_up_interval_seconds_node.set_reported<wake_up_interval_capabilities_report_minimum_wake_up_interval_seconds_t>(minimum_wake_up_interval_seconds);

        wake_up_interval_capabilities_report_maximum_wake_up_interval_seconds_t maximum_wake_up_interval_seconds = 0;
        maximum_wake_up_interval_seconds                                                                         = get_value_or_default(attribute_map, "maximum_wake_up_interval_seconds", maximum_wake_up_interval_seconds);
        auto maximum_wake_up_interval_seconds_node                                                               = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_report_group_attributes_t::maximum_wake_up_interval_seconds));
        maximum_wake_up_interval_seconds_node.set_reported<wake_up_interval_capabilities_report_maximum_wake_up_interval_seconds_t>(maximum_wake_up_interval_seconds);

        wake_up_interval_capabilities_report_default_wake_up_interval_seconds_t default_wake_up_interval_seconds = 0;
        default_wake_up_interval_seconds                                                                         = get_value_or_default(attribute_map, "default_wake_up_interval_seconds", default_wake_up_interval_seconds);
        auto default_wake_up_interval_seconds_node                                                               = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_report_group_attributes_t::default_wake_up_interval_seconds));
        default_wake_up_interval_seconds_node.set_reported<wake_up_interval_capabilities_report_default_wake_up_interval_seconds_t>(default_wake_up_interval_seconds);

        wake_up_interval_capabilities_report_wake_up_interval_step_seconds_t wake_up_interval_step_seconds = 0;
        wake_up_interval_step_seconds                                                                      = get_value_or_default(attribute_map, "wake_up_interval_step_seconds", wake_up_interval_step_seconds);
        auto wake_up_interval_step_seconds_node                                                            = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_report_group_attributes_t::wake_up_interval_step_seconds));
        wake_up_interval_step_seconds_node.set_reported<wake_up_interval_capabilities_report_wake_up_interval_step_seconds_t>(wake_up_interval_step_seconds);

        wake_up_interval_capabilities_report_properties1_t properties1 = {.value = 0};
        properties1.value                                              = get_value_or_default(attribute_map, "wake_up_on_demand", properties1.value);
        auto wake_up_on_demand_node                                    = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_capabilities_report_group_attributes_t::wake_up_on_demand));
        wake_up_on_demand_node.set_reported<uint8_t>(properties1.value);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_wake_up_attribute_store::on_wake_up_interval_report_received_store(attribute_store::attribute endpoint_node, command_class_wake_up_attribute_map_t attribute_map)
    {
        auto group_node = endpoint_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_report_group_attributes_t::WAKE_UP_INTERVAL_REPORT_GROUP));

        wake_up_interval_report_seconds_t seconds = 0;
        seconds                                   = get_value_or_default(attribute_map, "seconds", seconds);
        auto seconds_node                         = group_node.emplace_node(static_cast<attribute_store_type_t>(wake_up_interval_report_group_attributes_t::seconds));
        seconds_node.set_reported<uint32_t>(seconds);

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class