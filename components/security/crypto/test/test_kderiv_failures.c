#include <stdint.h>
#include <string.h>

#include "kderiv.h"
#include "unity.h"

static unsigned cmac_call_count;
static unsigned cmac_fail_call;

int aes_cmac_calculate(const uint8_t *key, const uint8_t *message, const uint16_t message_length, uint8_t *mac)
{
    (void)key;
    (void)message;
    (void)message_length;

    cmac_call_count++;
    if (cmac_call_count == cmac_fail_call) {
        return 0;
    }

    memset(mac, (int)cmac_call_count, 16);
    return 1;
}

void setUp(void)
{
    cmac_call_count = 0;
    cmac_fail_call  = 0;
}

void tearDown(void) {}

void test_networkkey_expand_leaves_outputs_unchanged_on_each_cmac_failure(void)
{
    const uint8_t key[16] = {0};

    for (cmac_fail_call = 1; cmac_fail_call <= 4; cmac_fail_call++) {
        uint8_t ccm_key[16];
        uint8_t nonce_pstring[32];
        uint8_t mpan_key[16];
        uint8_t expected_ccm_key[16];
        uint8_t expected_nonce_pstring[32];
        uint8_t expected_mpan_key[16];

        memset(ccm_key, 0xa1, sizeof(ccm_key));
        memset(nonce_pstring, 0xb2, sizeof(nonce_pstring));
        memset(mpan_key, 0xc3, sizeof(mpan_key));
        memcpy(expected_ccm_key, ccm_key, sizeof(ccm_key));
        memcpy(expected_nonce_pstring, nonce_pstring, sizeof(nonce_pstring));
        memcpy(expected_mpan_key, mpan_key, sizeof(mpan_key));
        cmac_call_count = 0;

        TEST_ASSERT_FALSE(networkkey_expand(0, key, ccm_key, nonce_pstring, mpan_key));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_ccm_key, ccm_key, sizeof(ccm_key));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_nonce_pstring, nonce_pstring, sizeof(nonce_pstring));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_mpan_key, mpan_key, sizeof(mpan_key));
    }
}

void test_tempkey_expand_leaves_outputs_unchanged_on_cmac_failure(void)
{
    const uint8_t key[16] = {0};
    uint8_t ccm_key[16];
    uint8_t nonce_pstring[32];
    uint8_t mpan_key[16];

    memset(ccm_key, 0xa1, sizeof(ccm_key));
    memset(nonce_pstring, 0xb2, sizeof(nonce_pstring));
    memset(mpan_key, 0xc3, sizeof(mpan_key));
    cmac_fail_call = 2;

    TEST_ASSERT_FALSE(tempkey_expand(0, key, ccm_key, nonce_pstring, mpan_key));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xa1, ccm_key, sizeof(ccm_key));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xb2, nonce_pstring, sizeof(nonce_pstring));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xc3, mpan_key, sizeof(mpan_key));
}

void test_tempkey_extract_leaves_output_unchanged_on_cmac_failure(void)
{
    const uint8_t shared_secret[32] = {0};
    const uint8_t auth_tag[64]      = {0};
    uint8_t keymat[16];

    memset(keymat, 0xd4, sizeof(keymat));
    cmac_fail_call = 1;

    TEST_ASSERT_FALSE(tempkey_extract(shared_secret, auth_tag, keymat));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xd4, keymat, sizeof(keymat));
}
