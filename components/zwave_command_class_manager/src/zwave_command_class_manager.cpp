/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
// Includes from this component
#include "zwave_command_class_manager.h"

// Includes from other components
#include "zwave_controller.h"
#include "zwave_controller_callbacks.h"
#include "zwave_controller_storage.h"
#include "zwave_controller_keyset.h"
#include "zwave_controller_utils.h"
#include "zwave_security_validation.h"
#include "zwave_rx.h"
#include "log.h"
#include "zwave_network_management.h"  // for zwave_network_management_get_home_id / get_node_id

// Interface includes
#include "ZW_classcmd.h"
#include "zwave_command_type.hpp"
#include "zwave_command_class_indices.h"  // COMMAND_CLASS_CONTROL_MARK

// Generic includes
#include <iomanip>
#include <sstream>
#include <vector>
#include <set>
#include <assert.h>
#include <cstdio>
#include <algorithm>

namespace
{
    /// Setup Log ID
    constexpr char LOG_TAG[] = "zwave_command_class_manager";
}  // namespace

const zwave_controller_callbacks_t zwave_command_class_manager::zwave_command_handler_callbacks = {
  .on_new_network_entered        = zwave_command_class_manager::zwave_command_handler_on_new_network_entered,
  .on_application_frame_received = zwave_command_class_manager::zwave_command_handler_on_frame_received,
};

///////////////////////////////////////////////////////////////////////////////
// Utils
///////////////////////////////////////////////////////////////////////////////
namespace
{

