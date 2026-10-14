/* © 2014 Silicon Laboratories Inc.
 */

#include <string.h>
#include <stdint.h>

#include "aes_cmac.h"
#include "ctr_drbg.h"
#include "nextnonce.h"

static int ckdf_nonce0_expand(uint8_t *prk, uint8_t *mei)
{
    const uint8_t constant_ei[15] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
    uint8_t t0[32];
    uint8_t t1[16] = {0};
    uint8_t t2[16] = {0};

    memcpy(t0, constant_ei, 15);
    t0[15] = 0;

    memcpy(t0 + 16, constant_ei, 15);
    t0[16 + 15] = 1;
    if (!aes_cmac_calculate(prk, t0, 32, t1)) {
        return 0;
    }

    memcpy(t0, t1, 16);
    memcpy(t0 + 16, constant_ei, 15);
    t0[16 + 15] = 2;
    if (!aes_cmac_calculate(prk, t0, 32, t2)) {
        return 0;
    }

    memcpy(mei, t1, 16);
    memcpy(mei + 16, t2, 16);
    return 1;
}

int next_nonce_instantiate(CTR_DRBG_CTX *ctx, const uint8_t *ei_sender, const uint8_t *ei_receiver, uint8_t *k_nonce)
{
    const uint8_t constant_nonce[16] = {0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26};
    uint8_t temp[32];
    uint8_t X[16];
    uint8_t mei[32];

    memcpy(temp, ei_sender, 16);
    memcpy(temp + 16, ei_receiver, 16);
    if (!aes_cmac_calculate(constant_nonce, temp, 32, X) || !ckdf_nonce0_expand(X, mei)) {
        return 0;
    }

    return AES_CTR_DRBG_Instantiate(ctx, mei, k_nonce);
}

int next_nonce_generate(CTR_DRBG_CTX *ctx, uint8_t *rand)
{
    return AES_CTR_DRBG_Generate(ctx, rand);
}
