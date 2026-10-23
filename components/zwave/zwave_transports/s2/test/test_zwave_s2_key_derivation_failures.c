#include <stdint.h>
#include <string.h>

#include "zwave_s2_key_derivation.h"
#include "aes_cmac.h"
#include "sl_status.h"
#include "unity.h"

static unsigned cmac_call_count;
static unsigned cmac_fail_call;

sl_status_t crypto_aes_cmac_compute(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac)
{
    (void)key;
    (void)message;
    (void)message_length;

    cmac_call_count++;
    if (cmac_call_count == cmac_fail_call) {
        return SL_STATUS_FAIL;
    }

    memset(mac, (int)cmac_call_count, 16);
    return SL_STATUS_OK;
}

void setUp(void)
{
    cmac_call_count = 0;
    cmac_fail_call  = 0;
}

void tearDown(void) {}

void test_zwave_s2_network_key_expand_leaves_outputs_unchanged_on_each_cmac_failure(void)
{
    const uint8_t key[16] = {0};

    for (cmac_fail_call = 1; cmac_fail_call <= 4; cmac_fail_call++) {
        uint8_t ccm_key[16];
        uint8_t nonce_pers_string[32];
        uint8_t mpan_key[16];
        uint8_t expected_ccm_key[16];
        uint8_t expected_nonce_pers_string[32];
        uint8_t expected_mpan_key[16];

        memset(ccm_key, 0xa1, sizeof(ccm_key));
        memset(nonce_pers_string, 0xb2, sizeof(nonce_pers_string));
        memset(mpan_key, 0xc3, sizeof(mpan_key));
        memcpy(expected_ccm_key, ccm_key, sizeof(ccm_key));
        memcpy(expected_nonce_pers_string, nonce_pers_string, sizeof(nonce_pers_string));
        memcpy(expected_mpan_key, mpan_key, sizeof(mpan_key));
        cmac_call_count = 0;

        TEST_ASSERT_NOT_EQUAL(SL_STATUS_OK, zwave_s2_derive_network_key(key, ccm_key, nonce_pers_string, mpan_key));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_ccm_key, ccm_key, sizeof(ccm_key));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_nonce_pers_string, nonce_pers_string, sizeof(nonce_pers_string));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_mpan_key, mpan_key, sizeof(mpan_key));
    }
}

void test_zwave_s2_temp_key_expand_leaves_outputs_unchanged_on_cmac_failure(void)
{
    const uint8_t key[16] = {0};
    uint8_t ccm_key[16];
    uint8_t nonce_pers_string[32];
    uint8_t mpan_key[16];

    memset(ccm_key, 0xa1, sizeof(ccm_key));
    memset(nonce_pers_string, 0xb2, sizeof(nonce_pers_string));
    memset(mpan_key, 0xc3, sizeof(mpan_key));
    cmac_fail_call = 2;

    TEST_ASSERT_NOT_EQUAL(SL_STATUS_OK, zwave_s2_derive_temp_key(key, ccm_key, nonce_pers_string, mpan_key));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xa1, ccm_key, sizeof(ccm_key));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xb2, nonce_pers_string, sizeof(nonce_pers_string));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xc3, mpan_key, sizeof(mpan_key));
}

void test_zwave_s2_extract_pseudo_random_key_leaves_output_unchanged_on_cmac_failure(void)
{
    const uint8_t shared_secret[32] = {0};
    const uint8_t public_keys[64]   = {0};
    uint8_t keymat[16];

    memset(keymat, 0xd4, sizeof(keymat));
    cmac_fail_call = 1;

    TEST_ASSERT_NOT_EQUAL(SL_STATUS_OK, zwave_s2_extract_pseudo_random_key(shared_secret, public_keys, keymat));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xd4, keymat, sizeof(keymat));
}
