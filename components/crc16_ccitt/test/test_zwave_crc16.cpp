/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <gtest/gtest.h>

#include "zwave_crc16.h"

TEST(ZwaveCrc16, EmptyBufferLeavesInitValue)
{
    const uint8_t *data = nullptr;
    EXPECT_EQ(zwave_crc16(CRC16_INIT_VALUE, data, 0), CRC16_INIT_VALUE);
}

TEST(ZwaveCrc16, SingleByte)
{
    const uint8_t data[] = {'A'};
    EXPECT_EQ(zwave_crc16(CRC16_INIT_VALUE, data, sizeof(data)), 0x9479U);
}

TEST(ZwaveCrc16, ShortVector)
{
    const uint8_t data[] = {0x01, 0x02, 0x03};
    EXPECT_EQ(zwave_crc16(CRC16_INIT_VALUE, data, sizeof(data)), 0x703DU);
}
