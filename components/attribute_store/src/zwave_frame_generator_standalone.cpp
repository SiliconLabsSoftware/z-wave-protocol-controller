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
#include "zwave_frame_generator_standalone.hpp"

// ZPC
#include "attribute_store_type_registration.h"
#include "log.h"
#include "attribute.hpp"

// Cpp includes
#include <stdexcept>
#include <sstream>    // for ostringstream
#include <algorithm>  // for std::for_each,

[[maybe_unused]] constexpr char LOG_TAG[] = "zwave_frame_generator_standalone";

zwave_frame_generator_standalone::zwave_frame_generator_standalone()
{
    // Initialize the frame with the maximum size
    current_zwave_frame = array_frame.data();
}

std::vector<uint8_t> zwave_frame_generator_standalone::generate_frame()
{
    std::vector<uint8_t> frame(array_frame.begin(), array_frame.begin() + current_zwave_frame_index);
    return frame;
}

void zwave_frame_generator_standalone::clear()
{
    current_zwave_frame_index = 0;
    array_frame.fill(0);
    current_zwave_frame = array_frame.data();
}

void zwave_frame_generator_standalone::add_header(uint8_t command_class, uint8_t command_id)
{
    add_raw_byte(command_class);
    add_raw_byte(command_id);
}

void zwave_frame_generator_standalone::add_byte_range(const std::vector<uint8_t> &bytes)
{
    // With C++20 we could use std::ranges::for_each
    for (const auto &byte: bytes) {
        add_raw_byte(byte);
    }
}