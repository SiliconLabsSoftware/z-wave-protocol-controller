#include <stdint.h>
#include <string.h>

#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include "psa_test_support.h"
#include "aes_cbc_mac.h"
#include "aes_ecb.h"
#include "unity.h"

/* NIST SP 800-38A AES-128 key and IV (same material as OFB/CMAC tests). */

static const uint8_t nist_key_128[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static const uint8_t nist_iv[]      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

static const uint8_t nist_plain_16[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};

/*
 * Z-Wave S0 auth-data layout (IV passed separately to crypto_aes_cbc_mac):
 * command, sender node ID, receiver node ID, payload length, encrypted payload.
 */
static const uint8_t s0_auth_iv[]   = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
static const uint8_t s0_auth_data[] = {
  0x81, /* SECURITY_MESSAGE_ENCAPSULATION */
  0x03, /* sender */
  0x04, /* receiver */
  0x02, /* encrypted payload length */
  0x11,
  0x22, /* encrypted payload (example ciphertext) */
};

void setUp(void)
{
    TEST_ASSERT_EQUAL(PSA_SUCCESS, psa_crypto_init());
}

void tearDown(void)
{
    mbedtls_psa_crypto_free();
}

static void assert_s0_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *message, uint8_t message_len, const uint8_t *expected_mac)
{
    uint8_t calculated_mac[16];

    TEST_ASSERT_EQUAL(SL_STATUS_OK, crypto_aes_cbc_mac(key, iv, message, message_len, calculated_mac));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_mac, calculated_mac, 16);
}

/* Empty auth payload: MAC state is only E_K(IV) (NIST AES-128-ECB encrypt IV). */
void test_s0_cbc_mac_empty_message_equals_aes_ecb_of_iv(void)
{
    const uint8_t expected_mac[] = {0x50, 0xfe, 0x67, 0xcc, 0x99, 0x6d, 0x32, 0xb6, 0xda, 0x09, 0x37, 0xe9, 0x9b, 0xaf, 0xec, 0x60};
    uint8_t ecb_out[16];

    assert_s0_cbc_mac(nist_key_128, nist_iv, NULL, 0, expected_mac);
    TEST_ASSERT_EQUAL(SL_STATUS_OK, crypto_aes_ecb_encrypt(nist_key_128, nist_iv, ecb_out));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_mac, ecb_out, 16);
}

/* One full 16-byte block (NIST SP 800-38A plaintext); exercises one mid-loop ECB step. */
void test_s0_cbc_mac_nist_plaintext_one_block(void)
{
    const uint8_t expected_mac[] = {0x66, 0x8b, 0xcf, 0x60, 0xbe, 0xb0, 0x05, 0xa3, 0x53, 0x54, 0xa2, 0x01, 0xda, 0xb3, 0x6b, 0xda};

    assert_s0_cbc_mac(nist_key_128, nist_iv, nist_plain_16, sizeof(nist_plain_16), expected_mac);
}

/* Partial final block: spec padding path (final AES encrypt when len % 16 != 0). */
void test_s0_cbc_mac_nist_plaintext_seven_bytes(void)
{
    const uint8_t expected_mac[] = {0x13, 0x73, 0x94, 0x01, 0x8c, 0xdf, 0xe1, 0x7f, 0x59, 0xf9, 0x77, 0x97, 0x9e, 0xff, 0x71, 0x08};

    assert_s0_cbc_mac(nist_key_128, nist_iv, nist_plain_16, 7, expected_mac);
}

/* Two AES blocks: second block triggers another ECB iteration at byte index 15. */
void test_s0_cbc_mac_nist_plaintext_two_blocks(void)
{
    uint8_t message[32];
    const uint8_t expected_mac[] = {0x11, 0xe0, 0xad, 0xbc, 0x92, 0x31, 0xe1, 0x3d, 0xd8, 0x43, 0x6e, 0x69, 0xd9, 0xf3, 0x14, 0xcc};

    memcpy(message, nist_plain_16, 16);
    memcpy(message + 16, nist_plain_16, 16);

    assert_s0_cbc_mac(nist_key_128, nist_iv, message, sizeof(message), expected_mac);
}

/* Typical S0 authentication data size (header + short encrypted payload). */
void test_s0_cbc_mac_message_encapsulation_auth_data(void)
{
    const uint8_t expected_mac[] = {0x4c, 0x5c, 0xb1, 0x62, 0xa5, 0x17, 0x4c, 0x77, 0xf4, 0x11, 0x84, 0xa2, 0x5a, 0xe7, 0xb6, 0x0c};

    assert_s0_cbc_mac(nist_key_128, s0_auth_iv, s0_auth_data, sizeof(s0_auth_data), expected_mac);
}
