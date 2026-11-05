#ifndef PSA_WRAPPERS_TEST_FIXTURE_HPP
#define PSA_WRAPPERS_TEST_FIXTURE_HPP

#include <gtest/gtest.h>

#include "psa/crypto_extra.h"
#include "psa/psa_wrappers.hpp"

class PsaWrappersTest : public ::testing::Test
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
