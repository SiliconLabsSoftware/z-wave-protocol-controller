/* © 2014 Silicon Laboratories Inc.
 */

#include <assert.h>
#include <string.h>

#include "aes_cmac.h"
#include "kderiv.h"

#define ZWAVE_KEY_ID_NONE 0
#define CONSTANT_NK_LEN   15

static int zw_expand_aes_cmac(uint32_t key_id, const uint8_t *network_key, const uint8_t *input, uint16_t length, uint8_t *output)
{
    assert(key_id == ZWAVE_KEY_ID_NONE);
    return aes_cmac_calculate(network_key, input, length, output);
}

int generic_expand(uint32_t key_id, const uint8_t *network_key, const uint8_t *constant_nk, uint8_t *ccm_key, uint8_t *nonce_pstring, uint8_t *mpan_key)
{
    uint8_t temp[32];
    uint8_t derived_ccm_key[16];
    uint8_t derived_nonce_pstring[32];
    uint8_t derived_mpan_key[16];

    memcpy(temp, constant_nk, 15);
    temp[15] = 0x01;
    if (!zw_expand_aes_cmac(key_id, network_key, temp, 16, derived_ccm_key)) {
        return 0;
    }

    memcpy(temp, derived_ccm_key, 16);
    memcpy(temp + 16, constant_nk, 15);
    temp[31] = 0x02;
    if (!zw_expand_aes_cmac(key_id, network_key, temp, 32, derived_nonce_pstring)) {
        return 0;
    }

    memcpy(temp, derived_nonce_pstring, 16);
    memcpy(temp + 16, constant_nk, 15);
    temp[31] = 0x03;
    if (!zw_expand_aes_cmac(key_id, network_key, temp, 32, derived_nonce_pstring + 16)) {
        return 0;
    }

    memcpy(temp, derived_nonce_pstring + 16, 16);
    memcpy(temp + 16, constant_nk, 15);
    temp[31] = 0x04;
    if (!zw_expand_aes_cmac(key_id, network_key, temp, 32, derived_mpan_key)) {
        return 0;
    }

    memcpy(ccm_key, derived_ccm_key, sizeof(derived_ccm_key));
    memcpy(nonce_pstring, derived_nonce_pstring, sizeof(derived_nonce_pstring));
    memcpy(mpan_key, derived_mpan_key, sizeof(derived_mpan_key));
    return 1;
}

int networkkey_expand(uint32_t key_id, const uint8_t *network_key, uint8_t *ccm_key, uint8_t *nonce_pstring, uint8_t *mpan_key)
{
    uint8_t constant_nk[15] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    return generic_expand(key_id, network_key, constant_nk, ccm_key, nonce_pstring, mpan_key);
}

int tempkey_extract(const uint8_t *const ecdh_share_secret, const uint8_t *const auth_tag, uint8_t *pseudo_random_keymat_output)
{
    uint8_t temp[96];
    uint8_t constant_prk[16] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
    uint8_t derived_keymat[16];

    memcpy(temp, ecdh_share_secret, 32);
    memcpy(temp + 32, auth_tag, 64);
    if (!aes_cmac_calculate(constant_prk, temp, 96, derived_keymat)) {
        return 0;
    }
    memcpy(pseudo_random_keymat_output, derived_keymat, sizeof(derived_keymat));
    return 1;
}

int tempkey_expand(uint32_t key_id, const uint8_t *prk, uint8_t *temp_ccm_key, uint8_t *temp_nonce_pstring, uint8_t *temp_mpan_key)
{
    uint8_t constant_TE[15] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
    return generic_expand(key_id, prk, constant_TE, temp_ccm_key, temp_nonce_pstring, temp_mpan_key);
}
