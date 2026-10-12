#ifndef PSA_TEST_SUPPORT_H_
#define PSA_TEST_SUPPORT_H_

#include <stddef.h>
#include <stdint.h>

#include "psa/crypto.h"

/** Unity setUp/tearDown helpers (call mbedtls_psa_crypto_free in tearDown). */
void s2_psa_test_setUp(void);
void s2_psa_test_tearDown(void);

psa_status_t s2_psa_aes128_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);
psa_status_t s2_psa_aes128_ecb_decrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);

psa_status_t s2_psa_aes_cmac(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac);

psa_status_t s2_psa_ccm_encrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_len);

psa_status_t s2_psa_ccm_decrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len);

psa_status_t s2_psa_x25519_public_from_private(const uint8_t *private_key, uint8_t *public_key);
psa_status_t s2_psa_x25519_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret);

#endif
