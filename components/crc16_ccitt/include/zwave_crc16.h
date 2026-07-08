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

/**
 * @defgroup crc16_ccitt CRC-16 CCITT Calculation Utility
 * @brief Inline CRC16-CCITT calculation function with Polynomial 0x1021
 *
 * @{
 */
#ifndef ZWAVE_CRC16_H
#define ZWAVE_CRC16_H

#include <stdint.h>

#define CRC16_INIT_VALUE 0x1D0F
#define CRC16_POLY       0x1021

/**
 * @brief Calculation of CRC16-CCITT with Polynomial 0x1021 on data
 *
 * @param crc16             Initial value for CRC 16 calculation algorithm
 * @param data_buf          Pointer to the data buffer
 * @param data_length       Length of the data
 * @return calculated two byte CRC16 checksum
 */
static inline uint16_t zwave_crc16(uint16_t crc16, const uint8_t *data_buf, unsigned long data_length)
{
    uint8_t work_data;
    uint8_t new_bit;
    while (data_length--) {
        work_data = *data_buf++;
        for (uint8_t bit_mask = 0x80; bit_mask != 0; bit_mask >>= 1) {
            /* Align test bit with next bit of the message byte, starting with msb. */
            new_bit = ((work_data & bit_mask) != 0) ^ ((crc16 & 0x8000) != 0);
            crc16 <<= 1;
            if (new_bit) {
                crc16 ^= CRC16_POLY;
            }
        }
    }
    return crc16;
}

#endif  // ZWAVE_CRC16_H
/** @} end crc16_ccitt */
