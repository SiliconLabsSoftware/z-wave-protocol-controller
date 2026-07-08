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
#include "zwave_frame_generator.hpp"

// ZPC
#include "zwave_node_id_definitions.h"  // ZWAVE_MAX_FRAME_SIZE
#include "attribute_store_type_registration.h"
#include "log.h"
#include "attribute.hpp"

// Cpp includes
#include <sstream>    // for ostringstream
#include <algorithm>  // for std::for_each

constexpr char LOG_TAG[] = "zwave_frame_generator";

zwave_frame_generator::zwave_frame_generator(uint8_t zwave_command_class) : current_command_class(zwave_command_class) {}

void zwave_frame_generator::initialize_frame(uint8_t zwave_command, uint8_t *data, uint16_t *data_size)
{
    // Reset current frame index
    current_zwave_frame_index = 0;

    // Assignments
    current_zwave_frame      = data;
    current_zwave_frame_size = data_size;

    // Create frame header
    add_raw_byte(current_command_class);
    add_raw_byte(zwave_command);
}

sl_status_t zwave_frame_generator::generate_frame() const
{
    *current_zwave_frame_size = current_zwave_frame_index;
    return SL_STATUS_OK;
}

sl_status_t zwave_frame_generator::generate_no_args_frame() const
{
    return generate_frame();
}

sl_status_t zwave_frame_generator::generate_no_args_frame(uint8_t zwave_command_id, uint8_t *raw_data, uint16_t *frame_length)
{
    try {
        initialize_frame(zwave_command_id, raw_data, frame_length);
    } catch (const std::exception &e) {
        sl_log_error(LOG_TAG, "Failed to generate frame: %s", e.what());
        return SL_STATUS_FAIL;
    }
    return generate_frame();
}
