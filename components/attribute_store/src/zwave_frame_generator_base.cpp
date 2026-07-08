/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
// Base class
#include "zwave_frame_generator_base.hpp"

// ZPC
#include "zwave_node_id_definitions.h"  // ZWAVE_MAX_FRAME_SIZE
#include "attribute_store_type_registration.h"
#include "log.h"
#include "attribute.hpp"

// Cpp includes
#include <stdexcept>
#include <sstream>    // for ostringstream
#include <algorithm>  // for std::for_each

// CRC16
#include "zwave_crc16.h"

constexpr char LOG_TAG[] = "zwave_frame_generator";

zwave_frame_generator_base::zwave_frame_generator_base() {}

void zwave_frame_generator_base::add_raw_byte(uint8_t byte)
{
    if (current_zwave_frame_index >= ZWAVE_MAX_FRAME_SIZE) {
        std::ostringstream out;
        out << "Attempt to set index " << current_zwave_frame_index << " in a frame of size " << ZWAVE_MAX_FRAME_SIZE;
        throw std::out_of_range(out.str());
    }
    current_zwave_frame[current_zwave_frame_index++] = byte;
}

std::vector<uint8_t> zwave_frame_generator_base::helper_get_raw_data(attribute_store_node_t node, attribute_store_node_value_state_t node_value_state)
{
    // Used to get the name of the attribute in case of error
    const attribute_store::attribute node_cpp(node);

    sl_log_debug(LOG_TAG, node_cpp.value_to_string().c_str());

    // First get value size
    uint8_t value_size = 0;

    // The logic isn't implemented for DESIRED_OR_REPORTED_ATTRIBUTE, so we need to do it
    // ourselves.
    if (node_value_state == DESIRED_OR_REPORTED_ATTRIBUTE) {
        value_size = attribute_store_get_node_value_size(node, DESIRED_ATTRIBUTE);
        if (value_size == 0) {
            value_size = attribute_store_get_node_value_size(node, REPORTED_ATTRIBUTE);
        }
    } else {
        value_size = attribute_store_get_node_value_size(node, node_value_state);
    }

    if (value_size == 0) {
        throw std::runtime_error("Failed to get value size from attribute store for attribute " + node_cpp.name_and_id());
    }

    // Then get raw data
    std::vector<uint8_t> raw_data;
    raw_data.resize(value_size);
    sl_status_t status = attribute_store_get_node_attribute_value(node, node_value_state, raw_data.data(), &value_size);

    if (status != SL_STATUS_OK) {
        throw std::runtime_error("Failed to get value for attribute " + node_cpp.name_and_id());
    }

    return raw_data;
}

void zwave_frame_generator_base::add_value(attribute_store_node_t node, attribute_store_node_value_state_t node_value_state)
{
    auto raw_data = helper_get_raw_data(node, node_value_state);

    auto storage_type = attribute_store_get_storage_type(attribute_store_get_node_type(node));
    // Remove the NULL terminator if we are dealing with a string
    if (storage_type == C_STRING_STORAGE_TYPE) {
        raw_data.pop_back();
    }

    // For non-numeric types we need to send them as is
    if (storage_type == C_STRING_STORAGE_TYPE || storage_type == BYTE_ARRAY_STORAGE_TYPE || storage_type == FIXED_SIZE_STRUCT_STORAGE_TYPE || storage_type == INVALID_STORAGE_TYPE) {
        // Store in order
        std::for_each(raw_data.begin(), raw_data.end(), [this](uint8_t byte) { add_raw_byte(byte); });
    }  // Otherwise the MSB is always the first in Z-Wave frames (and in attribute store it is last)
    else {
        // Store in reverse order (reverse iterator to start from the MSB)
        std::for_each(raw_data.rbegin(), raw_data.rend(), [this](uint8_t byte) { add_raw_byte(byte); });
    }
}

void zwave_frame_generator_base::add_shifted_values(const std::vector<shifted_value> &shifted_values)
{
    uint8_t final_value = 0;
    for (const auto &shifted_value: shifted_values) {
        uint8_t current_value = 0;
        // If we don't have a node, take the raw value instead
        if (shifted_value.node == ATTRIBUTE_STORE_INVALID_NODE) {
            current_value = shifted_value.raw_value;
        } else {
            auto raw_data = helper_get_raw_data(shifted_value.node, shifted_value.node_value_state);
            if (raw_data.size() != 1) {
                throw std::runtime_error("Shifted value should be 1 byte long");
            }
            current_value = raw_data[0];
        }
        final_value |= (current_value << shifted_value.left_shift);
    }
    add_raw_byte(final_value);
}

void zwave_frame_generator_base::add_shifted_values(const shifted_value &sv)
{
    std::vector<shifted_value> shifted_values = {sv};
    add_shifted_values(shifted_values);
}

sl_status_t zwave_frame_generator_base::check_frame_size(uint16_t expected_frame_length) const
{
    if (expected_frame_length == 0 || current_zwave_frame_index == expected_frame_length) {
        return SL_STATUS_OK;
    }
    std::ostringstream out;
    out << "Frame size (" << current_zwave_frame_index << ") does not match the expected size (" << expected_frame_length << ")";
    sl_log_error(LOG_TAG, out.str().c_str());
    return SL_STATUS_FAIL;
}

void zwave_frame_generator_base::add_checksum(uint16_t init_value)
{
    auto checksum = zwave_crc16(init_value, current_zwave_frame, current_zwave_frame_index);
    add_raw_value(checksum);
}

void zwave_frame_generator_base::add_raw_value(uint16_t value)
{
    add_raw_byte(value >> 8);
    add_raw_byte(value & 0xFF);
}