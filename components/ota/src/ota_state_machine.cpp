/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "ota_state_machine.hpp"
#include "mqtt_api_base.hpp"
#include "ota_mqtt_api.hpp"
#include "ota_step.hpp"
#include "ota_mqtt_constants.hpp"
#include "update_manager_types.hpp"

#include "steps/ota_step_transfer_done.hpp"
#include "steps/ota_step_failed.hpp"
#include "steps/ota_step_start_upload.hpp"
#include "steps/ota_step_upload_prepare.hpp"
#include "steps/ota_step_publish_transfer_progress.hpp"
#include "steps/ota_step_record_pending_transfer_abort.hpp"
#include "steps/ota_step_process_device_transfer_outcome.hpp"
#include "steps/ota_step_deliver_requested_firmware_chunks.hpp"
#include "steps/ota_step_waiting_for_activation.hpp"
#include "steps/ota_step_activating.hpp"
#include "steps/ota_step_waiting_for_reconnect.hpp"
#include "steps/ota_step_trigger_interview.hpp"

#include "log.h"
#include "nlohmann/json.hpp"

#include <any>
#include <string_view>

namespace ota
{

    using namespace mqtt_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_state_machine";

    void OtaStateMachine::register_transitions()
    {
        using state_machine::StepResultCode;

        set_transition(OtaState::START_UPLOAD, StepResultCode::DONE, OtaState::UPLOAD_PREPARE_TRANSFER);
        set_transition(OtaState::START_UPLOAD, StepResultCode::SKIP, OtaState::TRANSFER_DONE);

        set_transition(OtaState::UPLOAD_PREPARE_TRANSFER, StepResultCode::DONE, OtaState::UPLOAD_DELIVER_FIRMWARE_CHUNKS);
        set_transition(OtaState::UPLOAD_PREPARE_TRANSFER, StepResultCode::FAIL, OtaState::FAILED);

        set_transition(OtaState::UPLOAD_PUBLISH_TRANSFER_PROGRESS, StepResultCode::DONE, OtaState::UPLOAD_DELIVER_FIRMWARE_CHUNKS);
        set_transition(OtaState::UPLOAD_PUBLISH_TRANSFER_PROGRESS, StepResultCode::SKIP, OtaState::TRANSFER_DONE);
        set_transition(OtaState::UPLOAD_PUBLISH_TRANSFER_PROGRESS, StepResultCode::FAIL, OtaState::FAILED);

        set_transition(OtaState::UPLOAD_RECORD_PENDING_TRANSFER_ABORT, StepResultCode::DONE, OtaState::TRANSFER_DONE);
        set_transition(OtaState::UPLOAD_RECORD_PENDING_TRANSFER_ABORT, StepResultCode::FAIL, OtaState::FAILED);

        set_transition(OtaState::UPLOAD_DELIVER_FIRMWARE_CHUNKS, StepResultCode::FAIL, OtaState::FAILED);
        set_transition(OtaState::UPLOAD_DELIVER_FIRMWARE_CHUNKS, StepResultCode::DONE, OtaState::UPLOAD_PROCESS_DEVICE_OUTCOME);

        // UPLOAD_PROCESS_DEVICE_OUTCOME routing is handled explicitly in process_event
        // based on session.transfer_outcome to distinguish the three success paths.
        set_transition(OtaState::UPLOAD_PROCESS_DEVICE_OUTCOME, StepResultCode::DONE, OtaState::TRANSFER_DONE);
        set_transition(OtaState::UPLOAD_PROCESS_DEVICE_OUTCOME, StepResultCode::FAIL, OtaState::FAILED);

        set_transition(OtaState::WAITING_FOR_ACTIVATION, StepResultCode::DONE, OtaState::ACTIVATING);
        set_transition(OtaState::WAITING_FOR_ACTIVATION, StepResultCode::FAIL, OtaState::FAILED);

        set_transition(OtaState::ACTIVATING, StepResultCode::DONE, OtaState::WAITING_FOR_RECONNECT);
        set_transition(OtaState::ACTIVATING, StepResultCode::FAIL, OtaState::FAILED);

        set_transition(OtaState::WAITING_FOR_RECONNECT, StepResultCode::DONE, OtaState::TRIGGER_INTERVIEW);
        set_transition(OtaState::WAITING_FOR_RECONNECT, StepResultCode::FAIL, OtaState::FAILED);

        set_transition(OtaState::TRIGGER_INTERVIEW, StepResultCode::DONE, OtaState::TRANSFER_DONE);
        set_transition(OtaState::TRIGGER_INTERVIEW, StepResultCode::FAIL, OtaState::FAILED);
    }

