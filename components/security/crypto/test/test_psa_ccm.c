#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include "psa_test_support.h"
#include "unity.h"

#define S2_CCM_TAG_LEN   8U
#define S2_CCM_NONCE_LEN 13U

void setUp(void)
{
    TEST_ASSERT_EQUAL(PSA_SUCCESS, psa_crypto_init());
}

void tearDown(void)
{
    mbedtls_psa_crypto_free();
}

void test_psa_ccm_nist_example1(void)
{
    const uint8_t key[16]       = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};
    const uint8_t nonce[8]      = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
    const uint8_t aad[16]       = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const uint8_t plaintext[16] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
    const uint8_t expected[22]  = {0xd2, 0xa1, 0xf0, 0xe0, 0x51, 0xea, 0x5f, 0x62, 0x08, 0x1a, 0x77, 0x92, 0x07, 0x3d, 0x59, 0x3d, 0x1f, 0xc6, 0x4f, 0xbf, 0xac, 0xcd};
    uint8_t ciphertext[32];
    size_t ciphertext_len;
    size_t plaintext_len;

    TEST_ASSERT_EQUAL(PSA_SUCCESS, s2_psa_ccm_encrypt(key, 6, nonce, sizeof(nonce), aad, sizeof(aad), plaintext, sizeof(plaintext), ciphertext, sizeof(ciphertext), &ciphertext_len));
    TEST_ASSERT_EQUAL(22, ciphertext_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, ciphertext, ciphertext_len);

    TEST_ASSERT_EQUAL(PSA_SUCCESS, s2_psa_ccm_decrypt(key, 6, nonce, sizeof(nonce), aad, sizeof(aad), ciphertext, ciphertext_len, ciphertext, sizeof(ciphertext), &plaintext_len));
    TEST_ASSERT_EQUAL(sizeof(plaintext), plaintext_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, ciphertext, plaintext_len);
}

void test_psa_ccm_s2_roundtrip_payload_lengths(void)
{
    const uint8_t key[16]                 = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};
    const uint8_t nonce[S2_CCM_NONCE_LEN] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c};
    const uint8_t aad[16]                 = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const int lengths[]                   = {0, 10, 22, 54, 1279, 1280};

    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        const size_t pt_len = (size_t)lengths[i];
        uint8_t *plaintext  = calloc(1, pt_len + 1);
        uint8_t *backup     = calloc(1, pt_len + 1);
        uint8_t *buffer     = calloc(1, pt_len + S2_CCM_TAG_LEN + 1);
        size_t ct_len;
        size_t out_len;

        TEST_ASSERT_NOT_NULL(plaintext);
        TEST_ASSERT_NOT_NULL(backup);
        TEST_ASSERT_NOT_NULL(buffer);

        for (size_t j = 0; j < pt_len; j++) {
            plaintext[j] = (uint8_t)j;
            backup[j]    = (uint8_t)j;
        }

        TEST_ASSERT_EQUAL(PSA_SUCCESS, s2_psa_ccm_encrypt(key, S2_CCM_TAG_LEN, nonce, S2_CCM_NONCE_LEN, aad, sizeof(aad), plaintext, pt_len, buffer, pt_len + S2_CCM_TAG_LEN, &ct_len));
        TEST_ASSERT_EQUAL(pt_len + S2_CCM_TAG_LEN, ct_len);

        TEST_ASSERT_EQUAL(PSA_SUCCESS, s2_psa_ccm_decrypt(key, S2_CCM_TAG_LEN, nonce, S2_CCM_NONCE_LEN, aad, sizeof(aad), buffer, ct_len, buffer, pt_len + S2_CCM_TAG_LEN, &out_len));
        if (pt_len != 0) {
            TEST_ASSERT_EQUAL(pt_len, out_len);
            TEST_ASSERT_EQUAL_UINT8_ARRAY(backup, buffer, pt_len);
        }

        free(plaintext);
        free(backup);
        free(buffer);
    }
}

void test_psa_ccm_s2_handful_aad_lengths(void)
{
    const uint8_t key[16]                 = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};
    const uint8_t nonce[S2_CCM_NONCE_LEN] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c};
    const uint8_t plaintext[32]           = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};
    const int aad_lengths[]               = {0, 1, 10, 11, 12, 21, 22, 300};
    const size_t pt_len                   = sizeof(plaintext);

    for (size_t i = 0; i < sizeof(aad_lengths) / sizeof(aad_lengths[0]); i++) {
        const size_t aad_len = (size_t)aad_lengths[i];
        uint8_t *aad         = calloc(1, aad_len);
        uint8_t buffer[64];
        size_t ct_len;
        size_t out_len;

        TEST_ASSERT_NOT_NULL(aad);
        for (size_t j = 0; j < aad_len; j++) {
            aad[j] = (uint8_t)j;
        }

        TEST_ASSERT_EQUAL(PSA_SUCCESS, s2_psa_ccm_encrypt(key, S2_CCM_TAG_LEN, nonce, S2_CCM_NONCE_LEN, aad, aad_len, plaintext, pt_len, buffer, sizeof(buffer), &ct_len));
        TEST_ASSERT_EQUAL(pt_len + S2_CCM_TAG_LEN, ct_len);

        TEST_ASSERT_EQUAL(PSA_SUCCESS, s2_psa_ccm_decrypt(key, S2_CCM_TAG_LEN, nonce, S2_CCM_NONCE_LEN, aad, aad_len, buffer, ct_len, buffer, sizeof(buffer), &out_len));
        TEST_ASSERT_EQUAL(pt_len, out_len);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, buffer, pt_len);

        free(aad);
    }
}

void test_psa_ccm_decrypt_auth_failure(void)
{
    const uint8_t key[16]                 = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};
    const uint8_t nonce[S2_CCM_NONCE_LEN] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c};
    const uint8_t aad[16]                 = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const uint8_t plaintext[16]           = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
    uint8_t buffer[32];
    size_t ct_len;
    size_t out_len;
    psa_status_t status;

    TEST_ASSERT_EQUAL(PSA_SUCCESS, s2_psa_ccm_encrypt(key, S2_CCM_TAG_LEN, nonce, S2_CCM_NONCE_LEN, aad, sizeof(aad), plaintext, sizeof(plaintext), buffer, sizeof(buffer), &ct_len));

    buffer[ct_len - 1] ^= 0x01U;

    status = s2_psa_ccm_decrypt(key, S2_CCM_TAG_LEN, nonce, S2_CCM_NONCE_LEN, aad, sizeof(aad), buffer, ct_len, buffer, sizeof(buffer), &out_len);
    TEST_ASSERT_TRUE(status == PSA_ERROR_INVALID_SIGNATURE || status == PSA_ERROR_INVALID_ARGUMENT);
    /* PSA does not leave the in-place ciphertext intact on authentication failure. */
}
