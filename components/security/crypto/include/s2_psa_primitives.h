#ifndef S2_PSA_PRIMITIVES_H_
#define S2_PSA_PRIMITIVES_H_

#include <stddef.h>
#include <stdint.h>

#include "psa/crypto.h"

/** Idempotent; safe to call from process init and before any PSA primitive use. */
psa_status_t s2_psa_crypto_init(void);

psa_status_t s2_psa_aes128_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);
psa_status_t s2_psa_aes_cmac(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac);

#endif
