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

#ifndef COMMAND_CLASS_FIRMWARE_UPDATE_MD_EVENTS_H
#define COMMAND_CLASS_FIRMWARE_UPDATE_MD_EVENTS_H

#include <stdint.h>

enum class command_class_firmware_update_md_events_t : uint32_t {
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_BASE_EVENT = (122 << 8),
    /// Fired by components or during interview to request Firmware Meta Data Get
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_FIRMWARE_MD_GET,
    /// Fired by components to request Firmware Update MD Request Get
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_REQUEST_GET,
    /// Fired by components to send a Firmware Update MD Report (firmware data chunk to device)
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_REPORT,
    /// Fired after Firmware Meta Data Report has been parsed and stored
    FIRMWARE_MD_REPORT_PARSED,
    /// Fired after Firmware Update MD Request Report has been parsed and stored
    FIRMWARE_UPDATE_MD_REQUEST_REPORT_PARSED,
    /// Fired after Firmware Update MD Get has been parsed (device requesting a chunk)
    FIRMWARE_UPDATE_MD_GET_PARSED,
    /// Fired after Firmware Update MD Status Report has been parsed and stored
    FIRMWARE_UPDATE_MD_STATUS_REPORT_PARSED,
    /// Fired after Firmware Update Activation Status Report has been parsed and stored
    FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_PARSED,
    /// Fired after Firmware Update MD Prepare Report has been parsed and stored
    FIRMWARE_UPDATE_MD_PREPARE_REPORT_PARSED,
    /// Fired by components to request a Firmware Update Activation Set command
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_ACTIVATION_SET,
};

#endif  // COMMAND_CLASS_FIRMWARE_UPDATE_MD_EVENTS_H
