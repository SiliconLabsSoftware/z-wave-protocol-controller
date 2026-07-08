/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

/**
 * @brief One-shot anonymous public-key encryption to an X25519 recipient.
 *
 * Construction (per envelope):
 *
 *   zpc_eph_priv, zpc_eph_pub := X25519_keygen()
 *   shared_secret             := X25519(zpc_eph_priv, recipient_pub_key)  // 32 B
 *   aes_key                   := HKDF-SHA256(IKM=shared_secret,
 *                                            salt=zpc_eph_pub || recipient_pub_key,
 *                                            info=HKDF_INFO,
 *                                            length=32)
 *   iv                        := CSPRNG(12 B)
 *   aad                       := magic || version || zpc_eph_pub || recipient_pub_key
 *   ciphertext, auth_tag      := AES-256-GCM-Encrypt(aes_key, iv, aad, plaintext)
 *
 *   file                      := magic(4) || version(1) || zpc_eph_pub(32) || iv(12) || ciphertext(N) || auth_tag(16)
 *
 * Each envelope uses a unique AES key derived from a fresh ephemeral keypair,
 * so the IV value need only be unique within that single encryption. Including
 * recipient_pub_key in the AAD binds the ciphertext to the intended recipient:
 * an attacker cannot present this envelope as having been encrypted to a
 * different public key.
 */

#ifndef SECURITY_SEAL_X25519_HPP
#define SECURITY_SEAL_X25519_HPP

#include "sl_status.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace security
{
    inline constexpr std::array<uint8_t, 4> SEAL_MAGIC = {'Z', 'P', 'C', 'K'};
    inline constexpr uint8_t SEAL_VERSION              = 0x01;

    inline constexpr std::size_t X25519_KEY_BYTES     = 32;
    inline constexpr std::size_t SHA256_DIGEST_LENGTH = 32;
    inline constexpr std::size_t AES_GCM_IV_BYTES     = 12;
    inline constexpr std::size_t AES_GCM_TAG_BYTES    = 16;
    inline constexpr std::size_t AES_GCM_KEY_BYTES    = 32;

    /// HKDF "info" string. Bumping this string (or the SEAL_VERSION byte) is
    /// the right way to deprecate an old format on a forward-compatible
    /// envelope reader.
    inline constexpr char HKDF_INFO[] = "zpc-keydump-v1";

    /**
     * @brief Seal plaintext to a recipient identified by an X25519 public
     *        key loaded from a PEM file.
     *
     * @param[in]  recipient_pubkey_pem_path Path to a PEM-encoded X25519
     *             public key.
     * @param[in]  plaintext     Pointer to plaintext bytes.
     * @param[in]  plaintext_len Length of plaintext.
     * @param[out] out_envelope  The sealed envelope length.
     * @param[out] out_recipient_fingerprint
     *             Optional. If non-null, receives the SHA-256 fingerprint
     *             of the raw 32-byte recipient public key. Intended to
     *             distinguish which recipient a given file was encrypted to.
     *
     * @return SL_STATUS_OK on success.
     * @return SL_STATUS_NULL_POINTER on invalid arguments.
     * @return SL_STATUS_NOT_FOUND if the PEM file cannot be opened.
     * @return SL_STATUS_INVALID_TYPE if the PEM is not an X25519 public key.
     * @return SL_STATUS_FAIL if the PEM cannot be parsed or the raw key extracted.
     * @return SL_STATUS_ABORT on ephemeral keygen, ECDH, HKDF, RNG, or AES-GCM failure.
     */
    sl_status_t seal_to_x25519_pem(const std::string &recipient_pubkey_pem_path, const uint8_t *plaintext, std::size_t plaintext_len, std::vector<uint8_t> &out_envelope, std::array<uint8_t, SHA256_DIGEST_LENGTH> *out_recipient_fingerprint = nullptr);
}  // namespace security

#endif  // SECURITY_SEAL_X25519_HPP

/** @} end seal_x25519 */
