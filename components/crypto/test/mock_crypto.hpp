#ifndef MOCK_CRYPTO_HPP
#define MOCK_CRYPTO_HPP

#include <gmock/gmock.h>

#include "crypto.hpp"

namespace crypto::test
{
    class MockCrypto : public CryptoInterface
    {
        public:
            MOCK_METHOD(sl_status_t, aes_ecb_encrypt, ((std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>), (std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>), (std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>)), (const, override));
            MOCK_METHOD(sl_status_t, aes_ecb_decrypt, ((std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>), (std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>), (std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>)), (const, override));
            MOCK_METHOD(sl_status_t, aes_ccm_encrypt, ((std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>), size_t, std::span<const uint8_t>, std::span<const uint8_t>, std::span<const uint8_t>, std::span<uint8_t>, size_t &), (const, override));
            MOCK_METHOD(sl_status_t, aes_ccm_decrypt, ((std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>), size_t, std::span<const uint8_t>, std::span<const uint8_t>, std::span<const uint8_t>, std::span<uint8_t>, size_t &), (const, override));
            MOCK_METHOD(sl_status_t, aes_cmac_compute, ((std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>), std::span<const uint8_t>, (std::span<uint8_t, CRYPTO_AES_CMAC_OUTPUT_LEN>)), (const, override));
            MOCK_METHOD(sl_status_t, aes_ofb_xor, ((std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>), (std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>), std::span<uint8_t>), (const, override));
            MOCK_METHOD(sl_status_t, aes_cbc_mac, ((std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>), (std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>), std::span<const uint8_t>, (std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>)), (const, override));
            MOCK_METHOD(sl_status_t, generate_random, (std::span<uint8_t>), (const, override));
            MOCK_METHOD(sl_status_t, x25519_derive_public_key, ((std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE>), (std::span<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE>)), (const, override));
            MOCK_METHOD(sl_status_t, x25519_derive_shared_secret, ((std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE>), (std::span<const uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE>), (std::span<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE>)), (const, override));
            MOCK_METHOD(sl_status_t, aes_ctr_drbg_init, (crypto_aes_ctr_drbg_ctx_t &, (std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE>), (std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE>)), (const, override));
            MOCK_METHOD(sl_status_t, aes_ctr_drbg_generate, (crypto_aes_ctr_drbg_ctx_t &, std::span<uint8_t>), (const, override));
    };
}  // namespace crypto::test

#endif
