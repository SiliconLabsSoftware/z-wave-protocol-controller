#include "psa_wrappers.hpp"

namespace crypto
{

    psa_status_t PsaWrappers::cipher_encrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length)
    {
        return psa_cipher_encrypt(key_id, algorithm, input, input_length, output, output_size, output_length);
    }

    psa_status_t PsaWrappers::cipher_decrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length)
    {
        return psa_cipher_decrypt(key_id, algorithm, input, input_length, output, output_size, output_length);
    }

    psa_status_t PsaWrappers::aead_encrypt(psa_key_id_t key_id,
                                           psa_algorithm_t algorithm,
                                           const uint8_t *nonce,
                                           size_t nonce_length,
                                           const uint8_t *additional_data,
                                           size_t additional_data_length,
                                           const uint8_t *plaintext,
                                           size_t plaintext_length,
                                           uint8_t *ciphertext,
                                           size_t ciphertext_size,
                                           size_t *ciphertext_length)
    {
        return psa_aead_encrypt(key_id, algorithm, nonce, nonce_length, additional_data, additional_data_length, plaintext, plaintext_length, ciphertext, ciphertext_size, ciphertext_length);
    }

    psa_status_t PsaWrappers::aead_decrypt(psa_key_id_t key_id,
                                           psa_algorithm_t algorithm,
                                           const uint8_t *nonce,
                                           size_t nonce_length,
                                           const uint8_t *additional_data,
                                           size_t additional_data_length,
                                           const uint8_t *ciphertext,
                                           size_t ciphertext_length,
                                           uint8_t *plaintext,
                                           size_t plaintext_size,
                                           size_t *plaintext_length)
    {
        return psa_aead_decrypt(key_id, algorithm, nonce, nonce_length, additional_data, additional_data_length, ciphertext, ciphertext_length, plaintext, plaintext_size, plaintext_length);
    }

    psa_status_t PsaWrappers::mac_compute(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *mac, size_t mac_size, size_t *mac_length)
    {
        return psa_mac_compute(key_id, algorithm, input, input_length, mac, mac_size, mac_length);
    }

    psa_status_t PsaWrappers::generate_random(uint8_t *output, size_t output_size)
    {
        return psa_generate_random(output, output_size);
    }

    psa_status_t PsaWrappers::export_public_key(psa_key_id_t key_id, uint8_t *data, size_t data_size, size_t *data_length)
    {
        return psa_export_public_key(key_id, data, data_size, data_length);
    }

    psa_status_t PsaWrappers::raw_key_agreement(psa_algorithm_t algorithm, psa_key_id_t private_key, const uint8_t *peer_key, size_t peer_key_length, uint8_t *output, size_t output_size, size_t *output_length)
    {
        return psa_raw_key_agreement(algorithm, private_key, peer_key, peer_key_length, output, output_size, output_length);
    }

    psa_status_t PsaWrappers::cipher_finish(psa_cipher_operation_t *operation, uint8_t *output, size_t output_size, size_t *output_length)
    {
        return psa_cipher_finish(operation, output, output_size, output_length);
    }

    psa_status_t PsaWrappers::cipher_abort(psa_cipher_operation_t *operation)
    {
        return psa_cipher_abort(operation);
    }

    psa_status_t PsaWrappers::destroy_key(psa_key_id_t key_id)
    {
        return psa_destroy_key(key_id);
    }

    psa_status_t PsaWrappers::crypto_init()
    {
        return psa_crypto_init();
    }

    psa_status_t PsaWrappers::import_key(psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id, psa_key_type_t type, size_t bits, size_t key_size)
    {
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

        psa_set_key_usage_flags(&attributes, usage);
        psa_set_key_algorithm(&attributes, algorithm);
        psa_set_key_type(&attributes, type);
        psa_set_key_bits(&attributes, bits);

        return psa_import_key(&attributes, key, key_size, key_id);
    }

    psa_status_t PsaWrappers::cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key_id, psa_algorithm_t algorithm)
    {
        return psa_cipher_encrypt_setup(operation, key_id, algorithm);
    }

    psa_status_t PsaWrappers::cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv, size_t iv_length)
    {
        return psa_cipher_set_iv(operation, iv, iv_length);
    }

    psa_status_t PsaWrappers::cipher_update(psa_cipher_operation_t *operation, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length)
    {
        return psa_cipher_update(operation, input, input_length, output, output_size, output_length);
    }

    sl_status_t PsaWrappers::psa_status_to_sl_status(psa_status_t status)
    {
        switch (status) {
            case PSA_SUCCESS:
                return SL_STATUS_OK;
            case PSA_ERROR_INVALID_SIGNATURE:
                return SL_STATUS_INVALID_SIGNATURE;
            case PSA_ERROR_INVALID_ARGUMENT:
                return SL_STATUS_INVALID_PARAMETER;
            case PSA_ERROR_NOT_SUPPORTED:
                return SL_STATUS_NOT_SUPPORTED;
            case PSA_ERROR_INSUFFICIENT_MEMORY:
                return SL_STATUS_ALLOCATION_FAILED;
            case PSA_ERROR_BAD_STATE:
                return SL_STATUS_INVALID_STATE;
            default:
                return SL_STATUS_FAIL;
        }
    }

}  // namespace crypto
