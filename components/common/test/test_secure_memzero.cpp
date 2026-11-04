#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "secure_memzero.h"

TEST(SecureMemzero, ClearsOnlyRequestedStorage)
{
    std::array<uint8_t, 6> bytes {1, 2, 3, 4, 5, 6};
    secure_memzero(bytes.data() + 1, 4);
    EXPECT_EQ((std::array<uint8_t, 6> {1, 0, 0, 0, 0, 6}), bytes);
}

TEST(SecureMemzero, IgnoresNullAndEmptyBuffers)
{
    uint8_t sentinel = 0xa5;
    secure_memzero(nullptr, 0);
    secure_memzero(nullptr, 16);
    secure_memzero(&sentinel, 0);
    EXPECT_EQ(0xa5, sentinel);
}
