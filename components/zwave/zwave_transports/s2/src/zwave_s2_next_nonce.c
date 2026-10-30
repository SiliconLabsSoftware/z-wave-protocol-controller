#include <string.h>
#include <stdint.h>

#include "crypto.h"
#include "sl_status.h"
#include "zwave_s2_next_nonce.h"

#define CKDF_EXPAND_CONSTANT_LEN 15U
#define CKDF_EXPAND_PREFIX_LEN   (CKDF_EXPAND_CONSTANT_LEN + sizeof(uint8_t)) /* constant_entropy_input || step */

static sl_status_t ckdf_nonce0_expand(const uint8_t *pseudo_random_key, uint8_t *mixing_entropy_input)
{
    if (pseudo_random_key == NULL || mixing_entropy_input == NULL) {
        return SL_STATUS_NULL_POINTER;
    }

    static const uint8_t constant_entropy_input[CKDF_EXPAND_CONSTANT_LEN] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
    uint8_t t0[2 * CKDF_EXPAND_PREFIX_LEN];
    uint8_t t1[CRYPTO_AES_CMAC_OUTPUT_LEN];
    uint8_t t2[CRYPTO_AES_CMAC_OUTPUT_LEN];
    sl_status_t status;

    memcpy(t0, constant_entropy_input, CKDF_EXPAND_CONSTANT_LEN);
    t0[CKDF_EXPAND_CONSTANT_LEN] = 0;

    memcpy(t0 + CKDF_EXPAND_PREFIX_LEN, constant_entropy_input, CKDF_EXPAND_CONSTANT_LEN);
    t0[CKDF_EXPAND_PREFIX_LEN + CKDF_EXPAND_CONSTANT_LEN] = 1;
    status                                                = crypto_aes_cmac_compute(pseudo_random_key, t0, 2 * CKDF_EXPAND_PREFIX_LEN, t1);
    if (status != SL_STATUS_OK) {
        return status;
    }

    memcpy(t0, t1, CRYPTO_AES_CMAC_OUTPUT_LEN);
    memcpy(t0 + CRYPTO_AES_CMAC_OUTPUT_LEN, constant_entropy_input, CKDF_EXPAND_CONSTANT_LEN);
    t0[CRYPTO_AES_CMAC_OUTPUT_LEN + CKDF_EXPAND_CONSTANT_LEN] = 2;
    status                                                    = crypto_aes_cmac_compute(pseudo_random_key, t0, CRYPTO_AES_CMAC_OUTPUT_LEN + CKDF_EXPAND_PREFIX_LEN, t2);
    if (status != SL_STATUS_OK) {
        return status;
    }

    memcpy(mixing_entropy_input, t1, CRYPTO_AES_CMAC_OUTPUT_LEN);
    memcpy(mixing_entropy_input + CRYPTO_AES_CMAC_OUTPUT_LEN, t2, CRYPTO_AES_CMAC_OUTPUT_LEN);
    return SL_STATUS_OK;
}

sl_status_t zwave_s2_next_nonce_instantiate(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input_sender, const uint8_t *entropy_input_receiver, const uint8_t *nonce_key)
{
    if (ctx == NULL || entropy_input_sender == NULL || entropy_input_receiver == NULL || nonce_key == NULL) {
        return SL_STATUS_NULL_POINTER;
    }

    static const uint8_t constant_nonce[CRYPTO_AES_BLOCK_SIZE] = {0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26};
    uint8_t temp[2 * CRYPTO_AES_BLOCK_SIZE];
    uint8_t X[CRYPTO_AES_CMAC_OUTPUT_LEN];
    uint8_t mei[CRYPTO_AES_CTR_DRBG_SEED_SIZE];
    sl_status_t status;

    memcpy(temp, entropy_input_sender, CRYPTO_AES_BLOCK_SIZE);
    memcpy(temp + CRYPTO_AES_BLOCK_SIZE, entropy_input_receiver, CRYPTO_AES_BLOCK_SIZE);
    status = crypto_aes_cmac_compute(constant_nonce, temp, sizeof(temp), X);
    if (status != SL_STATUS_OK) {
        return status;
    }

    status = ckdf_nonce0_expand(X, mei);
    if (status != SL_STATUS_OK) {
        return status;
    }

    return crypto_aes_ctr_drbg_init(ctx, mei, nonce_key);
}
