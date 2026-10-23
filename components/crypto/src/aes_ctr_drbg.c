#include <string.h>
#include <stdint.h>

#include "aes_ctr_drbg.h"
#include "aes_ecb.h"

static sl_status_t increment(uint8_t *data, uint8_t length)
{
    if (data == NULL || length == 0) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    while (length--) {
        data[length]++;
        if (data[length]) {
            break;
        }
    }

    return SL_STATUS_OK;
}

static sl_status_t update(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *data)
{
    if (ctx == NULL || data == NULL) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    uint8_t tmp[CRYPTO_AES_CTR_DRBG_SEED_SIZE] = {0};

    for (uint8_t i = 0; i < CRYPTO_AES_CTR_DRBG_SEED_SIZE; i += CRYPTO_AES_BLOCK_SIZE) {
        sl_status_t status = increment(ctx->counter, CRYPTO_AES_BLOCK_SIZE);
        if (status != SL_STATUS_OK) {
            return status;
        }
        status = crypto_aes_ecb_encrypt(ctx->key, ctx->counter, &tmp[i]);
        if (status != SL_STATUS_OK) {
            return status;
        }
    }

    for (uint8_t i = 0; i < CRYPTO_AES_CTR_DRBG_SEED_SIZE; i++) {
        tmp[i] ^= data[i];
    }

    memcpy(ctx->key, tmp, CRYPTO_AES_KEY_SIZE);
    memcpy(ctx->counter, tmp + CRYPTO_AES_KEY_SIZE, CRYPTO_AES_BLOCK_SIZE);
    return SL_STATUS_OK;
}

sl_status_t crypto_aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input, const uint8_t *pers_string)
{
    if (ctx == NULL || entropy_input == NULL || pers_string == NULL) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    crypto_aes_ctr_drbg_ctx_t next_ctx = {0};
    uint8_t seed[CRYPTO_AES_CTR_DRBG_SEED_SIZE];

    memcpy(seed, entropy_input, sizeof(seed));
    if (pers_string != NULL) {
        for (uint8_t i = 0; i < CRYPTO_AES_CTR_DRBG_SEED_SIZE; i++) {
            seed[i] ^= pers_string[i];
        }
    }

    sl_status_t status = update(&next_ctx, seed);
    if (status != SL_STATUS_OK) {
        return status;
    }

    *ctx = next_ctx;
    return SL_STATUS_OK;
}

sl_status_t crypto_aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t *ctx, uint8_t *out)
{
    if (ctx == NULL || out == NULL) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    crypto_aes_ctr_drbg_ctx_t next_ctx = *ctx;
    uint8_t block[CRYPTO_AES_BLOCK_SIZE];
    uint8_t provided_data[CRYPTO_AES_CTR_DRBG_SEED_SIZE] = {0};
    uint8_t generated[CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE];
    uint8_t *generated_pos = generated;
    size_t copy;
    size_t remaining = CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE;

    while (remaining) {
        sl_status_t status = increment(next_ctx.counter, CRYPTO_AES_BLOCK_SIZE);
        if (status != SL_STATUS_OK) {
            return status;
        }
        status = crypto_aes_ecb_encrypt(next_ctx.key, next_ctx.counter, block);
        if (status != SL_STATUS_OK) {
            return status;
        }
        copy = (remaining < CRYPTO_AES_BLOCK_SIZE) ? remaining : CRYPTO_AES_BLOCK_SIZE;
        memcpy(generated_pos, block, copy);
        generated_pos += copy;
        remaining -= copy;
    }

    memset(provided_data, 0, sizeof(provided_data));
    sl_status_t status = update(&next_ctx, provided_data);
    if (status != SL_STATUS_OK) {
        return status;
    }

    *ctx = next_ctx;
    memcpy(out, generated, sizeof(generated));
    return SL_STATUS_OK;
}
