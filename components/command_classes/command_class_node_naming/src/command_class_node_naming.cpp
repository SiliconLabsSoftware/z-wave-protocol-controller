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

#include <vector>

#include "command_class_node_naming.hpp"
#include "ZW_classcmd.h"

namespace zwave_command_class
{

    command_class_node_naming::command_class_node_naming() {}

    sl_status_t command_class_node_naming::on_node_naming_node_name_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        return args.get_frame_generator->generate_no_args_frame();
    }

    sl_status_t command_class_node_naming::on_node_naming_node_location_get_requested_assemble_frame(const get_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        return args.get_frame_generator->generate_no_args_frame();
    }

    sl_status_t command_class_node_naming::on_node_naming_node_name_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;
        auto presentation_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(node_naming_node_name_set_group_attributes_t::char__presentation));
        auto characters_node        = group_node.emplace_node(static_cast<attribute_store_type_t>(node_naming_node_name_set_group_attributes_t::node_name_char));
        if (!presentation_node.desired_exists() || !characters_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_raw_byte(static_cast<uint8_t>(presentation_node.desired<uint8_t>() & 0x07));
        for (uint8_t character: characters_node.desired<std::vector<uint8_t>>()) {
            frame_generator->add_raw_byte(character);
        }
        return frame_generator->generate_frame();
    }

    sl_status_t command_class_node_naming::on_node_naming_node_location_set_requested_assemble_frame(const set_requested_args &args, uint8_t * /*data*/, uint16_t * /*length*/)
    {
        auto group_node             = args.node;
        const auto &frame_generator = args.set_frame_generator;
        auto presentation_node      = group_node.emplace_node(static_cast<attribute_store_type_t>(node_naming_node_location_set_group_attributes_t::char__presentation));
        auto characters_node        = group_node.emplace_node(static_cast<attribute_store_type_t>(node_naming_node_location_set_group_attributes_t::node_location_char));
        if (!presentation_node.desired_exists() || !characters_node.desired_exists()) {
            return SL_STATUS_NOT_READY;
        }
        frame_generator->add_raw_byte(static_cast<uint8_t>(presentation_node.desired<uint8_t>() & 0x07));
        for (uint8_t character: characters_node.desired<std::vector<uint8_t>>()) {
            frame_generator->add_raw_byte(character);
        }
        return frame_generator->generate_frame();
    }

}  // namespace zwave_command_class
