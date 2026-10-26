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

#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "psa_test_support.hpp"
#include "rand.hpp"

namespace
{
    bool BufferIsAllZeros(const uint8_t *buf, size_t len)
    {
        for (size_t i = 0; i < len; i++) {
            if (buf[i] != 0) {
                return false;
            }
        }
        return true;
    }
}  // namespace

// C API fills a buffer with non-deterministic bytes.
TEST_F(PsaTest, CryptoGenerateRandomCApi)
{
    uint8_t buf[32] {};

    ASSERT_EQ(SL_STATUS_OK, crypto_generate_random(buf, sizeof(buf)));
    EXPECT_FALSE(BufferIsAllZeros(buf, sizeof(buf)));
}

// C++ generate_random fills a fixed-size buffer with non-zero data.
TEST_F(PsaTest, CryptoGenerateRandomCppApi)
{
    std::array<uint8_t, 32> buf {};

    ASSERT_EQ(SL_STATUS_OK, crypto::generate_random(buf));
    EXPECT_FALSE(BufferIsAllZeros(buf.data(), buf.size()));
}
