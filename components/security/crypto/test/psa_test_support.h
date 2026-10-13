#ifndef PSA_TEST_SUPPORT_H_
#define PSA_TEST_SUPPORT_H_

#include <stddef.h>
#include <stdint.h>

#include "psa/crypto.h"
#include "s2_psa_primitives.h"

/** Unity setUp/tearDown helpers (call mbedtls_psa_crypto_free in tearDown). */
void s2_psa_test_setUp(void);
void s2_psa_test_tearDown(void);

psa_status_t s2_psa_aes128_ecb_decrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);

#endif
