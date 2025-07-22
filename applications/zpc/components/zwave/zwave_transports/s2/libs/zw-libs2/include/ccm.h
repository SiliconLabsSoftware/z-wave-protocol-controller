/* © 2017 Silicon Laboratories Inc.
 */
/**
 * @file ccm.h
 */
#ifndef ZWAVE_PSA_AES

#ifndef CCM_H_
#define CCM_H_

#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * \ingroup crypto
 * @{
 */

#ifdef CCM_USE_PREDEFINED_VALUES
#define KYO   (2)
#define Q_DEF KYO
#define N_DEF (15 - KYO)
#define T_DEF (8)
#endif

/**
  * CCM Encrypt the text_to_encrypt and authenticate AAD, adding auth tag to ciphertext.
  * \param key 16 bytes
  * \param nonce 16 bytes
  * \param aad Additional Authenticated Data (AAD)
  * \param aad_len Length of AAD
  * \param plain_ciphertext The plaintext to encrypt AND the output buffer for ciphertext
  * \param plaintext_len Plaintext length
  * \return The total length of the cipher text (including MAC)
  */
DllExport uint32_t CCM_encrypt_and_auth(const uint8_t *key,
                                        const uint8_t *nonce,
                                        const uint8_t *aad,
                                        const uint32_t aad_len,
                                        uint8_t *plain_ciphertext,
                                        const uint16_t plaintext_len);

/**
  * Decrypt and authenticate received ciphertext and AAD.
  * The decryption is performed in-place on the plain_ciphertext buffer.
  *
  * \return Length of plaintext if authentication OK or 0 when auth fails
  * \param key 16 bytes
  * \param nonce 16 or 32 bytes - JBU will check
  * \param aad Pointer to additional authenticated data (AAD)
  * \param aad_len Length of AAD
  * \param[inout] cipher_plaintext In-place buffer to decrypt ciphertext in
  * \param ciphertext_len length of cipher text.
  */
DllExport uint16_t CCM_decrypt_and_auth(const uint8_t *key,
                                        const uint8_t *nonce,
                                        const uint8_t *aad,
                                        const uint32_t aad_len,
                                        uint8_t *cipher_plaintext,
                                        const uint32_t ciphertext_len);

#ifndef CCM_USE_PREDEFINED_VALUES
void set_q_n_t(uint8_t q_in, uint8_t n_in, uint8_t t_in);
#endif
void get_q_n_t(uint8_t *q_out, uint8_t *n_out, uint8_t *t_out);
void get_q(uint8_t *q_out);

/**
 * @}
 */

#endif
#endif /* !defined (ZWAVE_PSA_SECURE_VAULT) */
