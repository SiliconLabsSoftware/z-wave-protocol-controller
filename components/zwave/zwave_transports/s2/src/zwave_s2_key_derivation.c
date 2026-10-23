#include <string.h>

#include "zwave_s2_key_derivation.h"
#include "aes_cmac.h"
#include "x25519.h"

#define CKDF_EXPAND_CONSTANT_LEN 15U

static uint8_t ckdf_expand_message_first(uint8_t *message, const uint8_t *expand_constant, uint8_t step)
{
    memcpy(message, expand_constant, CKDF_EXPAND_CONSTANT_LEN);
    message[CKDF_EXPAND_CONSTANT_LEN] = step;
    return (uint8_t)(CKDF_EXPAND_CONSTANT_LEN + sizeof(step)); /* constant || step */
}

static uint8_t ckdf_expand_message_chain(uint8_t *message, const uint8_t *previous_block, const uint8_t *expand_constant, uint8_t step)
{
    memcpy(message, previous_block, CRYPTO_AES_CMAC_OUTPUT_LEN);
    memcpy(message + CRYPTO_AES_CMAC_OUTPUT_LEN, expand_constant, CKDF_EXPAND_CONSTANT_LEN);
    message[CRYPTO_AES_CMAC_OUTPUT_LEN + CKDF_EXPAND_CONSTANT_LEN] = step;
    return (uint8_t)(CRYPTO_AES_CMAC_OUTPUT_LEN + CKDF_EXPAND_CONSTANT_LEN + sizeof(step)); /* T || constant || step */
}

static sl_status_t derive_key(const uint8_t *cmac_key, const uint8_t *expand_constant, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key)
{
    if (cmac_key == NULL || expand_constant == NULL || ccm_key == NULL || nonce_pers_string == NULL || mpan_key == NULL) {
        return SL_STATUS_NULL_POINTER;
    }

    uint8_t message[CRYPTO_AES_CMAC_OUTPUT_LEN + CKDF_EXPAND_CONSTANT_LEN + sizeof(uint8_t)]; /* longest input: T || constant || step */
    uint8_t t1[CRYPTO_AES_CMAC_OUTPUT_LEN];
    uint8_t t2[CRYPTO_AES_CMAC_OUTPUT_LEN];
    uint8_t t3[CRYPTO_AES_CMAC_OUTPUT_LEN];
    uint8_t t4[CRYPTO_AES_CMAC_OUTPUT_LEN];
    sl_status_t status;
    uint8_t message_len;

    message_len = ckdf_expand_message_first(message, expand_constant, 0x01);
    status      = crypto_aes_cmac_compute(cmac_key, message, message_len, t1);
    if (status != SL_STATUS_OK) {
        return status;
    }

    message_len = ckdf_expand_message_chain(message, t1, expand_constant, 0x02);
    status      = crypto_aes_cmac_compute(cmac_key, message, message_len, t2);
    if (status != SL_STATUS_OK) {
        return status;
    }

    message_len = ckdf_expand_message_chain(message, t2, expand_constant, 0x03);
    status      = crypto_aes_cmac_compute(cmac_key, message, message_len, t3);
    if (status != SL_STATUS_OK) {
        return status;
    }

    message_len = ckdf_expand_message_chain(message, t3, expand_constant, 0x04);
    status      = crypto_aes_cmac_compute(cmac_key, message, message_len, t4);
    if (status != SL_STATUS_OK) {
        return status;
    }

    memcpy(ccm_key, t1, sizeof(t1));
    memcpy(nonce_pers_string, t2, sizeof(t2));
    memcpy(nonce_pers_string + sizeof(t2), t3, sizeof(t3));
    memcpy(mpan_key, t4, sizeof(t4));
    return SL_STATUS_OK;
}

sl_status_t zwave_s2_extract_pseudo_random_key(const uint8_t *ecdh_shared_secret, const uint8_t *public_keys, uint8_t *pseudo_random_key)
{
    if (ecdh_shared_secret == NULL || public_keys == NULL || pseudo_random_key == NULL) {
        return SL_STATUS_NULL_POINTER;
    }

    static const uint8_t constant_pseudo_random_key[CRYPTO_AES_CMAC_OUTPUT_LEN] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

    uint8_t message[CRYPTO_X25519_SHARED_SECRET_SIZE + (2 * CRYPTO_X25519_PUBLIC_KEY_SIZE)];
    uint8_t mac[CRYPTO_AES_CMAC_OUTPUT_LEN];
    sl_status_t status;

    memcpy(message, ecdh_shared_secret, CRYPTO_X25519_SHARED_SECRET_SIZE);
    memcpy(message + CRYPTO_X25519_SHARED_SECRET_SIZE, public_keys, 2 * CRYPTO_X25519_PUBLIC_KEY_SIZE);
    status = crypto_aes_cmac_compute(constant_pseudo_random_key, message, sizeof(message), mac);
    if (status != SL_STATUS_OK) {
        return status;
    }
    memcpy(pseudo_random_key, mac, sizeof(mac));
    return SL_STATUS_OK;
}

sl_status_t zwave_s2_derive_temp_key(const uint8_t *pseudo_random_key, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key)
{
    static const uint8_t constant_temp_key[CKDF_EXPAND_CONSTANT_LEN] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88};

    return derive_key(pseudo_random_key, constant_temp_key, ccm_key, nonce_pers_string, mpan_key);
}

sl_status_t zwave_s2_derive_network_key(const uint8_t *network_key, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key)
{
    static const uint8_t constant_network_key[CKDF_EXPAND_CONSTANT_LEN] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};

    return derive_key(network_key, constant_network_key, ccm_key, nonce_pers_string, mpan_key);
}
