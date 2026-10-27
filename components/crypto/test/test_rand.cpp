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

#include <cstdint>

#include <gtest/gtest.h>

#include "test_fixture.hpp"
#include "rand.h"

namespace
{
    bool buffer_is_all_zeros(const uint8_t *buf, size_t len)
    {
        for (size_t i = 0; i < len; i++) {
            if (buf[i] != 0) {
                return false;
            }
        }
        return true;
    }
}  // namespace

TEST_F(Crypto, GenerateRandomFillsBuffer)
{
    uint8_t buf[32] {};

    ASSERT_EQ(SL_STATUS_OK, crypto_generate_random(buf, sizeof(buf)));
    EXPECT_FALSE(buffer_is_all_zeros(buf, sizeof(buf)));
}

TEST_F(Crypto, GenerateRandomRejectsNullOutput)
{
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_generate_random(nullptr, 32));
}
