#ifndef CRYPTO_MOCK_PSA_WRAPPERS_HPP
#define CRYPTO_MOCK_PSA_WRAPPERS_HPP

#include <gmock/gmock.h>

#include "psa_wrappers_interface.hpp"

namespace crypto::test
{
    class MockPsaWrappers : public PsaWrappersInterface
    {
        public:
            MOCK_METHOD(psa_status_t, crypto_init, (), (override));

            MOCK_METHOD(psa_status_t, import_key, (psa_key_usage_t usage, psa_algorithm_t algorithm, std::span<const uint8_t> key, psa_key_id_t &key_id, psa_key_type_t type, size_t bits), (override));
            MOCK_METHOD(psa_status_t, export_public_key, (psa_key_id_t key_id, std::span<uint8_t> data, size_t &data_length), (override));
            MOCK_METHOD(psa_status_t, raw_key_agreement, (psa_algorithm_t algorithm, psa_key_id_t private_key, std::span<const uint8_t> peer_key, std::span<uint8_t> output, size_t &output_length), (override));
            MOCK_METHOD(psa_status_t, destroy_key, (psa_key_id_t key_id), (override));

            MOCK_METHOD(psa_status_t, cipher_encrypt, (psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length), (override));
            MOCK_METHOD(psa_status_t, cipher_decrypt, (psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length), (override));
            MOCK_METHOD(psa_status_t, cipher_encrypt_setup, (psa_cipher_operation_t & operation, psa_key_id_t key_id, psa_algorithm_t algorithm), (override));
            MOCK_METHOD(psa_status_t, cipher_set_iv, (psa_cipher_operation_t & operation, std::span<const uint8_t> iv), (override));
            MOCK_METHOD(psa_status_t, cipher_update, (psa_cipher_operation_t & operation, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length), (override));
            MOCK_METHOD(psa_status_t, cipher_finish, (psa_cipher_operation_t & operation, std::span<uint8_t> output, size_t &output_length), (override));
            MOCK_METHOD(psa_status_t, cipher_abort, (psa_cipher_operation_t & operation), (override));

            MOCK_METHOD(psa_status_t, aead_encrypt, (psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> nonce, std::span<const uint8_t> additional_data, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_length), (override));
            MOCK_METHOD(psa_status_t, aead_decrypt, (psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> nonce, std::span<const uint8_t> additional_data, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_length), (override));

            MOCK_METHOD(psa_status_t, mac_compute, (psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> mac, size_t &mac_length), (override));

            MOCK_METHOD(psa_status_t, generate_random, (std::span<uint8_t> output), (override));
    };
}  // namespace crypto::test

#endif