    void command_class_list_to_buffer(const std::set<uint16_t> &source_list, const std::set<uint16_t> &additional_controlled_list, std::vector<uint8_t> &destination_buffer)
    {
        // Clear our static byte array before copying
        destination_buffer.clear();

        // Special treatment for Z-Wave Plus Info CC, it comes first:
        auto it = std::find(source_list.begin(), source_list.end(), COMMAND_CLASS_ZWAVEPLUS_INFO);
        if (it != source_list.end()) {
            destination_buffer.push_back(COMMAND_CLASS_ZWAVEPLUS_INFO);
        }

        // Now copy the other Command Classes one by one.
        for (auto cc: source_list) {
            if (destination_buffer.size() >= ZWAVE_MAX_FRAME_SIZE) {
                sl_log_warning(LOG_TAG,
                               "NIF has grown too large to advertise all supported "
                               "Command Classes. Omitting Command Class 0x%02X");
                return;
            }
            if (cc == COMMAND_CLASS_ZWAVEPLUS_INFO) {
                continue;
            }

            if (cc <= 0xF1) {
                destination_buffer.push_back(cc & 0xFF);
            } else {
                // It's an extended CC, we copy MSB then LSB
                destination_buffer.push_back(cc >> 8);
                destination_buffer.push_back(cc & 0xFF);
            }
        }

        // Now add the controlled CC at the end:
        if (!additional_controlled_list.empty()) {
            destination_buffer.push_back(COMMAND_CLASS_CONTROL_MARK);
        }
        for (auto cc: additional_controlled_list) {
            if (destination_buffer.size() >= ZWAVE_MAX_FRAME_SIZE) {
                sl_log_info(LOG_TAG,
                            "NIF has grown too large to advertise all controlled "
                            "Command Classes. Omitting Command Class 0x%02X");
                return;
            }
            // Never put Basic in the NIF.
            if (cc == COMMAND_CLASS_BASIC) {
                continue;
            }
            if (cc <= 0xF1) {
                destination_buffer.push_back(cc & 0xFF);
            } else {
                // It's an extended CC, we copy MSB then LSB
                destination_buffer.push_back(cc >> 8);
                destination_buffer.push_back(cc & 0xFF);
            }
        }
    }

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// Callback functions
///////////////////////////////////////////////////////////////////////////////
void zwave_command_class_manager::zwave_command_handler_on_new_network_entered(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type)
{
    sl_log_debug(LOG_TAG, "Setting NIF for HomeID: %08X NodeID: %03u\n", home_id, node_id);

    std::set<zwave_command_class_t> secure_supported_command_classes;
    std::set<zwave_command_class_t> nonsecure_supported_command_classes;

    // Array buffers that we pass on to the Z-Wave API / S2.
    // They keep the uint16_t CC identifiers converted to uint8_t
    // when we need to set our NIFs
    std::vector<uint8_t> non_secure_command_class_buffer;
    std::vector<uint8_t> secure_command_class_buffer;

    /// Add Security 0 only if ZPC has the S0 key
    if ((granted_keys & ZWAVE_CONTROLLER_S0_KEY) != 0) {
        nonsecure_supported_command_classes.insert(COMMAND_CLASS_SECURITY);
    }

    /// Update the highest scheme of ZPC that we cache locally
    zpc_highest_scheme = zwave_controller_get_highest_encapsulation(granted_keys);

    /// Add only the supported CCs to the NIF. Control-only CCs are not advertised.
    for (auto &cc_handler: command_handler_list) {
        if (cc_handler->has_support_handler()) {
            auto minimal_scheme                   = cc_handler->supported_handler_minimal_scheme();
            bool supports_frame_at_security_level = false;

            // FIXME
            // Taken for supports_frame_at_security_level
            // Might need a refactor later it seems suspicious
            /// Old comment :
            /// Verify if that works if network scheme is non-secure.
            /// I don't think so.
            switch (minimal_scheme) {
                case ZWAVE_CONTROLLER_ENCAPSULATION_NONE:
                    supports_frame_at_security_level = true;
                    break;
                case ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME:
                    supports_frame_at_security_level = (zpc_highest_scheme == ZWAVE_CONTROLLER_ENCAPSULATION_NONE);
                    break;
                default:
                    supports_frame_at_security_level = (zwave_controller_encapsulation_scheme_greater_equal(ZWAVE_CONTROLLER_ENCAPSULATION_NONE, zpc_highest_scheme) && zwave_controller_encapsulation_scheme_greater_equal(ZWAVE_CONTROLLER_ENCAPSULATION_NONE, minimal_scheme));
                    break;
            }

            if (supports_frame_at_security_level) {
                nonsecure_supported_command_classes.insert(cc_handler->id());
            } else {
                // In the Application Command Class specification (2024B), under the Security 2 Commands Supported Report,
                // the specification defines the valid ranges for Command Class identifiers:
                // "A normal Command Class identifier MUST be one byte long in the range (0x20–0xEE)."

                // 0x01..0x1F → reserved for protocol
                // 0x20..0xFF → application command classes
                // 0xEF → special MARK
                // 0xF1.. → extended (2-byte CCs)
                if (cc_handler->id() > 0x20 && cc_handler->id() < 0xEF) {
                    secure_supported_command_classes.insert(cc_handler->id());
                } else {
                    sl_log_warning(LOG_TAG, "Command Class 0x%02X is not supported at security level %d", cc_handler->id(), zpc_highest_scheme);
                }
            }
        }
    }

    /// Set non-secure NIF (supported CCs only, no controlled-only CCs)
    std::set<zwave_command_class_t> empty_controlled_list;
    command_class_list_to_buffer(nonsecure_supported_command_classes, empty_controlled_list, non_secure_command_class_buffer);
    zwave_controller_set_application_nif((const uint8_t *)non_secure_command_class_buffer.data(), non_secure_command_class_buffer.size());

    /// Set secure NIF (supported CCs only)
    command_class_list_to_buffer(secure_supported_command_classes, empty_controlled_list, secure_command_class_buffer);

    zwave_controller_set_secure_application_nif((const uint8_t *)secure_command_class_buffer.data(), secure_command_class_buffer.size());
}

void zwave_command_class_manager::zwave_command_handler_on_frame_received(const zwave_controller_connection_info_t *connection_info, const zwave_rx_receive_options_t *rx_options, const uint8_t *frame_data, uint16_t frame_length)
{
    // Print out the frame dispatch
    std::stringstream message;
    message << "Dispatching incoming command (encapsulation " << int(connection_info->encapsulation) << ") from NodeID " << int(connection_info->remote.node_id) << ":" << int(connection_info->remote.endpoint_id) << " - [ ";

    for (uint16_t i = 0; i < frame_length; i++) {
        message << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(frame_data[i]) << " ";
    }
    message << "]";
    sl_log_debug(LOG_TAG, "%s", message.str().c_str());

    if (frame_length <= COMMAND_INDEX || frame_length >= ZWAVE_MAX_FRAME_SIZE) {
        sl_log_warning(LOG_TAG,
                       "Received frame length is invalid. Dropping frame. Expected "
                       "value between %d and %d, got %d",
                       COMMAND_INDEX,
                       ZWAVE_MAX_FRAME_SIZE,
                       frame_length);
        return;
    }
    // Dispatch and look at the status code
    sl_status_t status = zwave_command_class_manager::dispatch(connection_info, frame_data, frame_length);
    switch (status) {
        case SL_STATUS_OK:
            sl_log_debug(LOG_TAG, "Command from NodeID %d:%d was handled successfully.", connection_info->remote.node_id, connection_info->remote.endpoint_id);
            break;

        case SL_STATUS_FAIL:
            sl_log_debug(LOG_TAG,
                         "Command from NodeID %d:%d had an error during handling. "
                         "Not all parameters were accepted",
                         connection_info->remote.node_id,
                         connection_info->remote.endpoint_id);
            break;

        case SL_STATUS_BUSY:
            // This should not happen, or if it happens, we should be able to return
            // an application busy message or similar.
            sl_log_warning(LOG_TAG,
                           "Frame handler is busy and could not handle frame from "
                           "NodeID %d:%d correctly.",
                           connection_info->remote.node_id,
                           connection_info->remote.endpoint_id);
            break;

        case SL_STATUS_NOT_SUPPORTED:
            sl_log_debug(LOG_TAG,
                         "Command from NodeID %d:%d got rejected because it is not supported. "
                         "It was possibly also rejected due to security filtering",
                         connection_info->remote.node_id,
                         connection_info->remote.endpoint_id);
            break;

        default:
            sl_log_warning(LOG_TAG, "Command from NodeID %d:%d had an unexpected return status: 0x%04X\n", connection_info->remote.node_id, connection_info->remote.endpoint_id, status);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////
zwave_command_class::zwave_command_class_base *zwave_command_class_manager::get_command_class_handler(zwave_command_class_t command_class_id)
{
    auto cc_handler = std::find_if(command_handler_list.begin(), command_handler_list.end(), [&command_class_id](const auto &handler) { return handler->id() == command_class_id; });

    if (cc_handler != command_handler_list.end()) {
        return cc_handler->get();
    }

    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_manager::dispatch(const zwave_controller_connection_info_t *connection, const uint8_t *frame_data, uint16_t frame_length)
{
    sl_status_t rc           = SL_STATUS_NOT_SUPPORTED;
    uint8_t command_class_id = frame_data[0];

    // Check if this frame is a multicast get
    if (connection->local.is_multicast) {
        if (ZwaveCommandClassType::get_type(frame_data[0], frame_data[1]) == ZwaveCommandClassType::type_t::GET) {
            sl_log_debug(LOG_TAG, "Multicast get frame dropped");
            return SL_STATUS_NOT_SUPPORTED;
        }
    }

    auto *command_class_handle = get_command_class_handler(command_class_id);
    if (command_class_handle == nullptr) {
        sl_log_debug(LOG_TAG, "No handler for Command Class 0x%02X", command_class_id);
        return SL_STATUS_NOT_SUPPORTED;
    }

    // First try to dispatch the message to the control handler
    if (command_class_handle->manual_security_validation() || zwave_security_validation_is_security_valid_for_control(connection)) {
        // If no control handler is implement it will return SL_STATUS_NOT_SUPPORTED by design
        rc = command_class_handle->control_handler(connection, frame_data, frame_length);
    }

    // If we have a result from the control handler, we return it
    // Otherwise we continue to the support handler
    if (rc != SL_STATUS_NOT_SUPPORTED) {
        return rc;
    }

    // Support handler
    if (command_class_handle->manual_security_validation() || zwave_security_validation_is_security_valid_for_support(command_class_handle->supported_handler_minimal_scheme(), connection)) {
        // If no support handler is implement it will return SL_STATUS_NOT_SUPPORTED by design
        rc = command_class_handle->support_handler(connection, frame_data, frame_length);
    }

    return rc;
}

sl_status_t zwave_command_class_manager::init()
{
    /// We will assume all keys if the Z-Wave Controller storage can't find our own keys.
    zwave_keyset_t my_granted_keys = 0x87;
    if (SL_STATUS_OK != zwave_controller_storage_get_node_granted_keys(zwave_network_management_get_node_id(), &my_granted_keys)) {
        sl_log_warning(LOG_TAG, "Cannot read Z-Wave Controller's granted keys");
    };
    zwave_kex_fail_type_t kex_fail = ZWAVE_NETWORK_MANAGEMENT_KEX_FAIL_NONE;

    zwave_command_handler_on_new_network_entered(zwave_network_management_get_home_id(), zwave_network_management_get_node_id(), my_granted_keys, kex_fail);

    // Register on_frame_received and on_new_network_entered callbacks
    zwave_controller_register_callbacks(&zwave_command_handler_callbacks);

    return SL_STATUS_OK;
}

int zwave_command_class_manager::teardown()
{
    // Stop being notified of incoming frames and network changes
    command_handler_list.clear();
    zwave_controller_deregister_callbacks(&zwave_command_handler_callbacks);
    return 0;
}

sl_status_t zwave_command_class_manager::register_command_class(zwave_command_class::zwave_command_class_base *new_command_class_handler)
{
    auto *command_class_handler = get_command_class_handler(new_command_class_handler->id());
    if (command_class_handler != nullptr) {
        sl_log_critical(LOG_TAG, "Attempt to register duplicate command handler for CC 0x%0x", new_command_class_handler->id());
        return SL_STATUS_ALREADY_INITIALIZED;
    }

    command_handler_list.emplace_back(std::unique_ptr<zwave_command_class::zwave_command_class_base>(new_command_class_handler));

    // Control-only CCs must report version 0 in Version CC Reports.
    // The base constructor unconditionally populates supported_command_class_versions;
    // correct it here now that the vtable is fully resolved.
    if (!new_command_class_handler->has_support_handler()) {
        zwave_command_class::zwave_command_class_base::supported_command_class_versions[new_command_class_handler->id()] = 0;
    }

    return SL_STATUS_OK;
}

uint8_t zwave_command_class_manager::get_version(zwave_command_class_t command_class)
{
    auto *command_class_handler = get_command_class_handler(command_class);
    if (command_class_handler != nullptr) {
        // Per Z-Wave spec, only supported CCs report their version.
        // Control-only CCs must report version 0.
        if (!command_class_handler->has_support_handler()) {
            return 0;
        }
        return command_class_handler->supported_version();
    }

    // If the handler is not registered, we return 0.
    return 0;
}

bool zwave_command_class_manager::controls(zwave_command_class_t command_class)
{
    auto *handler = get_command_class_handler(command_class);
    if (handler == nullptr) {
        return false;
    }
    return handler->has_control_handler();
}

void zwave_command_class_manager::print_info(int fd)
{
    using namespace zwave_command_class;

    // Since we are handling a vector of unique pointer, we cannot copy the list
    // directly, we need to copy the pointers
    std::vector<zwave_command_class_base *> command_handler_list_alphabetic_sort;
    command_handler_list_alphabetic_sort.reserve(command_handler_list.size());
    for (auto &command_class_handler: command_handler_list) {
        command_handler_list_alphabetic_sort.push_back(command_class_handler.get());
    }

    // Sort the new list alphabetically
    std::sort(command_handler_list_alphabetic_sort.begin(), command_handler_list_alphabetic_sort.end(), [](zwave_command_class_base *a, zwave_command_class_base *b) { return a->display_name() < b->display_name(); });

    std::stringstream ss;
    ss << std::endl;
    // clang-format off
  ss << "| " << "Command Class                 " << " | Version | Support | Control | Security Level              | Comment |" << std::endl;
  ss << "| " << "------------------------------" << " | ------- | ------- | ------- | --------------------------- | ------- |" << std::endl;
    // clang-format on

    // Then print the info of each element
    for (auto &command_class_handler: command_handler_list_alphabetic_sort) {
        ss << "| ";
        ss << std::setw(30) << std::left << command_class_handler->display_name();
        ss << " | ";
        ss << std::setw(sizeof("Version") - 1) << std::right << std::to_string(command_class_handler->supported_version());
        ss << " | ";
        ss << std::setw(sizeof("Support") - 1) << (command_class_handler->has_support_handler() ? "x" : " ");
        ss << " | ";
        ss << std::setw(sizeof("Control") - 1) << (command_class_handler->has_control_handler() ? "x" : " ");
        ss << " | ";
        std::string security_level;
        // Security Level only makes sense for command classes that have Support set
        if (command_class_handler->has_support_handler()) {
            security_level = zwave_network_scheme_str(command_class_handler->supported_handler_minimal_scheme());
        } else {
            security_level = "N/A";
        }
        // With is the maximum with of the value returned by zwave_network_scheme_str
        ss << std::setw(27) << std::left << security_level;
        ss << " | ";
        ss << std::setw(sizeof("Comment") - 1) << command_class_handler->comments();
        ss << " |" << std::endl;
    }
    if (fd < 0) {
        sl_log_info(LOG_TAG, ss.str().c_str());
    } else {
        dprintf(fd, "%s", ss.str().c_str());
    }
}