    void OtaStateMachine::register_steps()
    {
        register_step(OtaState::TRANSFER_DONE, std::make_unique<OtaStepTransferDone>());
        register_step(OtaState::FAILED, std::make_unique<OtaStepFailed>());
        register_step(OtaState::START_UPLOAD, std::make_unique<OtaStepStartUpload>());
        register_step(OtaState::UPLOAD_PREPARE_TRANSFER, std::make_unique<OtaStepUploadPrepare>());
        register_step(OtaState::UPLOAD_DELIVER_FIRMWARE_CHUNKS, std::make_unique<OtaStepDeliverRequestedFirmwareChunks>());
        register_step(OtaState::UPLOAD_PUBLISH_TRANSFER_PROGRESS, std::make_unique<OtaStepPublishTransferProgress>());
        register_step(OtaState::UPLOAD_RECORD_PENDING_TRANSFER_ABORT, std::make_unique<OtaStepRecordPendingTransferAbort>());
        register_step(OtaState::UPLOAD_PROCESS_DEVICE_OUTCOME, std::make_unique<OtaStepProcessDeviceTransferOutcome>());
        register_step(OtaState::WAITING_FOR_ACTIVATION, std::make_unique<OtaStepWaitingForActivation>());
        register_step(OtaState::ACTIVATING, std::make_unique<OtaStepActivating>());
        register_step(OtaState::WAITING_FOR_RECONNECT, std::make_unique<OtaStepWaitingForReconnect>());
        register_step(OtaState::TRIGGER_INTERVIEW, std::make_unique<OtaStepTriggerInterview>());
    }

    OtaStateMachine::OtaStateMachine() : state_machine::state_machine_base<OtaState, OtaSession, ota_external_event_data>(OtaState::FAILED, LOG_TAG.data())
    {
        register_transitions();
        register_steps();
    }

    void OtaStateMachine::start_ota(zwave_node_id_t node_id, const std::string &image_name, bool wait_for_activation)
    {
        sessions.insert_or_assign(node_id, OtaSession(node_id));
        OtaSession &s                = sessions.at(node_id);
        s.upload.image_name          = image_name;
        s.upload.wait_for_activation = wait_for_activation;
        transition_to_state(s, OtaState::START_UPLOAD);
    }

    bool OtaStateMachine::is_active_transfer_in_progress() const
    {
        static constexpr OtaState active_states[] = {
          OtaState::START_UPLOAD,
          OtaState::UPLOAD_PREPARE_TRANSFER,
          OtaState::UPLOAD_DELIVER_FIRMWARE_CHUNKS,
          OtaState::UPLOAD_PUBLISH_TRANSFER_PROGRESS,
          OtaState::UPLOAD_RECORD_PENDING_TRANSFER_ABORT,
          OtaState::UPLOAD_PROCESS_DEVICE_OUTCOME,
        };
        for (const auto &[nid, s]: sessions) {
            for (OtaState state: active_states) {
                if (s.current_state == state) {
                    return true;
                }
            }
        }
        return false;
    }

    sl_status_t OtaStateMachine::process_event(const ota_external_event_data &event)
    {
        if (event.event == ota_external_event_t::MQTT_ABORT) {
            auto it = sessions.find(event.node_id);
            if (it == sessions.end()) {
                sl_log_error(LOG_TAG.data(), "MQTT_ABORT: no session for node %d", event.node_id);
                return SL_STATUS_NOT_FOUND;
            }
            transition_to_state(it->second, OtaState::UPLOAD_RECORD_PENDING_TRANSFER_ABORT);
            if (it->second.current_state == OtaState::TRANSFER_DONE || it->second.current_state == OtaState::FAILED) {
                sessions.erase(it);
            }
            return SL_STATUS_OK;
        }

        if (event.event == ota_external_event_t::MQTT_PROGRESS_REQUEST) {
            OtaSession *target = nullptr;
            if (event.node_id != 0) {
                auto it = sessions.find(event.node_id);
                if (it != sessions.end()) {
                    target = &it->second;
                }
            } else {
                for (auto &[nid, s]: sessions) {
                    if (s.upload_in_progress) {
                        target = &s;
                        break;
                    }
                }
            }
            if (target == nullptr || !target->upload_in_progress) {
                return SL_STATUS_OK;
            }
            transition_to_state(*target, OtaState::UPLOAD_PUBLISH_TRANSFER_PROGRESS);
            return SL_STATUS_OK;
        }

        if (event.event == ota_external_event_t::MQTT_START_UPLOAD) {
            const auto *start_payload = std::any_cast<StartFirmwareUploadPayload>(&event.payload);
            if (start_payload == nullptr) {
                sl_log_error(LOG_TAG.data(), "MQTT_START_UPLOAD: payload type mismatch");
                return SL_STATUS_FAIL;
            }

            if (sessions.contains(start_payload->node_id)) {
                sl_log_warning(LOG_TAG.data(), "Node %d already has an active OTA session, rejecting new request", start_payload->node_id);
                nlohmann::json report;
                report[key::NODE_ID] = start_payload->node_id;
                report[key::STATUS]  = status::ERROR;
                report[key::REASON]  = reason::UPDATE_ALREADY_IN_PROGRESS;
                OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_START_FIRMWARE_UPLOAD_REPORT_TOPIC, report.dump(), false);
                return SL_STATUS_OK;
            }

            if (is_active_transfer_in_progress()) {
                sl_log_warning(LOG_TAG.data(),
                               "An active firmware transfer is already in progress, "
                               "rejecting new request for node %d",
                               start_payload->node_id);
                nlohmann::json report;
                report[key::NODE_ID] = start_payload->node_id;
                report[key::STATUS]  = status::ERROR;
                report[key::REASON]  = reason::UPDATE_ALREADY_IN_PROGRESS;
                OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_START_FIRMWARE_UPLOAD_REPORT_TOPIC, report.dump(), false);
                return SL_STATUS_OK;
            }

