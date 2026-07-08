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

#ifndef ZWAVE_FRAME_GENERATOR_BASE_HPP
#define ZWAVE_FRAME_GENERATOR_BASE_HPP

#ifdef __cplusplus

// ZPC includes
#include "attribute_store.h"
#include "sl_status.h"

// Cpp includes
#include <vector>

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
class zwave_frame_generator_base
{
    public:
        /**
         * @brief Represent a value that needs to be shifted before being added to the frame.
         *
         * You can either specify the node and value state to get the value from the attribute store
         * or provide a raw value to be shifted.
         *
         * @note If you provide a node, it will read a uint8_t value from it.
         */
        struct shifted_value {
                /**
                 * @brief The number of bits to shift the value (left)
                 */
                uint8_t left_shift = 0;
                /**
                 * @brief Node to get the value from (uint8_t). If ATTRIBUTE_STORE_INVALID_NODE, use raw_value
                 */
                attribute_store_node_t node = ATTRIBUTE_STORE_INVALID_NODE;
                /**
                 * @brief State of the value to get from the node. Only used if node is not ATTRIBUTE_STORE_INVALID_NODE
                 */
                attribute_store_node_value_state_t node_value_state = REPORTED_ATTRIBUTE;
                /**
                 * @brief Raw value to shift. Only used if node is ATTRIBUTE_STORE_INVALID_NODE
                 */
                uint8_t raw_value = 0;
        };

        /**
         * @brief Constructor
         *
         * @param zwave_command_class The Z-Wave command class to use in the header of all generated commands
         */
        explicit zwave_frame_generator_base();
        ~zwave_frame_generator_base() = default;

        /**
         * @brief Add a raw byte to the Z-Wave frame
         *
         * @param byte The byte to add to the frame
         */
        void add_raw_byte(uint8_t byte);

        /**
         * @brief Add the value contained in the given node to the Z-Wave frame
         *
         * @throws std::runtime_error if the node is invalid or if the value can't be read
         *
         * @note The size of the value is automatically determined by the attribute store.
         *       Numerical values will be stored in big-endian (MSB first LSB last).
         *       Other formats will keep their original order.
         *
         * @param node The node to get the value from
         * @param node_value_state The state of the value to get from the node
         *
         */
        void add_value(attribute_store_node_t node, attribute_store_node_value_state_t node_value_state = REPORTED_ATTRIBUTE);
        /**
         * @brief Add a shifted value to the Z-Wave frame
         *
         * You can either specify a raw value to be shifted, or directly pass the attribute
         * store node.
         *
         * @see shifted_value
         *
         * @param shifted_values The shifted value to add to the frame
         */
        void add_shifted_values(const std::vector<shifted_value> &shifted_values);
        /**
         * @brief Add a shifted value to the Z-Wave frame (single value version)
         *
         * Convenience function to add a single shifted value to the frame.
         *
         * @see shifted_value
         *
         * @param shifted_values The shifted value to add to the frame
         */
        void add_shifted_values(const shifted_value &sv);

        /**
         * @brief Check frame size
         *
         * @param frame_length Check if the frame is the expected length
         *
         * @return SL_STATUS_OK if the frame is the expected length, SL_STATUS_FAIL otherwise
         */
        sl_status_t check_frame_size(uint16_t expected_frame_length) const;

        /**
         * @brief Add checksum to the frame (uint16_t)
         *
         * Take all the frame contents and compute the crc16 checksum using CRC-CCITT polynomial.
         *
         * @param init_value The initialization value (default is 0x1D0F)
         */
        void add_checksum(uint16_t init_value = 0x1D0F);

        /**
         * @brief Add a uin16_t to the frame
         */
        void add_raw_value(uint16_t value);

    protected:
        /**
         * @brief Helper function to get the raw data from the attribute store
         *
         * @note Number will be returned in little-endian (LSB first MSB last)
         *
         * @param node The node to get the value from
         * @param node_value_state The state of the value to get from the node
         *
         * @return The raw data from the attribute store
         */
        static std::vector<uint8_t> helper_get_raw_data(attribute_store_node_t node, attribute_store_node_value_state_t node_value_state);

        // Preallocated memory where the contents of the frame will be written.
        uint8_t *current_zwave_frame;
        // Total bytes we have written in current_zwave_frame.
        uint16_t *current_zwave_frame_size;
        // Current Z-Wave frame index (we use uint16_t to match ZPC API)
        uint16_t current_zwave_frame_index = 0;
};

#endif  // __cplusplus
#endif  // ZWAVE_FRAME_GENERATOR_BASE_HPP