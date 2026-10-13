/* © 2017 Silicon Laboratories Inc.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "ctr_drbg.h"
#include "s2_psa_primitives.h"

static void aes_ecb_128_encrypt(const uint8_t *key, const uint8_t *in, uint8_t *out)
{
    uint8_t block[16];

    memcpy(block, in, 16);
    assert(s2_psa_aes128_ecb_encrypt(key, block, out) == PSA_SUCCESS);
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

static void AES_CTR_DRBG_Update(CTR_DRBG_CTX *ctx, const uint8_t __data[SEEDLEN])
{
    size_t i             = 0;
    uint8_t tmp[SEEDLEN] = {0};
    uint8_t *t           = tmp;

    for (i = 0; i < SEEDLEN; i += OUTLEN) {
        AES_CTR_DRBG_Increment(ctx->v, OUTLEN);
        aes_ecb_128_encrypt(ctx->k, ctx->v, t);
        t += OUTLEN;
    }

    for (i = 0; i < SEEDLEN; i++) {
        tmp[i] ^= __data[i];
    }

    memcpy(ctx->k, tmp, KEYLEN);
    memcpy(ctx->v, tmp + KEYLEN, OUTLEN);
}

void AES_CTR_DRBG_Reseed(CTR_DRBG_CTX *ctx, uint8_t *seed)
{
    AES_CTR_DRBG_Update(ctx, seed);
}

void AES_CTR_DRBG_Instantiate(CTR_DRBG_CTX *ctx, uint8_t *entropy, const uint8_t *personal)
{
    uint8_t i;

    if (personal != NULL) {
        for (i = 0; i < SEEDLEN; i++) {
            entropy[i] ^= personal[i];
        }
    } else {
        for (i = 0; i < SEEDLEN; i++) {
            entropy[i] ^= 0;
        }
    }

    memset(ctx->k, 0, KEYLEN);
    memset(ctx->v, 0, OUTLEN);
    ctx->df = 0;
    AES_CTR_DRBG_Reseed(ctx, entropy);
}

void AES_CTR_DRBG_Generate(CTR_DRBG_CTX *ctx, uint8_t *rand)
{
    uint8_t __data[SEEDLEN] = {0};
    size_t copy;
    size_t size = RANDLEN;

    while (size) {
        AES_CTR_DRBG_Increment(ctx->v, OUTLEN);
        aes_ecb_128_encrypt(ctx->k, ctx->v, __data);
        copy = (size < OUTLEN) ? size : OUTLEN;
        memcpy(rand, __data, copy);
        rand += copy;
        size -= copy;
    }

    memset(__data, 0, SEEDLEN);
    AES_CTR_DRBG_Update(ctx, __data);
}