            start_ota(start_payload->node_id, start_payload->image_name, start_payload->wait_for_activation);
            auto post_it = sessions.find(start_payload->node_id);
            if (post_it != sessions.end() && (post_it->second.current_state == OtaState::TRANSFER_DONE || post_it->second.current_state == OtaState::FAILED)) {
                sessions.erase(post_it);
            }
            return SL_STATUS_OK;
        }

        // All remaining events are routed to the session for event.node_id.
        auto it = sessions.find(event.node_id);
        if (it == sessions.end()) {
            // Device-originated firmware reports (e.g. the Firmware MD Report fetched
            // during every device interview) arrive with no active OTA session and are
            // simply ignored.
            if (event.event == ota_external_event_t::MQTT_ACTIVATE) {
                sl_log_warning(LOG_TAG.data(), "No session for node %d, ignoring event %s", event.node_id, to_string(event.event));
            } else {
                sl_log_debug(LOG_TAG.data(), "No session for node %d, ignoring event %s", event.node_id, to_string(event.event));
            }
            return SL_STATUS_INVALID_STATE;
        }
        OtaSession &session = it->second;

        if (event.event == ota_external_event_t::FIRMWARE_UPDATE_MD_STATUS_REPORT_RECEIVED && session.current_state == OtaState::UPLOAD_DELIVER_FIRMWARE_CHUNKS) {
            session.upload_in_progress = false;
            transition_to_state(session, OtaState::UPLOAD_PROCESS_DEVICE_OUTCOME);
        }

        auto *base_step = get_step(session.current_state);
        if (base_step == nullptr) {
            sl_log_error(LOG_TAG.data(), "No step registered for state %d (node %d)", static_cast<int>(session.current_state), session.node_id);
            return SL_STATUS_INVALID_STATE;
        }

        auto *step             = static_cast<OtaStep *>(base_step);
        std::string state_name = step->name();
        sl_log_debug(LOG_TAG.data(), "Processing event %s for node %d in state %s", to_string(event.event), session.node_id, state_name.c_str());

        if (!step->handles_external_event(event.event)) {
            return SL_STATUS_INVALID_STATE;
        }

        state_machine::StepResult result = step->handle_event(session, std::make_optional(event));

        // UPLOAD_PROCESS_DEVICE_OUTCOME has three successful exit paths distinguished by
        // session.transfer_outcome; route them explicitly instead of a single table entry.
        if (session.current_state == OtaState::UPLOAD_PROCESS_DEVICE_OUTCOME && result.code == state_machine::StepResultCode::DONE) {
            switch (session.transfer_outcome) {
                case OtaTransferOutcome::SUCCESS:
                    transition_to_state(session, OtaState::WAITING_FOR_RECONNECT);
                    break;
                case OtaTransferOutcome::WAIT_FOR_ACTIVATION:
                    transition_to_state(session, OtaState::WAITING_FOR_ACTIVATION);
                    break;
                case OtaTransferOutcome::STORED_NO_RESTART:
                    transition_to_state(session, OtaState::TRANSFER_DONE);
                    break;
                default:
                    sl_log_error(LOG_TAG.data(), "Unexpected transfer_outcome after UPLOAD_PROCESS_DEVICE_OUTCOME");
                    transition_to_state(session, OtaState::FAILED);
                    break;
            }
            const OtaState post_state = session.current_state;
            if (post_state == OtaState::TRANSFER_DONE || post_state == OtaState::FAILED) {
                sessions.erase(event.node_id);
            }
            return result.status;
        }

        this->apply_transition(session, result);

        const OtaState post_state = session.current_state;
        if (post_state == OtaState::TRANSFER_DONE || post_state == OtaState::FAILED) {
            sessions.erase(event.node_id);
        }

        return result.status;
    }

}  // namespace ota
