#ifndef CRYPTO_MOCK_HPP
#define CRYPTO_MOCK_HPP

#include <gmock/gmock.h>

#include "crypto.hpp"

namespace crypto::test
{
    class CryptoMock : public CryptoInterface
    {
        public:
            MOCK_METHOD(sl_status_t, aes_ecb_encrypt, ((std::span<const uint8_t, crypto::aes_key_size>), (std::span<const uint8_t, crypto::aes_block_size>), (std::span<uint8_t, crypto::aes_block_size>)), (const, override));
            MOCK_METHOD(sl_status_t, aes_ecb_decrypt, ((std::span<const uint8_t, crypto::aes_key_size>), (std::span<const uint8_t, crypto::aes_block_size>), (std::span<uint8_t, crypto::aes_block_size>)), (const, override));

            MOCK_METHOD(sl_status_t, aes_ccm_encrypt, ((std::span<const uint8_t, crypto::aes_key_size>), size_t, std::span<const uint8_t>, std::span<const uint8_t>, std::span<const uint8_t>, std::span<uint8_t>, size_t &), (const, override));
            MOCK_METHOD(sl_status_t, aes_ccm_decrypt, ((std::span<const uint8_t, crypto::aes_key_size>), size_t, std::span<const uint8_t>, std::span<const uint8_t>, std::span<const uint8_t>, std::span<uint8_t>, size_t &), (const, override));

            MOCK_METHOD(sl_status_t, aes_cmac_compute, ((std::span<const uint8_t, crypto::aes_key_size>), std::span<const uint8_t>, (std::span<uint8_t, crypto::aes_cmac_output_len>)), (const, override));

            MOCK_METHOD(sl_status_t, aes_ofb_xor, ((std::span<const uint8_t, crypto::aes_key_size>), (std::span<const uint8_t, crypto::aes_block_size>), std::span<uint8_t>), (const, override));

            MOCK_METHOD(sl_status_t, aes_cbc_mac, ((std::span<const uint8_t, crypto::aes_key_size>), (std::span<const uint8_t, crypto::aes_block_size>), std::span<const uint8_t>, (std::span<uint8_t, crypto::aes_block_size>)), (const, override));

            MOCK_METHOD(sl_status_t, generate_random, (std::span<uint8_t>), (const, override));

            MOCK_METHOD(sl_status_t, x25519_derive_public_key, ((std::span<const uint8_t, crypto::x25519_private_key_size>), (std::span<uint8_t, crypto::x25519_public_key_size>)), (const, override));
            MOCK_METHOD(sl_status_t, x25519_derive_shared_secret, ((std::span<const uint8_t, crypto::x25519_private_key_size>), (std::span<const uint8_t, crypto::x25519_public_key_size>), (std::span<uint8_t, crypto::x25519_shared_secret_size>)), (const, override));

            MOCK_METHOD(sl_status_t, aes_ctr_drbg_init, (crypto::CtrDrbgContext &, (std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size>), (std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size>)), (const, override));
            MOCK_METHOD(sl_status_t, aes_ctr_drbg_generate, (crypto::CtrDrbgContext &, std::span<uint8_t>), (const, override));
    };
}  // namespace crypto::test

#endif
