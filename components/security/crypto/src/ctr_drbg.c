/* © 2017 Silicon Laboratories Inc.
 */

#include <string.h>
#include <stdint.h>

#include "ctr_drbg.h"
#include "s2_psa_primitives.h"

static int aes_ecb_128_encrypt(const uint8_t *key, const uint8_t *in, uint8_t *out)
{
    uint8_t block[16];

    memcpy(block, in, 16);
    return s2_psa_aes128_ecb_encrypt(key, block, out) == PSA_SUCCESS;
}

static void AES_CTR_DRBG_Increment(uint8_t *__data, size_t size)
{
    while (size--) {
        __data[size]++;
        if (__data[size]) {
            break;
        }
    }
}

static int AES_CTR_DRBG_Update(CTR_DRBG_CTX *ctx, const uint8_t __data[SEEDLEN])
{
    size_t i             = 0;
    uint8_t tmp[SEEDLEN] = {0};
    uint8_t *t           = tmp;

    for (i = 0; i < SEEDLEN; i += OUTLEN) {
        AES_CTR_DRBG_Increment(ctx->v, OUTLEN);
        if (!aes_ecb_128_encrypt(ctx->k, ctx->v, t)) {
            return 0;
        }
        t += OUTLEN;
    }

    for (i = 0; i < SEEDLEN; i++) {
        tmp[i] ^= __data[i];
    }

    memcpy(ctx->k, tmp, KEYLEN);
    memcpy(ctx->v, tmp + KEYLEN, OUTLEN);
    return 1;
}

int AES_CTR_DRBG_Reseed(CTR_DRBG_CTX *ctx, const uint8_t *seed)
{
    if (ctx == NULL || seed == NULL) {
        return 0;
    }

    CTR_DRBG_CTX next_ctx = *ctx;

    if (!AES_CTR_DRBG_Update(&next_ctx, seed)) {
        return 0;
    }

    *ctx = next_ctx;
    return 1;
}

int AES_CTR_DRBG_Instantiate(CTR_DRBG_CTX *ctx, const uint8_t *entropy, const uint8_t *personal)
{
    if (ctx == NULL || entropy == NULL) {
        return 0;
    }

    CTR_DRBG_CTX next_ctx = {0};
    uint8_t seed[SEEDLEN];

    memcpy(seed, entropy, sizeof(seed));
    if (personal != NULL) {
        for (size_t i = 0; i < SEEDLEN; i++) {
            seed[i] ^= personal[i];
        }
    }

    next_ctx.df = 0;
    if (!AES_CTR_DRBG_Update(&next_ctx, seed)) {
        return 0;
    }

    *ctx = next_ctx;
    return 1;
}

int AES_CTR_DRBG_Generate(CTR_DRBG_CTX *ctx, uint8_t *rand)
{
    if (ctx == NULL || rand == NULL) {
        return 0;
    }

    CTR_DRBG_CTX next_ctx   = *ctx;
    uint8_t __data[SEEDLEN] = {0};
    uint8_t output[RANDLEN];
    uint8_t *output_pos = output;
    size_t copy;
    size_t size = RANDLEN;

    while (size) {
        AES_CTR_DRBG_Increment(next_ctx.v, OUTLEN);
        if (!aes_ecb_128_encrypt(next_ctx.k, next_ctx.v, __data)) {
            return 0;
        }
        copy = (size < OUTLEN) ? size : OUTLEN;
        memcpy(output_pos, __data, copy);
        output_pos += copy;
        size -= copy;
    }

    memset(__data, 0, SEEDLEN);
    if (!AES_CTR_DRBG_Update(&next_ctx, __data)) {
        return 0;
    }

    *ctx = next_ctx;
    memcpy(rand, output, sizeof(output));
    return 1;
}
