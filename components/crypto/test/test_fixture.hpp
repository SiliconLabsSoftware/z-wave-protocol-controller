#ifndef TEST_FIXTURE_HPP_
#define TEST_FIXTURE_HPP_

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include "sl_status.h"

class Crypto : public ::testing::Test
{
    protected:
        void SetUp() override
        {
            ASSERT_EQ(PSA_SUCCESS, psa_crypto_init());
        }

        void TearDown() override
        {
            mbedtls_psa_crypto_free();
        }
};

#endif
