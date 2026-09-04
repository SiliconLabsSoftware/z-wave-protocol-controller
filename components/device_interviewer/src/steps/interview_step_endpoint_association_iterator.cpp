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

#include "interview_step_endpoint_association_iterator.hpp"
#include "interview_state_machine.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "command_class_multi_channel_types.hpp"
#include "zwave_command_class_utils.hpp"
#include "ZW_classcmd.h"
#include "log.h"
#include <algorithm>

namespace zwave_command_class
{
    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "interview_steps";

    namespace
    {
        void append_reported_cc_list(attribute_store::attribute node, std::vector<uint8_t> &out)
        {
            if (!node.is_valid() || !node.reported_exists()) {
                return;
            }
            auto ccs = node.reported<std::vector<uint8_t>>();
            out.insert(out.end(), ccs.begin(), ccs.end());
        }

        std::vector<uint8_t> filter_command_class_list(std::vector<uint8_t> raw)
        {
            auto filtered = command_class_utils::get_normal_command_classes(raw);
            std::sort(filtered.begin(), filtered.end());
            filtered.erase(std::unique(filtered.begin(), filtered.end()), filtered.end());
            filtered.erase(std::remove(filtered.begin(), filtered.end(), COMMAND_CLASS_NO_OPERATION), filtered.end());
            filtered.erase(std::remove(filtered.begin(), filtered.end(), COMMAND_CLASS_BASIC), filtered.end());
            return filtered;
        }

        std::vector<uint8_t> collect_endpoint_supported_command_classes(attribute_store::attribute endpoint_node)
        {
            // Multi Channel Capability Report is authoritative for which CCs an End Point
            // supports. Do not fall back to Secure NIF when the report is present: S2
            // Commands Supported can list root-level CCs that the End Point does not implement.
            using mc_t = command_class_multi_channel_types::multi_channel_capability_report_group_attributes_t;
            auto grp   = endpoint_node.child_by_type(static_cast<attribute_store_type_t>(mc_t::MULTI_CHANNEL_CAPABILITY_REPORT_GROUP));
            if (grp.is_valid()) {
                std::vector<uint8_t> capability_ccs;
                append_reported_cc_list(grp.child_by_type(static_cast<attribute_store_type_t>(mc_t::command_class)), capability_ccs);
                if (!capability_ccs.empty()) {
                    return filter_command_class_list(std::move(capability_ccs));
                }
            }

            std::vector<uint8_t> raw;
            append_reported_cc_list(endpoint_node.child_by_type(ATTRIBUTE_ZWAVE_SECURE_NIF), raw);
            append_reported_cc_list(endpoint_node.child_by_type(ATTRIBUTE_ZWAVE_NIF), raw);
            return filter_command_class_list(std::move(raw));
        }
    }  // namespace

    bool EndpointAssociationIteratorStep::handles_external_event(device_interviewer_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult EndpointAssociationIteratorStep::on_enter(InterviewSession &session)
    {
        if (session.endpoints.endpoint_ids.empty()) {
            sl_log_info(LOG_TAG.data(), "Node %d: no endpoints, skipping per-endpoint association/AGI", session.node_id);
            return skip();
        }

        if (session.endpoint_id != 0) {
            // We just finished association/AGI/lifeline for an endpoint (came from POST_VALIDATE_LIFELINE)
            ++session.endpoints.current_endpoint_it;
            if (session.endpoints.current_endpoint_it == session.endpoints.endpoint_ids.end()) {
                session.endpoint_node = session.root_endpoint_node;
                session.endpoint_id   = 0;
                sl_log_info(LOG_TAG.data(), "Node %d: per-endpoint association/AGI completed for all endpoints", session.node_id);
                return skip();
            }
        } else {
            // First time (from ENDPOINT_ZWAVEPLUS_INFO): start with first endpoint
            session.endpoints.current_endpoint_it = session.endpoints.endpoint_ids.begin();
        }

        session.endpoint_node                              = session.device_node.emplace_node(ATTRIBUTE_ENDPOINT_ID, *session.endpoints.current_endpoint_it);
        session.endpoint_id                                = *session.endpoints.current_endpoint_it;
        session.agi.agi_current_group_id                   = 0;
        session.agi.agi_total_groups                       = 0;
        session.agi.agi_used_multi_channel                 = false;
        session.association_members.assoc_current_group_id = 0;
        session.version_cc.command_classes_to_query        = collect_endpoint_supported_command_classes(session.endpoint_node);
        session.version_cc.current_cc_it                   = session.version_cc.command_classes_to_query.begin();

        sl_log_info(LOG_TAG.data(), "Node %d: starting association/AGI interview for endpoint %d (%zu supported CCs)", session.node_id, session.endpoint_id, session.version_cc.command_classes_to_query.size());
        return done();
    }

    StepResult EndpointAssociationIteratorStep::handle_event(InterviewSession &session, std::optional<device_interviewer_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace zwave_command_class
