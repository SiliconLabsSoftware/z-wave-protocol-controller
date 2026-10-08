
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

#ifndef COMMAND_CLASS_THERMOSTAT_OPERATING_STATE_CONSTANTS_H
#define COMMAND_CLASS_THERMOSTAT_OPERATING_STATE_CONSTANTS_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace zwave_command_class::command_class_thermostat_operating_state_constants
{

    inline bool logging_supported_bit_mask_has_logs(const std::vector<uint8_t> &bit_mask)
    {
        for (size_t byte_idx = 0; byte_idx < bit_mask.size(); ++byte_idx) {
            const uint8_t start_bit = (byte_idx == 0) ? 1U : 0U;
            for (uint8_t bit = start_bit; bit < 8U; ++bit) {
                if ((bit_mask[byte_idx] & (1U << bit)) != 0U) {
                    return true;
                }
            }
        }
        return false;
    }

}  // namespace zwave_command_class::command_class_thermostat_operating_state_constants

#endif  // COMMAND_CLASS_THERMOSTAT_OPERATING_STATE_CONSTANTS_H
