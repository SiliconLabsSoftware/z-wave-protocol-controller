#include <string.h>
#include <stdint.h>

#include "nextnonce.h"
#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include "unity.h"

static const uint8_t test_sei_valid[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
static const uint8_t test_rei_valid[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
static uint8_t test_nonce_key[32]     = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

void setUp(void)
{
    TEST_ASSERT_EQUAL(PSA_SUCCESS, psa_crypto_init());
}

void tearDown(void)
{
    mbedtls_psa_crypto_free();
}

void test_nextnonce_deterministic_and_advances(void)
{
    CTR_DRBG_CTX span_rng_1;
    CTR_DRBG_CTX span_rng_2;
    uint8_t nonce_1[16];
    uint8_t nonce_2[16];
    uint8_t nonce_1_second[16];

    next_nonce_instantiate(&span_rng_1, test_sei_valid, test_rei_valid, test_nonce_key);
    TEST_ASSERT_EQUAL_INT(1, next_nonce_generate(&span_rng_1, nonce_1));

    next_nonce_instantiate(&span_rng_2, test_sei_valid, test_rei_valid, test_nonce_key);
    TEST_ASSERT_EQUAL_INT(1, next_nonce_generate(&span_rng_2, nonce_2));

    TEST_ASSERT_EQUAL_UINT8_ARRAY(nonce_1, nonce_2, 16);

    TEST_ASSERT_EQUAL_INT(1, next_nonce_generate(&span_rng_1, nonce_1_second));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(nonce_1, nonce_1_second, 16));
}
