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

#include "steps/ota_step_start_upload.hpp"
#include "mqtt_api_base.hpp"
#include "ota_mqtt_api.hpp"
#include "ota_image_store.hpp"
#include "ota_mqtt_constants.hpp"
#include "command_class_firmware_update_md_generated_types.hpp"
#include "command_class_firmware_update_md_events.hpp"
#include "command_class_firmware_update_md_types.hpp"
#include "log.h"
#include "nlohmann/json.hpp"

#include "component_connector.hpp"

#include "attribute.hpp"
#include "zwave_utils.h"

#include "zwave_crc16.h"

#include <string_view>
#include <variant>
#include <optional>

namespace ota
{

    using namespace mqtt_constants;

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_start_upload";

    // ========================= Helpers =========================

    static std::string_view reject_reason_from_status(FirmwareUpdateMdRequestReportStatus status)
    {
        switch (status) {
            case FirmwareUpdateMdRequestReportStatus::INVALID_COMBO:
                return reason::INVALID_COMBINATION;
            case FirmwareUpdateMdRequestReportStatus::REQUIRES_AUTHENTICATION:
                return reason::REQUIRES_AUTHENTICATION;
            case FirmwareUpdateMdRequestReportStatus::INVALID_FRAGMENT_SIZE:
                return reason::INVALID_FRAGMENT_SIZE;
            case FirmwareUpdateMdRequestReportStatus::NOT_DOWNLOADABLE:
                return reason::NOT_DOWNLOADABLE;
            case FirmwareUpdateMdRequestReportStatus::INVALID_HARDWARE_VER:
                return reason::INVALID_HARDWARE_VERSION;
            default:
                return reason::UNKNOWN;
        }
    }

    static std::optional<uint8_t> extract_status(const ZwaveReportPayload &report)
    {
        const auto &attr_map = report.attribute_map;

        auto it = attr_map.find("status");
        if (it == attr_map.end()) {
            return std::nullopt;
        }

        if (!std::holds_alternative<uint8_t>(it->second)) {
            return std::nullopt;
        }

        return std::get<uint8_t>(it->second);
    }

    // ========================= Class =========================

    std::string OtaStepStartUpload::name() const
    {
        return "OTA Step Start Upload";
    }

    bool OtaStepStartUpload::handles_external_event(ota_external_event_t event_type) const
    {
        return event_type == ota_external_event_t::FIRMWARE_UPDATE_MD_REQUEST_REPORT_RECEIVED || event_type == ota_external_event_t::MQTT_ABORT;
    }

    void OtaStepStartUpload::publish_report(const OtaSession &session, std::string_view report_status, std::string_view report_reason)
    {
        nlohmann::json report;
        report[key::NODE_ID]    = session.node_id;
        report[key::IMAGE_NAME] = session.upload.image_name;
        report[key::STATUS]     = report_status;

        if (!report_reason.empty()) {
            report[key::REASON] = report_reason;
        }

        OTAMqttApi::publish_report(OTAMqttApi::MQTT_API_OTA_START_FIRMWARE_UPLOAD_REPORT_TOPIC, report.dump(), false);
    }

    // ========================= on_enter =========================

    StepResult OtaStepStartUpload::on_enter(OtaSession &session)
    {
        attribute_store_node_t ep_node = zwave_get_endpoint_node(session.node_id, 0);

        if (ep_node == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_error(LOG_TAG.data(), "Node %d is invalid", session.node_id);
            publish_report(session, status::ERROR, reason::INVALID_NODE);
            return fail();
        }

        session.endpoint_node = attribute_store::attribute(ep_node);

        attribute_store::attribute endpoint(ep_node);
        attribute_store::attribute group = endpoint.child_by_type(static_cast<attribute_store_type_t>(zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_group_attributes_t::FIRMWARE_MD_REPORT_GROUP));

        if (!group.is_valid()) {
            sl_log_error(LOG_TAG.data(), "Firmware Update Meta Data is not supported on node %d or interview is incomplete", session.node_id);

            publish_report(session, status::ERROR, reason::UNSUPPORTED_FEATURE);
            return fail();
        }

        session.firmware_md.manufacturer_id
          = group.child_by_type(static_cast<attribute_store_type_t>(zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_group_attributes_t::manufacturer_id)).reported<zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_manufacturer_id_t>();
        session.firmware_md.firmware_id
          = group.child_by_type(static_cast<attribute_store_type_t>(zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_group_attributes_t::firmware_0_id)).reported<zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_firmware_0_id_t>();
        session.firmware_md.max_fragment_size
          = group.child_by_type(static_cast<attribute_store_type_t>(zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_group_attributes_t::max_fragment_size)).reported<zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_max_fragment_size_t>();
        session.firmware_md.hardware_version
          = group.child_by_type(static_cast<attribute_store_type_t>(zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_group_attributes_t::hardware_version)).reported<zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_hardware_version_t>();
        session.firmware_md.number_of_firmware_targets = group.child_by_type(static_cast<attribute_store_type_t>(zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_group_attributes_t::number_of_firmware_targets))
                                                           .reported<zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_number_of_firmware_targets_t>();

        const uint8_t upgradable_raw = group.child_by_type(static_cast<attribute_store_type_t>(zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_group_attributes_t::firmware_upgradable))
                                         .reported<zwave_command_class::command_class_firmware_update_md_types::firmware_md_report_firmware_upgradable_t>();

        session.firmware_md.firmware_upgradable = (upgradable_raw == 0xFF);

        sl_log_info(LOG_TAG.data(),
                    "Node %d: manufacturer=0x%04X, firmware_id=0x%04X, "
                    "max_fragment=%u, upgradable=%s, targets=%u",
                    session.node_id,
                    session.firmware_md.manufacturer_id,
                    session.firmware_md.firmware_id,
                    session.firmware_md.max_fragment_size,
                    session.firmware_md.firmware_upgradable ? "yes" : "no",
                    session.firmware_md.number_of_firmware_targets);

        if (!session.firmware_md.firmware_upgradable) {
            sl_log_warning(LOG_TAG.data(), "Node %d firmware is NOT upgradable, aborting", session.node_id);

            publish_report(session, status::ERROR, reason::FIRMWARE_NOT_UPGRADABLE);
            return fail();
        }

        auto image_data = ota::OTAImageStore::get_image(session.upload.image_name);
        if (image_data.empty()) {
            sl_log_error(LOG_TAG.data(), "Image '%s' not found in store", session.upload.image_name.c_str());

            publish_report(session, status::ERROR, reason::IMAGE_NOT_FOUND);
            return fail();
        }

        session.transfer.image_size = static_cast<uint32_t>(image_data.size());

        session.transfer.firmware_checksum = zwave_crc16(CRC16_INIT_VALUE, image_data.data(), image_data.size());

        sl_log_info(LOG_TAG.data(), "Loaded image '%s' (%u bytes), checksum=0x%04X", session.upload.image_name.c_str(), session.transfer.image_size, session.transfer.firmware_checksum);

        return stay();
    }

