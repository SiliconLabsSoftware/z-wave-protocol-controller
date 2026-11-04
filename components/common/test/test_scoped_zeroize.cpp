#include <array>
#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>

#include "scoped_zeroize.hpp"

static_assert(!std::is_copy_constructible_v<common::ScopedZeroize>);
static_assert(!std::is_move_constructible_v<common::ScopedZeroize>);

class ScopedZeroizeTest : public ::testing::TestWithParam<bool>
{};

TEST_P(ScopedZeroizeTest, ClearsStorageOnSuccessAndEarlyReturn)
{
    std::array<uint8_t, 16> secret;
    secret.fill(0xa5);
    const auto use_secret = [&] {
        const common::ScopedZeroize wipe(secret.data(), sizeof(secret));
        EXPECT_EQ(0xa5, secret.front());
        if (GetParam()) {
            return false;
        }
        secret.back() = 0x42;
        return true;
    };

    EXPECT_EQ(!GetParam(), use_secret());
    EXPECT_EQ((std::array<uint8_t, 16> {}), secret);
}

INSTANTIATE_TEST_SUITE_P(ReturnPaths, ScopedZeroizeTest, ::testing::Bool());
