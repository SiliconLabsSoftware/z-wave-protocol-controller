#ifndef NEXTNONCE_H
#define NEXTNONCE_H

#include "ctr_drbg.h"

/**
 * \ingroup crypto
 * @{
 */

/**
 * Instantiate NextNonce.
 * \param[out] ctx The DRBG to instantiate and seed.
 * \param[in] ei_sender The sender's 16-byte entropy input.
 * \param[in] ei_receiver The receiver's 16-byte entropy input.
 * \param[in] k_nonce The 32-byte nonce personalization string.
 * \return 1 on success, 0 on failure.
 */
extern int next_nonce_instantiate(CTR_DRBG_CTX *ctx, const uint8_t *ei_sender, const uint8_t *ei_receiver, uint8_t *k_nonce);

/**
 * Generate 16 bytes of nonce output.
 * \param[in,out] ctx The DRBG to generate from.
 * \param[out] rand The 16-byte output buffer.
 * \return 1 on success, 0 on failure.
 */
extern int next_nonce_generate(CTR_DRBG_CTX *ctx, uint8_t *rand);

/**
 * @}
 */

#endif
