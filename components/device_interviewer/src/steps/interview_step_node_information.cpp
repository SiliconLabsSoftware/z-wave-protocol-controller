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

#include "interview_step_node_information.hpp"
#include "interview_state_machine.hpp"
#include "device_interviewer_attribute_store.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zpc_attribute_store_network_helper.h"
#include "attribute_store_defined_attribute_types.h"
#include "command_class_protocol_events.hpp"
#include "command_class_protocol_types.hpp"
#include "zwave_controller_utils.h"
#include "log.h"
#include <any>
#include <vector>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    bool NodeInformationStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        return event_type == device_interviewer_external_event_t::NODE_INFORMATION_RECEIVED;
    }

    StepResult NodeInformationStep::on_enter(InterviewSession &session)
    {
        (void)session;
        return stay();
    }

    StepResult NodeInformationStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        if (!event.has_value()) {
            component_connector connector;
            command_class_protocol_types::command_class_protocol_commands_request_node_info_payload_t payload_map;
            payload_map.node_id = session.node_id;
            connector.fire_event(static_cast<uint32_t>(command_class_protocol_events_t::COMMAND_CLASS_PROTOCOL_COMMANDS_REQUEST_NODE_INFO), payload_map);
            return stay();
        }

        try {
            const auto &payload = std::any_cast<component_connector_node_information_received_payload_t>(event->payload);

            attribute_store_node_t node_id_node = attribute_store_network_helper_get_zwave_node_id_node(payload.node_id);
            attribute_store::attribute device_node(node_id_node);
            attribute_store::attribute endpoint_0_node = device_node.child_by_type(ATTRIBUTE_ENDPOINT_ID);

            auto endpoint_0_node_information_group = endpoint_0_node.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::NODE_INFORMATION_GROUP));

            auto command_class_list_length_node = endpoint_0_node_information_group.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::command_class_list_length));
            command_class_list_length_node.set_reported<uint8_t>(payload.node_info.command_class_list_length);

            auto command_class_list_node                                        = endpoint_0_node_information_group.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::command_class_list));
            uint8_t nif[ZWAVE_CONTROLLER_MAXIMUM_COMMAND_CLASS_LIST_LENGTH * 2] = {};
            uint8_t nif_length                                                  = 0;
            zwave_command_class_list_pack(&payload.node_info, nif, &nif_length);
            std::vector<uint8_t> command_class_list(nif, nif + nif_length);
            command_class_list_node.set_reported<std::vector<uint8_t>>(command_class_list);

            session.node_information_command_class_list = command_class_list;

            auto listening_protocol_node = endpoint_0_node_information_group.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::listening_protocol));
            listening_protocol_node.set_reported<uint8_t>(payload.node_info.listening_protocol);

            auto optional_protocol_node = endpoint_0_node_information_group.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::optional_protocol));
            optional_protocol_node.set_reported<uint8_t>(payload.node_info.optional_protocol);

            auto basic_device_class_node = endpoint_0_node_information_group.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::basic_device_class));
            basic_device_class_node.set_reported<uint8_t>(payload.node_info.basic_device_class);

            auto generic_device_class_node = endpoint_0_node_information_group.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::generic_device_class));
            generic_device_class_node.set_reported<uint8_t>(payload.node_info.generic_device_class);

            auto specific_device_class_node = endpoint_0_node_information_group.emplace_node(static_cast<attribute_store_type_t>(node_information_group_attributes_t::specific_device_class));
            specific_device_class_node.set_reported<uint8_t>(payload.node_info.specific_device_class);

            // Update session
            session.device_node   = device_node;
            session.endpoint_node = endpoint_0_node;

            return done();
        } catch (const std::bad_any_cast &) {
            sl_log_error(LOG_TAG.data(), "Invalid payload type for NODE_INFORMATION_RECEIVED");
            return fail();
        }
    }

}  // namespace zwave_command_class
