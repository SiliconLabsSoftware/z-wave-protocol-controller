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

/**
 * @defgroup zwave_frame_parser C++ Z-Wave Frame Handler Helper
 * @brief C++ definitions for handling Z-Wave Frames
 *
 * This group is used to handle Z-Wave frame and link their contents with the attribute store.
 *
 * @{
 */

#ifndef ZWAVE_FRAME_GENERATOR_HPP
#define ZWAVE_FRAME_GENERATOR_HPP

#ifdef __cplusplus

// ZPC includes
#include "attribute_store.h"
#include "sl_status.h"

// Cpp includes
#include <vector>

// Base class
#include "zwave_frame_generator_base.hpp"

/**
 * @class zwave_frame_generator
 * @brief Generate frames for Z-Wave commands based on attribute store values
 *
 * Mainly used to generate Set or Get frame to send to Z-Wave devices
 *
 * You can either set raw bytes, or let the function get the value from the attribute store for
 * you. You are able to specify if you want a Desired or Reported value.
 *
 * @code{.cpp}
 * // Only needed to be instantiated once
 * static zwave_frame_generator frame_generator(COMMAND_CLASS_SWITCH_BINARY);
 *
 * // On a frame callback :
 * static sl_status_t zwave_command_class_set(
 *    attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length) {
 *
 *  try {
 *    frame_generator.initialize_frame(SWITCH_BINARY_SET, frame, expected_frame_size);
 *    frame_generator.add_raw_byte(0x01);
 *    frame_generator.add_value(my_binary_node, REPORTED_ATTRIBUTE);
 *
 *    // Will take the DESIRED_ATTRIBUTE value from my_node and shift it 2 bits to the left,
 *    // then add 0b1 shifted 7 bits to the left.
 *    // Result value (if desired value is 0b11) : 0b10001100
 *    std::vector<zwave_frame_generator::shifted_value> values_mix = {
 *      {.left_shift       = 2,
 *        .node             = my_node,
 *        .node_value_state = DESIRED_ATTRIBUTE},
 *      {.left_shift = 7, .raw_value = 0b1},
 *    };
 *    frame_generator.add_shifted_values(values_mix);
 *
 *    return frame_generator.generate_frame();
 *
 *  } catch (const std::exception &e) {
 *   sl_log_error(LOG_TAG, "Error while generating frame : %s", e.what());
 *   return SL_STATUS_FAIL;
 *  }
 *
 *  return SL_STATUS_OK;
 * }
 *
 * @endcode
 */
class zwave_frame_generator : public zwave_frame_generator_base
{
    public:
        /**
         * @brief Constructor
         *
         * @param zwave_command_class The Z-Wave command class to use in the header of all generated commands
         */
        explicit zwave_frame_generator(uint8_t zwave_command_class);
        ~zwave_frame_generator() = default;

        /**
         * @brief Initialize a new Z-Wave frame on the given data section.
         *
         * @note This will reset the internal counter to 0 and update the data section provided with other functions.
         *
         * After calling this function your frame will look like :
         * 0: zwave_command_class (from constructor)
         * 1: zwave_command_id (from this function)
         *
         * @param zwave_command_id The Z-Wave command ID to use in the header of the frame
         * @param raw_data The data section of the frame (already allocated)
         * @param data_size The size section of the frame (already allocated)
         */
        void initialize_frame(uint8_t zwave_command_id, uint8_t *raw_data, uint16_t *data_size);

        /**
         * @brief Generate the Z-Wave frame
         *
         * Writes the frame length to the pointer provided in initialize_frame.
         *
         * @return SL_STATUS_OK
         */
        sl_status_t generate_frame() const;

        /**
         * @brief Generate a frame with no arguments
         *
         * Convenience function to generate a frame with no arguments like a simple Get command.
         *
         * @return SL_STATUS_OK if the frame was generated successfully, SL_STATUS_FAIL otherwise
         */
        sl_status_t generate_no_args_frame() const;

        /**
         * @brief Generate a Z-Wave frame with no arguments
         *
         * This function is used to generate a Z-Wave frame with no arguments like a simple Get command.
         * Since it is used for convenience, this method doesn't throw an exception and return a status instead.
         *
         * @param zwave_command_id The Z-Wave command ID to use in the header of the frame
         * @param raw_data The data section of the frame (must be able to write 2 byte to this address)
         * @param frame_length Frame length pointer (set to 2)
         *
         * @return SL_STATUS_OK if the frame was generated successfully, SL_STATUS_FAIL otherwise
         */
        sl_status_t generate_no_args_frame(uint8_t zwave_command_id, uint8_t *raw_data, uint16_t *frame_length);

    private:
        // Current Z-Wave command class used in the header of all generated commands
        const uint8_t current_command_class;
};

#endif  // __cplusplus
#endif  // ZWAVE_FRAME_GENERATOR_HPP