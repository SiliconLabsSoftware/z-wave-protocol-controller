#ifndef ZWAVE_S2_KEY_DERIVATION_H
#define ZWAVE_S2_KEY_DERIVATION_H

#include <stdint.h>

#include "sl_status.h"

/**
 * @brief Extract the S2 pseudo-random key from an ECDH shared secret.
 *
 * @param ecdh_shared_secret 32-byte X25519 shared secret; must not be NULL.
 * @param public_keys 64-byte concatenation KeyPub_A || KeyPub_B; must not be NULL.
 * @param[out] pseudo_random_key Buffer receiving 16 bytes; must not be NULL and is unchanged on failure.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER for a null argument, otherwise a crypto error status.
 */
sl_status_t zwave_s2_extract_pseudo_random_key(const uint8_t *ecdh_shared_secret, const uint8_t *public_keys, uint8_t *pseudo_random_key);

/**
 * @brief Derive S2 temporary CCM, nonce, and MPAN key material.
 *
 * Outputs are unchanged on failure and must not overlap one another.
 *
 * @param pseudo_random_key 16-byte pseudo-random key; must not be NULL.
 * @param[out] ccm_key Buffer receiving the 16-byte CCM key; must not be NULL.
 * @param[out] nonce_pers_string Buffer receiving the 32-byte nonce personalization string; must not be NULL.
 * @param[out] mpan_key Buffer receiving the 16-byte MPAN key; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER for a null argument, otherwise a crypto error status.
 */
sl_status_t zwave_s2_derive_temp_key(const uint8_t *pseudo_random_key, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key);

/**
 * @brief Derive S2 network CCM, nonce, and MPAN key material.
 *
 * Outputs are unchanged on failure and must not overlap one another.
 *
 * @param network_key 16-byte network key; must not be NULL.
 * @param[out] ccm_key Buffer receiving the 16-byte CCM key; must not be NULL.
 * @param[out] nonce_pers_string Buffer receiving the 32-byte nonce personalization string; must not be NULL.
 * @param[out] mpan_key Buffer receiving the 16-byte MPAN key; must not be NULL.
 * @return SL_STATUS_OK on success, SL_STATUS_NULL_POINTER for a null argument, otherwise a crypto error status.
 */
sl_status_t zwave_s2_derive_network_key(const uint8_t *network_key, uint8_t *ccm_key, uint8_t *nonce_pers_string, uint8_t *mpan_key);

#endif
