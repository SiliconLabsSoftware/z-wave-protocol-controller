
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

#ifndef COMMAND_CLASS_ASSOCIATION_GRP_INFO_H
#define COMMAND_CLASS_ASSOCIATION_GRP_INFO_H

#include "command_class_association_grp_info_mqtt.hpp"
#include "command_class_association_grp_info_attribute_store.hpp"
#include "command_class_association_grp_info_types.hpp"

namespace zwave_command_class
{
    using namespace command_class_association_grp_info_types;

    class command_class_association_grp_info final : public command_class_association_grp_info_attribute_store, public command_class_association_grp_info_mqtt
    {

        public:
            command_class_association_grp_info();
            ~command_class_association_grp_info() = default;

        private:
            // Commands registered for the Lifeline group, kept in memory.
            // The list is static (populated at construction time by other CCs)
            // and must survive factory resets, so the attribute store is not used.
            std::vector<std::pair<uint8_t, uint8_t>> lifeline_command_registry_;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;
            static uint8_t normalize_grouping_identifier_for_report(uint8_t requested_grouping_identifier);

            static sl_status_t on_association_group_name_get_requested(const component_connector_agi_group_name_get_payload_t &payload);
            static sl_status_t on_association_group_info_get_requested(const component_connector_agi_group_info_get_payload_t &payload);
            static sl_status_t on_association_group_command_list_get_requested(const component_connector_agi_group_command_list_get_payload_t &payload);

            sl_status_t on_association_group_name_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_grp_info_attribute_map_t payload) override;
            sl_status_t on_association_group_info_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_grp_info_attribute_map_t payload) override;
            sl_status_t on_association_group_command_list_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_association_grp_info_attribute_map_t payload) override;

            sl_status_t on_association_group_name_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_association_group_info_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_association_group_command_list_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;

            static sl_status_t on_check_command_list_requested(const component_connector_agi_check_command_list_payload_t &payload);

            static sl_status_t on_lifeline_add_requested(const component_connector_agi_lifeline_update_payload_t &payload);
            static sl_status_t on_lifeline_remove_requested(const component_connector_agi_lifeline_update_payload_t &payload);

            sl_status_t on_lifeline_command_add_requested(const component_connector_agi_lifeline_command_payload_t &payload);
            sl_status_t on_lifeline_command_remove_requested(const component_connector_agi_lifeline_command_payload_t &payload);

            static sl_status_t on_get_lifeline_destinations_requested(const component_connector_agi_empty_payload_t &payload, component_connector_agi_lifeline_destinations_t &result);
            static sl_status_t on_get_supported_groupings_count_requested(const component_connector_agi_empty_payload_t &payload, uint8_t &result);
            static sl_status_t on_get_group_max_nodes_requested(const component_connector_agi_group_max_nodes_query_t &payload, uint8_t &result);
            static sl_status_t on_get_group_destinations_requested(const component_connector_agi_group_destinations_query_t &payload, component_connector_agi_group_destinations_t &result);
            sl_status_t on_get_lifeline_command_list_requested(const component_connector_agi_empty_payload_t &payload, component_connector_agi_lifeline_command_list_t &result);

            sl_status_t on_association_group_name_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_grp_info_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_association_group_info_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_grp_info_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t
              on_association_group_command_list_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_association_grp_info_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_ASSOCIATION_GRP_INFO_H
