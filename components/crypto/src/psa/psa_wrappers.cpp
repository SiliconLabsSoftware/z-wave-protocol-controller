#include "psa_wrappers.hpp"

namespace crypto
{

    psa_status_t PsaWrappers::crypto_init()
    {
        return psa_crypto_init();
    }

    psa_status_t PsaWrappers::import_key(psa_key_usage_t usage, psa_algorithm_t algorithm, std::span<const uint8_t> key, psa_key_id_t &key_id, psa_key_type_t type, size_t bits)
    {
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

        psa_set_key_usage_flags(&attributes, usage);
        psa_set_key_algorithm(&attributes, algorithm);
        psa_set_key_type(&attributes, type);
        psa_set_key_bits(&attributes, bits);

        return psa_import_key(&attributes, key.data(), key.size(), &key_id);
    }

    psa_status_t PsaWrappers::export_public_key(psa_key_id_t key_id, std::span<uint8_t> data, size_t &data_length)
    {
        return psa_export_public_key(key_id, data.data(), data.size(), &data_length);
    }

    psa_status_t PsaWrappers::raw_key_agreement(psa_algorithm_t algorithm, psa_key_id_t private_key, std::span<const uint8_t> peer_key, std::span<uint8_t> output, size_t &output_length)
    {
        return psa_raw_key_agreement(algorithm, private_key, peer_key.data(), peer_key.size(), output.data(), output.size(), &output_length);
    }

    psa_status_t PsaWrappers::destroy_key(psa_key_id_t key_id)
    {
        return psa_destroy_key(key_id);
    }

    psa_status_t PsaWrappers::cipher_encrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length)
    {
        return psa_cipher_encrypt(key_id, algorithm, input.data(), input.size(), output.data(), output.size(), &output_length);
    }

    psa_status_t PsaWrappers::cipher_decrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length)
    {
        return psa_cipher_decrypt(key_id, algorithm, input.data(), input.size(), output.data(), output.size(), &output_length);
    }

    psa_status_t PsaWrappers::cipher_encrypt_setup(psa_cipher_operation_t &operation, psa_key_id_t key_id, psa_algorithm_t algorithm)
    {
        return psa_cipher_encrypt_setup(&operation, key_id, algorithm);
    }

    psa_status_t PsaWrappers::cipher_set_iv(psa_cipher_operation_t &operation, std::span<const uint8_t> iv)
    {
        return psa_cipher_set_iv(&operation, iv.data(), iv.size());
    }

    psa_status_t PsaWrappers::cipher_update(psa_cipher_operation_t &operation, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length)
    {
        return psa_cipher_update(&operation, input.data(), input.size(), output.data(), output.size(), &output_length);
    }

    psa_status_t PsaWrappers::cipher_finish(psa_cipher_operation_t &operation, std::span<uint8_t> output, size_t &output_length)
    {
        return psa_cipher_finish(&operation, output.data(), output.size(), &output_length);
    }

    psa_status_t PsaWrappers::cipher_abort(psa_cipher_operation_t &operation)
    {
        return psa_cipher_abort(&operation);
    }

    psa_status_t PsaWrappers::aead_encrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> nonce, std::span<const uint8_t> additional_data, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_length)
    {
        return psa_aead_encrypt(key_id, algorithm, nonce.data(), nonce.size(), additional_data.data(), additional_data.size(), plaintext.data(), plaintext.size(), ciphertext.data(), ciphertext.size(), &ciphertext_length);
    }

    psa_status_t PsaWrappers::aead_decrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> nonce, std::span<const uint8_t> additional_data, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_length)
    {
        return psa_aead_decrypt(key_id, algorithm, nonce.data(), nonce.size(), additional_data.data(), additional_data.size(), ciphertext.data(), ciphertext.size(), plaintext.data(), plaintext.size(), &plaintext_length);
    }

    psa_status_t PsaWrappers::mac_compute(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> mac, size_t &mac_length)
    {
        return psa_mac_compute(key_id, algorithm, input.data(), input.size(), mac.data(), mac.size(), &mac_length);
    }

    psa_status_t PsaWrappers::generate_random(std::span<uint8_t> output)
    {
        return psa_generate_random(output.data(), output.size());
    }

}  // namespace crypto
