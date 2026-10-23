#include <stdint.h>
#include <string.h>

#include "S2.h"
#include "zwave_s2_key_derivation.h"
#include "s2_protocol.h"
#include "unity.h"

static sl_status_t expand_result;
static unsigned networkkey_expand_calls;
static unsigned tempkey_expand_calls;

sl_status_t zwave_s2_derive_network_key(const uint8_t *network_key, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key)
{
    (void)network_key;
    (void)ccm_key;
    (void)nonce_pers_string;
    (void)mpan_key;
    networkkey_expand_calls++;
    return expand_result;
}

sl_status_t zwave_s2_derive_temp_key(const uint8_t *prk, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key)
{
    (void)prk;
    (void)ccm_key;
    (void)nonce_pers_string;
    (void)mpan_key;
    tempkey_expand_calls++;
    return expand_result;
}

sl_status_t zwave_s2_extract_pseudo_random_key(const uint8_t *ecdh_shared_secret, const uint8_t *public_keys, uint8_t *prk)
{
    (void)ecdh_shared_secret;
    (void)public_keys;
    (void)prk;
    return SL_STATUS_OK;
}

uint8_t S2_send_frame_no_cb(struct S2 *ctxt, const s2_connection_t *peer, uint8_t *buf, uint16_t len)
{
    (void)ctxt;
    (void)peer;
    (void)buf;
    (void)len;
    return 0;
}

uint8_t S2_send_frame_multi(struct S2 *ctxt, s2_connection_t *peer, uint8_t *buf, uint16_t len)
{
    (void)ctxt;
    (void)peer;
    (void)buf;
    (void)len;
    return 0;
}

void S2_get_commands_supported(node_t lnode, uint8_t class_id, const uint8_t **cmdClasses, uint8_t *length)
{
    (void)lnode;
    (void)class_id;
    *cmdClasses = NULL;
    *length     = 0;
}

void S2_resynchronization_event(node_t remote_node, sos_event_reason_t reason, uint8_t seqno, node_t local_node)
{
    (void)remote_node;
    (void)reason;
    (void)seqno;
    (void)local_node;
}

void setUp(void)
{
    expand_result           = SL_STATUS_FAIL;
    networkkey_expand_calls = 0;
    tempkey_expand_calls    = 0;
}

void tearDown(void) {}

static void assert_key_update_failure(uint8_t use_temp_key)
{
    struct S2 context;
    const network_key_t network_key = {0};
    const security_class_t class_id = 1;
    uint8_t expected_enc_key[16];
    uint8_t expected_nonce_key[32];
    uint8_t expected_mpan_key[16];

    memset(&context, 0, sizeof(context));
    memset(context.sg[class_id].enc_key, 0xa1, sizeof(context.sg[class_id].enc_key));
    memset(context.sg[class_id].nonce_key, 0xb2, sizeof(context.sg[class_id].nonce_key));
    memset(context.sg[class_id].mpan_key, 0xc3, sizeof(context.sg[class_id].mpan_key));
    memcpy(expected_enc_key, context.sg[class_id].enc_key, sizeof(expected_enc_key));
    memcpy(expected_nonce_key, context.sg[class_id].nonce_key, sizeof(expected_nonce_key));
    memcpy(expected_mpan_key, context.sg[class_id].mpan_key, sizeof(expected_mpan_key));

    TEST_ASSERT_FALSE(S2_network_key_update(&context, 0, class_id, network_key, use_temp_key));
    TEST_ASSERT_EQUAL_UINT8(0, context.loaded_keys);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_enc_key, context.sg[class_id].enc_key, sizeof(expected_enc_key));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_nonce_key, context.sg[class_id].nonce_key, sizeof(expected_nonce_key));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_mpan_key, context.sg[class_id].mpan_key, sizeof(expected_mpan_key));
}

void test_network_key_update_does_not_load_or_modify_key_on_expansion_failure(void)
{
    assert_key_update_failure(0);
    TEST_ASSERT_EQUAL_UINT32(1, networkkey_expand_calls);
    TEST_ASSERT_EQUAL_UINT32(0, tempkey_expand_calls);
}

void test_temp_key_update_does_not_load_or_modify_key_on_expansion_failure(void)
{
    assert_key_update_failure(1);
    TEST_ASSERT_EQUAL_UINT32(0, networkkey_expand_calls);
    TEST_ASSERT_EQUAL_UINT32(1, tempkey_expand_calls);
}
