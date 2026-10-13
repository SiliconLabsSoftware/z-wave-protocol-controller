#ifndef NEXTNONCE_H
#define NEXTNONCE_H

#include "ctr_drbg.h"

extern void next_nonce_instantiate(CTR_DRBG_CTX *ctx, const uint8_t *ei_sender, const uint8_t *ei_receiver, uint8_t *k_nonce);
extern int next_nonce_generate(CTR_DRBG_CTX *ctx, uint8_t *rand);

#endif
