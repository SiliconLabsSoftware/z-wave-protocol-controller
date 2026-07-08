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

#include "update_manager.hpp"

// Component connector for subscribing to CC report events
#include "component_connector.hpp"
#include "command_class_firmware_update_md_events.hpp"
#include "command_class_firmware_update_md_types.hpp"

// Attribute store helpers for extracting node_id
#include "zpc_attribute_store_network_helper.h"

#include "log.h"

#include <string_view>

namespace ota
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_update_manager";

    update_manager::update_manager() : threading::threading("OTA Update Manager"), mqttApi(event_queue)
    {
        register_event_handlers();
    }

    sl_status_t update_manager::initialize()
    {
        mqttApi.setup_mqtt_api();
        sl_log_info(LOG_TAG.data(), "OTA Update Manager initialized");
        return SL_STATUS_OK;
    }

    int update_manager::shutdown()
    {
        stop();
        sl_log_info(LOG_TAG.data(), "OTA Update Manager shut down");
        return 0;
    }

    std::string update_manager::name() const
    {
        return "OTA Update Manager";
    }

    void update_manager::run()
    {
        std::optional<ota_external_event_data> ev = event_queue.pop(50);

        if (ev.has_value()) {
            (void)stateMachine.process_event(ev.value());
        }
    }

    sl_status_t update_manager::queue_firmware_update_md_report(ota_external_event_t event_kind, const zwave_command_class::command_class_firmware_update_md_types::component_connector_firmware_update_md_report_payload_t &cc_payload)
    {
        zwave_node_id_t node_id = 0;
        sl_status_t status      = attribute_store_network_helper_get_node_id_from_node(cc_payload.endpoint_node, &node_id);
        if (status != SL_STATUS_OK) {
            return status;
        }

        ZwaveReportPayload payload;
        payload.node_id       = node_id;
        payload.endpoint_node = cc_payload.endpoint_node;
        payload.attribute_map = cc_payload.attribute_map;

        queue_event(event_kind, payload);
        return SL_STATUS_OK;
    }

    void update_manager::register_event_handlers()
    {
        using cc_events_t  = command_class_firmware_update_md_events_t;
        using cc_payload_t = zwave_command_class::command_class_firmware_update_md_types::component_connector_firmware_update_md_report_payload_t;

        component_connector connector;

        // Firmware Meta Data Report → FIRMWARE_MD_REPORT_RECEIVED
        connector.connect_typed<cc_events_t, cc_payload_t>(cc_events_t::FIRMWARE_MD_REPORT_PARSED, [this](const cc_payload_t &p) { return this->queue_firmware_update_md_report(ota_external_event_t::FIRMWARE_MD_REPORT_RECEIVED, p); });

        // Firmware Update MD Request Report → FIRMWARE_UPDATE_MD_REQUEST_REPORT_RECEIVED
        connector.connect_typed<cc_events_t, cc_payload_t>(cc_events_t::FIRMWARE_UPDATE_MD_REQUEST_REPORT_PARSED, [this](const cc_payload_t &p) { return this->queue_firmware_update_md_report(ota_external_event_t::FIRMWARE_UPDATE_MD_REQUEST_REPORT_RECEIVED, p); });

        // Firmware Update MD Get (device requesting chunk) → FIRMWARE_UPDATE_MD_GET_RECEIVED
        connector.connect_typed<cc_events_t, cc_payload_t>(cc_events_t::FIRMWARE_UPDATE_MD_GET_PARSED, [this](const cc_payload_t &p) { return this->queue_firmware_update_md_report(ota_external_event_t::FIRMWARE_UPDATE_MD_GET_RECEIVED, p); });

        // Firmware Update MD Status Report → FIRMWARE_UPDATE_MD_STATUS_REPORT_RECEIVED
        connector.connect_typed<cc_events_t, cc_payload_t>(cc_events_t::FIRMWARE_UPDATE_MD_STATUS_REPORT_PARSED, [this](const cc_payload_t &p) { return this->queue_firmware_update_md_report(ota_external_event_t::FIRMWARE_UPDATE_MD_STATUS_REPORT_RECEIVED, p); });

        // Firmware Update Activation Status Report → FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT
        connector.connect_typed<cc_events_t, cc_payload_t>(cc_events_t::FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_PARSED, [this](const cc_payload_t &p) { return this->queue_firmware_update_md_report(ota_external_event_t::FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT, p); });

        sl_log_debug(LOG_TAG.data(), "Registered event handlers for Firmware Update MD CC reports");
    }

    void update_manager::queue_event(ota_external_event_t event_kind, const std::any &payload)
    {
        ota_external_event_data ev;
        ev.event   = event_kind;
        ev.payload = payload;
        // Extract node_id from the payload when it's a ZwaveReportPayload
        if (const auto *zw = std::any_cast<ZwaveReportPayload>(&payload)) {
            ev.node_id = zw->node_id;
        }
        event_queue.push(ev);
    }

}  // namespace ota