    // ========================= Internal handler =========================

    StepResult OtaStepStartUpload::handle_request_report(OtaSession &session, const ZwaveReportPayload &report)
    {
        auto status_opt = extract_status(report);

        if (!status_opt.has_value()) {
            sl_log_error(LOG_TAG.data(), "Missing or invalid status in Request Report");
            return fail();
        }

        auto status_enum = static_cast<FirmwareUpdateMdRequestReportStatus>(*status_opt);

        if (status_enum == FirmwareUpdateMdRequestReportStatus::VALID_COMBO) {
            sl_log_info(LOG_TAG.data(), "Node %d accepted the firmware update request", session.node_id);

            publish_report(session, status::ACCEPTED);
            return done();
        }

        auto reject_reason = reject_reason_from_status(status_enum);

        sl_log_warning(LOG_TAG.data(), "Node %d rejected — status=0x%02X (%.*s)", session.node_id, *status_opt, static_cast<int>(reject_reason.size()), reject_reason.data());

        publish_report(session, status::REJECTED, reject_reason);
        return fail();
    }

    // ========================= handle_event =========================

    StepResult OtaStepStartUpload::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        // Initial entry
        if (!event.has_value()) {
            sl_log_info(LOG_TAG.data(), "Sending Firmware Update MD Request Get to node %d — image='%s' (%u bytes)", session.node_id, session.upload.image_name.c_str(), session.transfer.image_size);

            if (!session.endpoint_node.is_valid()) {
                sl_log_error(LOG_TAG.data(), "Invalid endpoint node for node %d", session.node_id);
                return fail();
            }

            zwave_command_class::command_class_firmware_update_md_types::command_class_firmware_update_md_request_get_payload_t payload;

            payload.endpoint_node    = session.endpoint_node;
            payload.manufacturer_id  = session.firmware_md.manufacturer_id;
            payload.firmware_id      = session.firmware_md.firmware_id;
            payload.checksum         = session.transfer.firmware_checksum;
            payload.firmware_target  = session.upload.firmware_target;
            payload.fragment_size    = session.firmware_md.max_fragment_size;
            payload.activation       = session.upload.wait_for_activation;
            payload.hardware_version = session.firmware_md.hardware_version;

            component_connector connector;
            connector.fire_event(static_cast<uint32_t>(command_class_firmware_update_md_events_t::COMMAND_CLASS_FIRMWARE_UPDATE_MD_REQUEST_GET), payload);

            return stay();
        }

        switch (event->event) {
            case ota_external_event_t::FIRMWARE_UPDATE_MD_REQUEST_REPORT_RECEIVED: {
                if (!event->payload.has_value()) {
                    sl_log_error(LOG_TAG.data(), "Missing payload for Request Report");
                    return fail();
                }

                const auto *report = std::any_cast<ZwaveReportPayload>(&event->payload);

                if (report == nullptr) {
                    sl_log_error(LOG_TAG.data(), "Bad payload type for Request Report");
                    return fail();
                }

                return handle_request_report(session, *report);
            }

            case ota_external_event_t::MQTT_ABORT:
                sl_log_info(LOG_TAG.data(), "Abort received for node %d (no transfer started)", session.node_id);

                publish_report(session, status::ABORTED);
                return skip();

            default:
                return stay();
        }
    }

}  // namespace ota