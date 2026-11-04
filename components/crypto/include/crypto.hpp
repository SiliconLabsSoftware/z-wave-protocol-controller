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

    class CryptoInterface
    {
        public:
            virtual ~CryptoInterface() = default;

            virtual sl_status_t aes_ecb_encrypt(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> input, std::span<std::uint8_t, aes_block_size> output) const = 0;
            virtual sl_status_t aes_ecb_decrypt(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> input, std::span<std::uint8_t, aes_block_size> output) const = 0;

            virtual sl_status_t
              aes_ccm_encrypt(std::span<const std::uint8_t, aes_key_size> key, std::size_t tag_len, std::span<const std::uint8_t> nonce, std::span<const std::uint8_t> aad, std::span<const std::uint8_t> plaintext, std::span<std::uint8_t> ciphertext, std::size_t &ciphertext_len) const
              = 0;
            virtual sl_status_t aes_ccm_decrypt(std::span<const std::uint8_t, aes_key_size> key, std::size_t tag_len, std::span<const std::uint8_t> nonce, std::span<const std::uint8_t> aad, std::span<const std::uint8_t> ciphertext, std::span<std::uint8_t> plaintext, std::size_t &plaintext_len) const
              = 0;

            virtual sl_status_t aes_cmac_compute(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t> message, std::span<std::uint8_t, aes_cmac_output_len> mac) const = 0;

            virtual sl_status_t aes_ofb_xor(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> iv, std::span<std::uint8_t> data) const = 0;

            virtual sl_status_t aes_cbc_mac(std::span<const std::uint8_t, aes_key_size> key, std::span<const std::uint8_t, aes_block_size> iv, std::span<const std::uint8_t> data, std::span<std::uint8_t, aes_block_size> mac) const = 0;

            virtual sl_status_t generate_random(std::span<std::uint8_t> output) const = 0;

            virtual sl_status_t x25519_derive_public_key(std::span<const std::uint8_t, x25519_private_key_size> private_key, std::span<std::uint8_t, x25519_public_key_size> public_key) const                                                                                 = 0;
            virtual sl_status_t x25519_derive_shared_secret(std::span<const std::uint8_t, x25519_private_key_size> private_key, std::span<const std::uint8_t, x25519_public_key_size> peer_public_key, std::span<std::uint8_t, x25519_shared_secret_size> shared_secret) const = 0;

            virtual sl_status_t aes_ctr_drbg_init(CtrDrbgContext &ctx, std::span<const std::uint8_t, aes_ctr_drbg_seed_size> entropy_input, std::span<const std::uint8_t, aes_ctr_drbg_seed_size> pers_string) const = 0;
            virtual sl_status_t aes_ctr_drbg_generate(CtrDrbgContext &ctx, std::span<std::uint8_t> out) const                                                                                                        = 0;
    };

    CryptoInterface &default_crypto();

}  // namespace crypto

#endif
