#ifndef CRYPTO_BACKEND_TEST_FIXTURE_HPP
#define CRYPTO_BACKEND_TEST_FIXTURE_HPP

#include <gtest/gtest.h>

#include "psa/crypto_extra.h"
#include "psa_wrappers.hpp"

class CryptoBackend : public ::testing::Test
{
    protected:
        crypto::PsaWrappers psa_wrappers;

        void SetUp() override
        {
            ASSERT_EQ(PSA_SUCCESS, psa_wrappers.crypto_init());
        }

        void TearDown() override
        {
            mbedtls_psa_crypto_free();
        }
};

#endif
