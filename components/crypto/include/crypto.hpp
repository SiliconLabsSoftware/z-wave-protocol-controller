#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "sl_status.h"

namespace crypto
{

    inline constexpr std::size_t aes_key_bits              = 128;
    inline constexpr std::size_t aes_key_size              = 16;
    inline constexpr std::size_t aes_block_size            = 16;
    inline constexpr std::size_t aes_cmac_output_len       = aes_block_size;
    inline constexpr std::size_t aes_ctr_drbg_seed_size    = aes_block_size + aes_key_size;
    inline constexpr std::size_t aes_ctr_drbg_output_size  = 16;
    inline constexpr std::size_t x25519_key_bits           = 255;
    inline constexpr std::size_t x25519_private_key_size   = 32;
    inline constexpr std::size_t x25519_public_key_size    = 32;
    inline constexpr std::size_t x25519_shared_secret_size = 32;

    struct CtrDrbgContext {
            std::array<std::uint8_t, aes_block_size> counter;
            std::array<std::uint8_t, aes_key_size> key;
    };

    /**
     * @brief Cryptographic operations using byte spans for inputs and outputs.
     *
     * Output buffers and lengths are valid only on success unless stated otherwise.
     */
    class CryptoInterface
    {
        public:
            virtual ~CryptoInterface() = default;

            /**
             * @brief Encrypt one block using AES-128 ECB without padding.
             *
             * @param key 16-byte encryption key.
             * @param input 16-byte plaintext block.
             * @param[out] output Buffer receiving the 16-byte ciphertext.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t aes_ecb_encrypt(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> input, std::span<std::uint8_t, aes_block_size> output) const = 0;

            /**
             * @brief Decrypt one block using AES-128 ECB without padding.
             *
             * @param key 16-byte encryption key.
             * @param input 16-byte ciphertext block.
             * @param[out] output Buffer receiving the 16-byte plaintext.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t aes_ecb_decrypt(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> input, std::span<std::uint8_t, aes_block_size> output) const = 0;

            /**
             * @brief Encrypt and authenticate a message using AES-128 CCM.
             *
             * The ciphertext includes the authentication tag at the end. Input and output may share the same buffer.
             *
             * @param key 16-byte encryption key.
             * @param tag_len Authentication tag length in bytes; an even number from 4 to 16.
             * @param nonce Nonce of 7 to 13 bytes.
             * @param aad Additional authenticated data; may be empty.
             * @param plaintext Plaintext bytes; may be empty.
             * @param[out] ciphertext Output buffer of at least plaintext.size() + tag_len bytes.
             * @param[out] ciphertext_len Bytes written, including the appended tag, on success.
             * @return SL_STATUS_OK on success, SL_STATUS_WOULD_OVERFLOW if the output is too small, otherwise an error status.
             */
            virtual sl_status_t
              aes_ccm_encrypt(std::span<const std::uint8_t, aes_key_size> key, std::size_t tag_len, std::span<const std::uint8_t> nonce, std::span<const std::uint8_t> aad, std::span<const std::uint8_t> plaintext, std::span<std::uint8_t> ciphertext, std::size_t &ciphertext_len) const
              = 0;

            /**
             * @brief Decrypt and verify a message using AES-128 CCM.
             *
             * Input and output may share the same buffer. Use plaintext only after successful authentication.
             *
             * @param key 16-byte encryption key.
             * @param tag_len Authentication tag length in bytes; an even number from 4 to 16.
             * @param nonce Nonce of 7 to 13 bytes.
             * @param aad Additional authenticated data; may be empty.
             * @param ciphertext Ciphertext followed by its authentication tag.
             * @param[out] plaintext Output buffer of at least ciphertext.size() - tag_len bytes.
             * @param[out] plaintext_len Plaintext bytes written on success, excluding the tag.
             * @return SL_STATUS_OK on success, SL_STATUS_INVALID_SIGNATURE if authentication fails, otherwise an error status.
             */
            virtual sl_status_t aes_ccm_decrypt(std::span<const std::uint8_t, aes_key_size> key, std::size_t tag_len, std::span<const std::uint8_t> nonce, std::span<const std::uint8_t> aad, std::span<const std::uint8_t> ciphertext, std::span<std::uint8_t> plaintext, std::size_t &plaintext_len) const
              = 0;

