#ifndef CRYPTO_IMPL_HPP
#define CRYPTO_IMPL_HPP

#include "crypto.hpp"

namespace crypto
{
    class PsaWrappersInterface;

    class Crypto : public CryptoInterface
    {
        public:
            explicit Crypto(PsaWrappersInterface &psa_wrappers) : psa_wrappers_(psa_wrappers) {}

            sl_status_t aes_ecb_encrypt(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> input, std::span<uint8_t, crypto::aes_block_size> output) const override;
            sl_status_t aes_ecb_decrypt(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> input, std::span<uint8_t, crypto::aes_block_size> output) const override;

            sl_status_t aes_ccm_encrypt(std::span<const uint8_t, crypto::aes_key_size> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_len) const override;
            sl_status_t aes_ccm_decrypt(std::span<const uint8_t, crypto::aes_key_size> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_len) const override;

            sl_status_t aes_cmac_compute(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t> message, std::span<uint8_t, crypto::aes_cmac_output_len> mac) const override;

            sl_status_t aes_ofb_xor(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> iv, std::span<uint8_t> data) const override;

            sl_status_t aes_cbc_mac(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> iv, std::span<const uint8_t> data, std::span<uint8_t, crypto::aes_block_size> mac) const override;

            sl_status_t generate_random(std::span<uint8_t> output) const override;

            sl_status_t x25519_derive_public_key(std::span<const uint8_t, crypto::x25519_private_key_size> private_key, std::span<uint8_t, crypto::x25519_public_key_size> public_key) const override;
            sl_status_t x25519_derive_shared_secret(std::span<const uint8_t, crypto::x25519_private_key_size> private_key, std::span<const uint8_t, crypto::x25519_public_key_size> peer_public_key, std::span<uint8_t, crypto::x25519_shared_secret_size> shared_secret) const override;

            sl_status_t aes_ctr_drbg_init(crypto::CtrDrbgContext &ctx, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size> entropy_input, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size> pers_string) const override;
            sl_status_t aes_ctr_drbg_generate(crypto::CtrDrbgContext &ctx, std::span<uint8_t> out) const override;

        private:
            sl_status_t update_ctr_drbg(crypto::CtrDrbgContext &ctx, const uint8_t *data) const;
            PsaWrappersInterface &psa_wrappers_;
    };
}  // namespace crypto

#endif
