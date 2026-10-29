#ifndef PSA_WRAPPERS_HPP
#define PSA_WRAPPERS_HPP

#include <stdint.h>

#include "psa/crypto.h"
#include "sl_status.h"

namespace crypto
{

    class PsaWrappersInterface
    {
        public:
            virtual ~PsaWrappersInterface() = default;

            virtual psa_status_t cipher_encrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length) = 0;
            virtual psa_status_t cipher_decrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length) = 0;
            virtual psa_status_t aead_encrypt(psa_key_id_t key_id,
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
              = 0;
            virtual psa_status_t aead_decrypt(psa_key_id_t key_id,
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
              = 0;
            virtual psa_status_t mac_compute(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *mac, size_t mac_size, size_t *mac_length)                           = 0;
            virtual psa_status_t generate_random(uint8_t *output, size_t output_size)                                                                                                                                = 0;
            virtual psa_status_t export_public_key(psa_key_id_t key_id, uint8_t *data, size_t data_size, size_t *data_length)                                                                                        = 0;
            virtual psa_status_t raw_key_agreement(psa_algorithm_t algorithm, psa_key_id_t private_key, const uint8_t *peer_key, size_t peer_key_length, uint8_t *output, size_t output_size, size_t *output_length) = 0;

            virtual psa_status_t cipher_finish(psa_cipher_operation_t *operation, uint8_t *output, size_t output_size, size_t *output_length)                                              = 0;
            virtual psa_status_t cipher_abort(psa_cipher_operation_t *operation)                                                                                                           = 0;
            virtual psa_status_t destroy_key(psa_key_id_t key_id)                                                                                                                          = 0;
            virtual psa_status_t crypto_init()                                                                                                                                             = 0;
            virtual sl_status_t psa_status_to_sl_status(psa_status_t status)                                                                                                               = 0;
            virtual psa_status_t import_key(psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id, psa_key_type_t type, size_t bits, size_t key_size) = 0;
            virtual psa_status_t cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key_id, psa_algorithm_t algorithm)                                                   = 0;
            virtual psa_status_t cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv, size_t iv_length)                                                                     = 0;
            virtual psa_status_t cipher_update(psa_cipher_operation_t *operation, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length)   = 0;
    };

    class PsaWrappers final : public PsaWrappersInterface
    {
        public:
            psa_status_t cipher_encrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length) override;
            psa_status_t cipher_decrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length) override;
            psa_status_t aead_encrypt(psa_key_id_t key_id,
                                      psa_algorithm_t algorithm,
                                      const uint8_t *nonce,
                                      size_t nonce_length,
                                      const uint8_t *additional_data,
                                      size_t additional_data_length,
                                      const uint8_t *plaintext,
                                      size_t plaintext_length,
                                      uint8_t *ciphertext,
                                      size_t ciphertext_size,
                                      size_t *ciphertext_length) override;
            psa_status_t aead_decrypt(psa_key_id_t key_id,
                                      psa_algorithm_t algorithm,
                                      const uint8_t *nonce,
                                      size_t nonce_length,
                                      const uint8_t *additional_data,
                                      size_t additional_data_length,
                                      const uint8_t *ciphertext,
                                      size_t ciphertext_length,
                                      uint8_t *plaintext,
                                      size_t plaintext_size,
                                      size_t *plaintext_length) override;
            psa_status_t mac_compute(psa_key_id_t key_id, psa_algorithm_t algorithm, const uint8_t *input, size_t input_length, uint8_t *mac, size_t mac_size, size_t *mac_length) override;
            psa_status_t generate_random(uint8_t *output, size_t output_size) override;
            psa_status_t export_public_key(psa_key_id_t key_id, uint8_t *data, size_t data_size, size_t *data_length) override;
            psa_status_t raw_key_agreement(psa_algorithm_t algorithm, psa_key_id_t private_key, const uint8_t *peer_key, size_t peer_key_length, uint8_t *output, size_t output_size, size_t *output_length) override;
            psa_status_t crypto_init() override;
            psa_status_t cipher_finish(psa_cipher_operation_t *operation, uint8_t *output, size_t output_size, size_t *output_length) override;
            psa_status_t cipher_abort(psa_cipher_operation_t *operation) override;
            psa_status_t destroy_key(psa_key_id_t key_id) override;
            sl_status_t psa_status_to_sl_status(psa_status_t status) override;
            psa_status_t import_key(psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *key, psa_key_id_t *key_id, psa_key_type_t type, size_t bits, size_t key_size) override;
            psa_status_t cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key_id, psa_algorithm_t algorithm) override;
            psa_status_t cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv, size_t iv_length) override;
            psa_status_t cipher_update(psa_cipher_operation_t *operation, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length) override;
    };

}  // namespace crypto

#endif
