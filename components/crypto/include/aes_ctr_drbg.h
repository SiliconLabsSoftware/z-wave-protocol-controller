#ifndef AES_CTR_DRBG_H
#define AES_CTR_DRBG_H

#include <stdint.h>

#include "aes.h"
#include "sl_status.h"

#define CRYPTO_AES_CTR_DRBG_SEED_SIZE (CRYPTO_AES_BLOCK_SIZE + CRYPTO_AES_KEY_SIZE)

#ifndef CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE
#define CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE 16
#endif

typedef struct crypto_aes_ctr_drbg_ctx {
        uint8_t counter[CRYPTO_AES_BLOCK_SIZE];
        uint8_t key[CRYPTO_AES_KEY_SIZE];
} crypto_aes_ctr_drbg_ctx_t;

sl_status_t crypto_aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input, const uint8_t *pers_string);
sl_status_t crypto_aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t *ctx, uint8_t *out);

#endif
