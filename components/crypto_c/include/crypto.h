#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRYPTO_AES_KEY_BITS   128U
#define CRYPTO_AES_KEY_SIZE   16U
#define CRYPTO_AES_BLOCK_SIZE 16U

#define CRYPTO_AES_CMAC_OUTPUT_LEN CRYPTO_AES_BLOCK_SIZE

#define CRYPTO_AES_CTR_DRBG_SEED_SIZE   (CRYPTO_AES_BLOCK_SIZE + CRYPTO_AES_KEY_SIZE)
#define CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE 16U

#define CRYPTO_X25519_KEY_BITS           255U
#define CRYPTO_X25519_PRIVATE_KEY_SIZE   32U
#define CRYPTO_X25519_PUBLIC_KEY_SIZE    32U
#define CRYPTO_X25519_SHARED_SECRET_SIZE 32U

typedef struct crypto_aes_ctr_drbg_ctx {
        uint8_t counter[CRYPTO_AES_BLOCK_SIZE];
        uint8_t key[CRYPTO_AES_KEY_SIZE];
} crypto_aes_ctr_drbg_ctx_t;

sl_status_t crypto_aes_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);

sl_status_t crypto_aes_cmac_compute(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac);

sl_status_t crypto_aes_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *data, uint8_t len, uint8_t *mac);

sl_status_t crypto_aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input, const uint8_t *pers_string);
sl_status_t crypto_aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t *ctx, uint8_t *out);

sl_status_t crypto_generate_random(uint8_t *output, size_t output_size);

sl_status_t crypto_x25519_derive_public_key(const uint8_t *private_key, uint8_t *public_key);
sl_status_t crypto_x25519_derive_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret);

sl_status_t crypto_aes_ccm_encrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_len);
sl_status_t crypto_aes_ccm_decrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len);

sl_status_t crypto_aes_ofb_xor(const uint8_t *key, uint8_t iv[16], uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
