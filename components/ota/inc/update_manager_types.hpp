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

#ifndef OTA_UPDATE_MANAGER_TYPES_HPP
#define OTA_UPDATE_MANAGER_TYPES_HPP

#include "zwave_node_id_definitions.h"
#include "attribute.hpp"
#include "command_class_firmware_update_md_types.hpp"
#include "ota_external_event_types.hpp"

#include <any>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ota
{

    /**
     * @brief Firmware Update MD Request Report status values (SDS13782, Table 3.69).
     */
    enum class FirmwareUpdateMdRequestReportStatus : uint8_t {
        INVALID_COMBO           = 0x00,
        REQUIRES_AUTHENTICATION = 0x01,
        INVALID_FRAGMENT_SIZE   = 0x02,
        NOT_DOWNLOADABLE        = 0x03,
        INVALID_HARDWARE_VER    = 0x04,
        VALID_COMBO             = 0xFF,
    };

    /**
     * @brief Firmware Update MD Status Report values (SDS13782, Table 3.93).
     */
    enum class FirmwareUpdateMdStatusReport : uint8_t {
        CHECKSUM_ERROR             = 0x00,
        UNABLE_TO_RECEIVE          = 0x01,
        MANUFACTURER_ID_MISMATCH   = 0x02,
        FIRMWARE_ID_MISMATCH       = 0x03,
        FIRMWARE_TARGET_MISMATCH   = 0x04,
        INVALID_FILE_HEADER_INFO   = 0x05,
        INVALID_FILE_HEADER_FORMAT = 0x06,
        INSUFFICIENT_MEMORY        = 0x07,
        HARDWARE_VERSION_MISMATCH  = 0x08,
        WAIT_FOR_ACTIVATION        = 0xFD,
        STORED_NO_RESTART          = 0xFE,
        SUCCESS                    = 0xFF,
    };

    /**
     * @brief States of the OTA firmware manager state machine.
     */
    enum class OtaState {
        /// Idle terminal state; session is cleared when entered.
        TRANSFER_DONE,
        START_UPLOAD,  ///< Firmware Update MD Request Get
        /// Load image into session and pause resolution; then UPLOAD_DELIVER_FIRMWARE_CHUNKS.
        UPLOAD_PREPARE_TRANSFER,
        /// Home state for the device-driven MD Get / MD Report loop (see dedicated steps for MQTT-side effects).
        UPLOAD_DELIVER_FIRMWARE_CHUNKS,
        /// MQTT: publish current byte progress to Progress/Report, then return to deliver state.
        UPLOAD_PUBLISH_TRANSFER_PROGRESS,
        /// MQTT: set abort_requested; next MD Get uses corrupted last frame per spec.
        UPLOAD_RECORD_PENDING_TRANSFER_ABORT,
        /// Z-Wave: Firmware Update MD Status Report (transfer outcome).
        UPLOAD_PROCESS_DEVICE_OUTCOME,
        /// Transfer complete with status 0xFD; waiting for MQTT Activate command before applying firmware.
        WAITING_FOR_ACTIVATION,
        /// Firmware Update Activation Set sent; waiting for Activation Status Report 0xFF.
        ACTIVATING,
        /// Activation confirmed or immediate restart; waiting WaitTime then NOP probe before interview.
        WAITING_FOR_RECONNECT,
        /// Node confirmed reachable; trigger re-interview of the updated node.
        TRIGGER_INTERVIEW,
        FAILED
    };

    /**
     * @brief Outcome of the Firmware Update MD Status Report, used to route
     *        post-transfer states correctly.
     */
    enum class OtaTransferOutcome {
        NONE,
        SUCCESS,              ///< 0xFF — immediate activation, wait for reconnect then interview.
        WAIT_FOR_ACTIVATION,  ///< 0xFD — delayed activation, wait for MQTT Activate command.
        STORED_NO_RESTART,    ///< 0xFE — stored only, no restart or interview needed.
    };

    /**
     * @brief MQTT StartFirmwareUpload parameters cached for the session.
     */
    struct OtaUploadRequest {
            std::string image_name;
            bool wait_for_activation = false;
            uint8_t firmware_target  = 0;
    };

    /**
     * @brief Image transfer counters and abort state (upload steps, completion reports).
     */
    struct OtaTransferProgress {
            uint32_t image_size        = 0;
            uint32_t bytes_transferred = 0;
            bool abort_requested       = false;
            uint16_t firmware_checksum = 0;  ///< CRC16 over image bytes (Z-Wave transfer)
    };

    /**
     * @brief Firmware Update MD values from attribute store / Firmware MD Report (OtaStepStartUpload).
     */
    struct OtaFirmwareMdMetadata {
            uint16_t manufacturer_id           = 0;
            uint16_t firmware_id               = 0;
            uint16_t max_fragment_size         = 0;
            uint8_t hardware_version           = 0;
            uint8_t number_of_firmware_targets = 0;
            bool firmware_upgradable           = false;
    };

    /**
     * @brief Context for a single OTA firmware transfer (mirrors InterviewSession layout:
     *        identity → state → attribute store node → grouped step data).
     *
     * state_machine_base requires a public `current_state` field of type OtaState.
     */
    struct OtaSession {
            zwave_node_id_t node_id = 0;
            OtaState current_state  = OtaState::TRANSFER_DONE;

            /// True during the MD Get / Report image transfer; cleared when the MD Status Report
            /// is received (outcome step) or the transfer fails. Drives return state after MQTT progress report.
            bool upload_in_progress = false;

            /// True when OtaStepUploadPrepare has paused attribute resolution and it has not
            /// yet been resumed. Guards against double-resume if the activation path resumes
            /// resolution early (WAITING_FOR_ACTIVATION) and TransferDone/Failed also try to resume.
            bool resolution_paused = false;

            attribute_store::attribute endpoint_node;

            OtaUploadRequest upload;
            OtaTransferProgress transfer;
            OtaFirmwareMdMetadata firmware_md;

            /// Cached firmware image bytes for the active transfer (shared across upload substeps).
            std::vector<uint8_t> firmware_image_cache;

            /// WaitTime (seconds) reported by MD Status Report or Activation Status Report;
            /// carried into WAITING_FOR_RECONNECT to delay the NOP probe.
            uint16_t wait_time = 0;

            /// Routing signal set by UPLOAD_PROCESS_DEVICE_OUTCOME to distinguish post-transfer paths.
            OtaTransferOutcome transfer_outcome = OtaTransferOutcome::NONE;

            OtaSession(zwave_node_id_t node_id) :
              node_id(node_id), current_state(OtaState::TRANSFER_DONE), upload_in_progress(false), resolution_paused(false), endpoint_node(ATTRIBUTE_STORE_INVALID_NODE), upload(), transfer(), firmware_md(), firmware_image_cache(), wait_time(0), transfer_outcome(OtaTransferOutcome::NONE)
            {}
    };

    // ---- Z-Wave CC report payload (received via component_connector) ----

    /**
     * @brief Payload for Z-Wave report events routed from the Firmware Update MD CC
     *        through component_connector into the OTA state machine.
     */
    struct ZwaveReportPayload {
            zwave_node_id_t node_id = 0;
            attribute_store::attribute endpoint_node;
            zwave_command_class::command_class_firmware_update_md_types::command_class_firmware_update_md_attribute_map_t attribute_map;
    };

    // ---- MQTT command payload structs ----

    struct StartFirmwareUploadPayload {
            zwave_node_id_t node_id = 0;
            std::string image_name;
            bool wait_for_activation = false;
    };

    struct ActivatePayload {
            zwave_node_id_t node_id = 0;
    };

    struct UploadImagePayload {
            std::string image_name;
            std::vector<uint8_t> data;
    };

    struct RemoveImagePayload {
            std::string image_name;
    };

}  // namespace ota

#endif  // OTA_UPDATE_MANAGER_TYPES_HPP
