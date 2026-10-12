#include "unity.h"
#include "psa/crypto.h"
#include "psa/crypto_extra.h"

void setUp(void)
{
    psa_status_t status = psa_crypto_init();
    TEST_ASSERT_EQUAL_MESSAGE(PSA_SUCCESS, status, "psa_crypto_init");
}

void tearDown(void)
{
    mbedtls_psa_crypto_free();
}

void test_psa_generate_random(void)
{
    uint8_t buf[16]     = {0};
    psa_status_t status = psa_generate_random(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(PSA_SUCCESS, status);
}
