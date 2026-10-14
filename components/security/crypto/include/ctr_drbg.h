#ifndef CTR_DRBH_H
#define CTR_DRBH_H

#include <stdint.h>

#define OUTLEN  16
#define KEYLEN  16
#define SEEDLEN (OUTLEN + KEYLEN)

#ifndef RANDLEN
#define RANDLEN 16
#endif

#define AJ_BLOCKSZ 16

typedef struct _CTR_DRBG_CTX {
        uint8_t df;
        uint8_t v[OUTLEN];
        uint8_t k[KEYLEN];
} CTR_DRBG_CTX;

extern int AES_CTR_DRBG_Reseed(CTR_DRBG_CTX *ctx, const uint8_t *seed);
extern int AES_CTR_DRBG_Instantiate(CTR_DRBG_CTX *ctx, const uint8_t *entropy_input, const uint8_t *personalization);
extern int AES_CTR_DRBG_Generate(CTR_DRBG_CTX *ctx, uint8_t *rand);

#endif
