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

/**
 * @brief Encrypt one block using AES-128 ECB without padding.
 *
 * @param key 16-byte encryption key; must not be NULL.
 * @param input 16-byte plaintext block; must not be NULL.
 * @param[out] output Buffer receiving the 16-byte ciphertext; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_aes_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output);

/**
 * @brief Compute an AES-128 CMAC.
 *
 * @param key 16-byte authentication key; must not be NULL.
 * @param message Message bytes; may be NULL when message_length is zero.
 * @param message_length Message length in bytes.
 * @param[out] mac Buffer receiving the 16-byte MAC; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_aes_cmac_compute(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac);

/**
 * @brief Compute the AES-128 CBC-MAC used by S0.
 *
 * Encrypts iv followed by data in CBC mode, starting from an all-zero block. The MAC is the last encrypted block.
 *
 * @param key 16-byte authentication key; must not be NULL.
 * @param iv 16-byte block processed before data; must not be NULL.
 * @param data Message bytes; may be NULL when len is zero.
 * @param len Message length in bytes; a partial last block is padded with zeros.
 * @param[out] mac Buffer receiving the 16-byte MAC; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_aes_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *mac);

/**
 * @brief Initialize the AES-128 CTR-DRBG state without a derivation function.
 *
 * @param[out] ctx Context initialized on success; must not be NULL and is unchanged on failure.
 * @param entropy_input 32-byte entropy input; must not be NULL.
 * @param pers_string 32-byte personalization string, XORed with entropy_input; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input, const uint8_t *pers_string);

/**
 * @brief Generate 16 bytes and advance the AES-128 CTR-DRBG state.
 *
 * The context and output are unchanged on failure.
 *
 * @param[in,out] ctx Initialized context; must not be NULL.
 * @param[out] out Buffer receiving 16 bytes; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t *ctx, uint8_t *out);

/**
 * @brief Fill a buffer with cryptographically secure random bytes.
 *
 * @param[out] output Buffer to fill; may be NULL when output_size is zero.
 * @param output_size Number of random bytes to generate.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_generate_random(uint8_t *output, size_t output_size);

/**
 * @brief Derive an X25519 public key from a private key.
 *
 * @param private_key 32-byte private key; must not be NULL.
 * @param[out] public_key Buffer receiving the 32-byte public key; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_x25519_derive_public_key(const uint8_t *private_key, uint8_t *public_key);

/**
 * @brief Derive an X25519 shared secret for a peer.
 *
 * @param private_key 32-byte local private key; must not be NULL.
 * @param peer_public_key 32-byte peer public key; must not be NULL.
 * @param[out] shared_secret Buffer receiving the 32-byte shared secret; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_x25519_derive_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret);

/**
 * @brief Encrypt and authenticate a message using AES-128 CCM.
 *
 * Input and output may share the same buffer. Output contents and length are valid only on success.
 *
 * @param key 16-byte encryption key; must not be NULL.
 * @param tag_len Authentication tag length in bytes; an even number from 4 to 16.
 * @param nonce Nonce bytes; must not be NULL.
 * @param nonce_len Nonce length in bytes; CCM requires 7 to 13.
 * @param aad Additional authenticated data; may be NULL when aad_len is zero.
 * @param aad_len Additional authenticated data length in bytes.
 * @param plaintext Plaintext bytes; may be NULL when plaintext_len is zero.
 * @param plaintext_len Plaintext length in bytes.
 * @param[out] ciphertext Buffer receiving ciphertext followed by the tag; must not be NULL.
 * @param ciphertext_size Output capacity in bytes; at least plaintext_len + tag_len.
 * @param[out] ciphertext_len Bytes written, including the appended tag, on success; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_aes_ccm_encrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_len);

/**
 * @brief Decrypt and verify a message using AES-128 CCM.
 *
 * Input and output may share the same buffer. Use plaintext and its length only after successful authentication.
 *
 * @param key 16-byte encryption key; must not be NULL.
 * @param tag_len Authentication tag length in bytes; an even number from 4 to 16.
 * @param nonce Nonce bytes; must not be NULL.
 * @param nonce_len Nonce length in bytes; CCM requires 7 to 13.
 * @param aad Additional authenticated data; may be NULL when aad_len is zero.
 * @param aad_len Additional authenticated data length in bytes.
 * @param ciphertext Ciphertext followed by its tag; must not be NULL.
 * @param ciphertext_len Input length in bytes, including the tag; at least tag_len.
 * @param[out] plaintext Output buffer; may be NULL only when plaintext_size is zero.
 * @param plaintext_size Output capacity in bytes; at least ciphertext_len - tag_len.
 * @param[out] plaintext_len Plaintext bytes written on success; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, SL_STATUS_INVALID_SIGNATURE if authentication fails, otherwise an error status.
 */
sl_status_t crypto_aes_ccm_decrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len);

/**
 * @brief Encrypt or decrypt data in place using AES-128 OFB.
 *
 * @param key 16-byte encryption key; required when len is nonzero.
 * @param iv 16-byte initialization vector; required when len is nonzero.
 * @param[in,out] data Bytes to XOR with the keystream; required when len is nonzero.
 * @param len Data length in bytes; zero returns success without accessing any buffer.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if a required pointer is NULL, otherwise an error status.
 */
sl_status_t crypto_aes_ofb_xor(const uint8_t *key, const uint8_t iv[16], uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
