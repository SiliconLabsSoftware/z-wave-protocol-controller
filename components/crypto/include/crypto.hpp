#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <span>

#include "aes_ecb.h"
#include "aes_ccm.h"
#include "aes_cmac.h"
#include "aes_ofb.h"
#include "aes_cbc_mac.h"
#include "rand.h"
#include "x25519.h"
#include "aes_ctr_drbg.h"
#include "psa_wrappers.hpp"

namespace crypto
{
    class Crypto
    {
        public:
            // The dependency is not owned and must outlive this object.
            explicit Crypto(PsaWrappersInterface &psa_wrappers) : psa_wrappers_(psa_wrappers) {}

            // Production instance used by the C and free-function adapters.
            static Crypto &instance();

            sl_status_t aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> input, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> output) const;
            sl_status_t aes_ecb_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> input, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> output) const;
            sl_status_t aes_ccm_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_len) const;
            sl_status_t aes_ccm_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_len) const;
            sl_status_t aes_cmac_compute(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t> message, std::span<uint8_t, CRYPTO_AES_CMAC_OUTPUT_LEN> mac) const;
            sl_status_t aes_ofb_xor(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> iv, std::span<uint8_t> data) const;
            sl_status_t aes_cbc_mac(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> iv, std::span<const uint8_t> data, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> mac) const;
            sl_status_t generate_random(std::span<uint8_t> output) const;
            sl_status_t x25519_derive_public_key(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key, std::span<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key) const;
            sl_status_t x25519_derive_shared_secret(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key, std::span<const uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> peer_public_key, std::span<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> shared_secret) const;
            sl_status_t aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> entropy_input, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> pers_string) const;
            // out must have the output size configured for the linked DRBG library.
            // A dynamic span keeps this class identical in normal and KAT builds.
            sl_status_t aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<uint8_t> out) const;

        private:
            sl_status_t update_ctr_drbg(crypto_aes_ctr_drbg_ctx_t &ctx, const uint8_t *data) const;
            PsaWrappersInterface &psa_wrappers_;
    };
}  // namespace crypto

#endif