            /**
             * @brief Compute an AES-128 CMAC.
             *
             * @param key 16-byte authentication key.
             * @param message Message bytes; may be empty.
             * @param[out] mac Buffer receiving the 16-byte MAC.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t aes_cmac_compute(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t> message, std::span<std::uint8_t, aes_cmac_output_len> mac) const = 0;

            /**
             * @brief Encrypt or decrypt data in place using AES-128 OFB.
             *
             * @param key 16-byte encryption key.
             * @param iv 16-byte initialization vector.
             * @param[in,out] data Bytes to XOR with the keystream; may be empty.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t aes_ofb_xor(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> iv, std::span<std::uint8_t> data) const = 0;

            /**
             * @brief Compute the AES-128 CBC-MAC used by S0.
             *
             * Encrypts iv followed by data in CBC mode, starting from an all-zero block. The MAC is the last encrypted block.
             *
             * @param key 16-byte authentication key.
             * @param iv 16-byte block processed before data.
             * @param data Message bytes; a partial last block is padded with zeros.
             * @param[out] mac Buffer receiving the final 16-byte encrypted block.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t aes_cbc_mac(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> iv, std::span<const std::uint8_t> data, std::span<std::uint8_t, aes_block_size> mac) const = 0;

            /**
             * @brief Fill a buffer with cryptographically secure random bytes.
             *
             * @param[out] output Buffer to fill; may be empty.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t generate_random(std::span<std::uint8_t> output) const = 0;

            /**
             * @brief Derive an X25519 public key from a private key.
             *
             * @param private_key 32-byte private key.
             * @param[out] public_key Buffer receiving the 32-byte public key.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t x25519_derive_public_key(std::span<const std::uint8_t, x25519_private_key_size> private_key, std::span<std::uint8_t, x25519_public_key_size> public_key) const = 0;

            /**
             * @brief Derive an X25519 shared secret for a peer.
             *
             * @param private_key 32-byte local private key.
             * @param peer_public_key 32-byte peer public key.
             * @param[out] shared_secret Buffer receiving the 32-byte shared secret.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t x25519_derive_shared_secret(std::span<const std::uint8_t, x25519_private_key_size> private_key, std::span<const std::uint8_t, x25519_public_key_size> peer_public_key, std::span<std::uint8_t, x25519_shared_secret_size> shared_secret) const = 0;

            /**
             * @brief Initialize the AES-128 CTR-DRBG state without a derivation function.
             *
             * @param[out] ctx Context initialized on success; unchanged on failure.
             * @param entropy_input 32-byte entropy input.
             * @param pers_string 32-byte personalization string, XORed with the entropy input.
             * @return SL_STATUS_OK on success, otherwise an error status.
             */
            virtual sl_status_t aes_ctr_drbg_init(CtrDrbgContext &ctx, std::span<const std::uint8_t, aes_ctr_drbg_seed_size> entropy_input, std::span<const std::uint8_t, aes_ctr_drbg_seed_size> pers_string) const = 0;

            /**
             * @brief Generate 16 bytes and advance the AES-128 CTR-DRBG state.
             *
             * @param[in,out] ctx Initialized context; unchanged on failure.
             * @param[out] out Buffer of exactly 16 bytes; unchanged on failure.
             * @return SL_STATUS_OK on success, SL_STATUS_INVALID_PARAMETER for an invalid output span, otherwise an error status.
             */
            virtual sl_status_t aes_ctr_drbg_generate(CtrDrbgContext &ctx, std::span<std::uint8_t> out) const = 0;
    };

    /**
     * @brief Get the shared crypto implementation, built on PSA Crypto.
     * @return Reference to a single instance that lives until the program exits.
     */
    CryptoInterface &default_crypto();

}  // namespace crypto

#endif
