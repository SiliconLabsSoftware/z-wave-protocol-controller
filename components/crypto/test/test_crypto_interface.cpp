#include <algorithm>
#include <array>

#include "crypto_mock.hpp"

using ::testing::StrictMock;

TEST(CryptoInterface, ApplicationCanUseMockWithoutPsaBackend)
{
    StrictMock<crypto::test::CryptoMock> mock;
    const crypto::CryptoInterface &algorithms = mock;
    std::array<uint8_t, 8> output {};
    EXPECT_CALL(mock, generate_random(::testing::_)).WillOnce([](std::span<uint8_t> bytes) {
        std::fill(bytes.begin(), bytes.end(), 0xa5);
        return SL_STATUS_OK;
    });

    EXPECT_EQ(SL_STATUS_OK, algorithms.generate_random(output));
    EXPECT_THAT(output, ::testing::Each(0xa5));
}
