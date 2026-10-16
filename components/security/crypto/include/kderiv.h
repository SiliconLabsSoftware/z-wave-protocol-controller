#ifndef KDERIV_H
#define KDERIV_H

#include <stdint.h>

/**
 * \ingroup crypto
 * @{
 */

/*
 * Input:
 * - NetworkKey is obtained during the key exchange phase.
 * - Constant NK = 0x55 repeated 15 times.
 *
 * Output:
 * - T0 = empty string
 * - T1 = CCM key, K(C) = CMAC(NetworkKey, T0 | Constant(NK) | 0x01)
 * - T2 = nonce personalization string 1,
 *        K(NONCE) = CMAC(NetworkKey, T1 | Constant(NK) | 0x02)
 * - T3 = nonce personalization string 2,
 *        K(NONCE) = CMAC(NetworkKey, T2 | Constant(NK) | 0x03)
 * - T4 = MPAN key, K(MPAN) = CMAC(NetworkKey, T3 | Constant(NK) | 0x04)
 */
/**
 * Expand a network key into a CCM key, nonce personalization string, and MPAN key.
 * \param[in] key_id Must be ZWAVE_KEY_ID_NONE; retained for API compatibility.
 * \param[in] network_key The 16-byte network key.
 * \param[out] ccm_key The 16-byte CCM key.
 * \param[out] nonce_pstring The 32-byte SPAN nonce personalization string.
 * \param[out] mpan_key The 16-byte MPAN key.
 * \return 1 on success, 0 on failure.
 */
extern int networkkey_expand(uint32_t key_id, const uint8_t *network_key, uint8_t *ccm_key, uint8_t *nonce_pstring, uint8_t *mpan_key);

/**
 * Extract temporary pseudo-random key material.
 *
 * Input:
 * - ConstantPRK = 0x33 repeated 16 times.
 * - ECDH shared secret is the output of the ECDH key exchange.
 * - AuthTag = ECDH controller public key | ECDH device public key.
 *
 * Output:
 * - PRK = CMAC(ConstantPRK, ECDH shared secret | AuthTag).
 *
 * \param[in] ecdh_share_secret The 32-byte ECDH shared secret.
 * \param[in] auth_tag The 64-byte authentication tag.
 * \param[out] pseudo_random_keymat_output The 16-byte pseudo-random key.
 * \return 1 on success, 0 on failure.
 */
extern int tempkey_extract(const uint8_t *const ecdh_share_secret, const uint8_t *const auth_tag, uint8_t *pseudo_random_keymat_output);

/*
 * Input:
 * - PRK is defined by tempkey_extract().
 * - Constant TE = 0x88 repeated 15 times.
 *
 * Output:
 * - T0 = empty string
 * - T1 = CCM key, K(C) = CMAC(PRK, T0 | Constant(TE) | 0x01)
 * - T2 = nonce personalization string 1,
 *        K(NONCE) = CMAC(PRK, T1 | Constant(TE) | 0x02)
 * - T3 = nonce personalization string 2,
 *        K(NONCE) = CMAC(PRK, T2 | Constant(TE) | 0x03)
 * - T4 = MPAN key, K(MPAN) = CMAC(PRK, T3 | Constant(TE) | 0x04)
 */
/**
 * Expand temporary pseudo-random key material.
 * \param[in] key_id Must be ZWAVE_KEY_ID_NONE; retained for API compatibility.
 * \param[in] prk The 16-byte pseudo-random key.
 * \param[out] temp_ccm_key The 16-byte temporary CCM key.
 * \param[out] temp_nonce_pstring The 32-byte temporary nonce personalization string.
 * \param[out] temp_mpan_key The 16-byte temporary MPAN key.
 * \return 1 on success, 0 on failure.
 */
extern int tempkey_expand(uint32_t key_id, const uint8_t *prk, uint8_t *temp_ccm_key, uint8_t *temp_nonce_pstring, uint8_t *temp_mpan_key);

/**
 * @}
 */

#endif
