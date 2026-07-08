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

#ifndef ZWAVE_FRAME_GENERATOR_GENERIC_HPP
#define ZWAVE_FRAME_GENERATOR_GENERIC_HPP

#ifdef __cplusplus

// ZPC includes
#include "attribute_store.h"
#include "sl_status.h"

// Cpp includes
#include <array>

// Base class
#include "zwave_frame_generator_base.hpp"

#include "zwave_node_id_definitions.h"  // ZWAVE_MAX_FRAME_SIZE

/**
 * @class zwave_frame_generator_standalone
 * @brief GFrame generator for Z-Wave commands for standalone use
 *
 * This command class allows you to generate a Z-Wave frame without the need to
 * initialize a C array to store the frame. Useful to generate reports.
 *
 * @endcode
 */
class zwave_frame_generator_standalone : public zwave_frame_generator_base
{
    public:
        explicit zwave_frame_generator_standalone();
        ~zwave_frame_generator_standalone() = default;

        /**
         * @brief Generate the frame
         *
         * @return std::vector<uint8_t> The generated frame
         */
        std::vector<uint8_t> generate_frame();

        /**
         * @brief Clear current frame contents
         */
        void clear();
        /**
         * @brief Add a header to the frame
         *
         * @note This function should be called before adding any other byte to the frame. Otherwise the header will be put after the frame contents.
         *
         * @param command_class The command class
         * @param command_id The command id
         */
        void add_header(uint8_t command_class, uint8_t command_id);

        /**
         * @brief Add multiples bytes to a frame
         */
        void add_byte_range(const std::vector<uint8_t> &bytes);

    private:
        std::array<uint8_t, ZWAVE_MAX_FRAME_SIZE> array_frame;
};

#endif  // __cplusplus
#endif  // ZWAVE_FRAME_GENERATOR_GENERIC_HPP