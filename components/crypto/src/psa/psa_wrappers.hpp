#ifndef PSA_WRAPPERS_HPP
#define PSA_WRAPPERS_HPP

#include <cstdint>
#include <span>

#include "psa/crypto.h"

namespace crypto
{
    /**
     * @brief Span-based interface to PSA Crypto, so tests can replace PSA calls with mocks.
     *
     * Lengths and buffer capacities are in bytes unless stated otherwise. Output buffers and lengths are valid only on success.
     */
    class PsaWrappersInterface
    {
        public:
            virtual ~PsaWrappersInterface() = default;

            /**
             * @brief Initialize the PSA Crypto subsystem before using its operations.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t crypto_init() = 0;

            /**
             * @brief Import key bytes with the requested attributes.
             *
             * @param usage Permitted key operations.
             * @param algorithm Permitted algorithm.
             * @param key Encoded key bytes.
             * @param[out] key_id Imported key identifier; the caller must destroy it.
             * @param type PSA key type.
             * @param bits Key size in bits.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t import_key(psa_key_usage_t usage, psa_algorithm_t algorithm, std::span<const uint8_t> key, psa_key_id_t &key_id, psa_key_type_t type, size_t bits) = 0;

            /**
             * @brief Export the public part of an imported key.
             *
             * @param key_id Key identifier.
             * @param[out] data Buffer receiving the encoded public key.
             * @param[out] data_length Number of bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t export_public_key(psa_key_id_t key_id, std::span<uint8_t> data, size_t &data_length) = 0;

            /**
             * @brief Compute a raw shared secret without key derivation.
             *
             * @param algorithm Key agreement algorithm.
             * @param private_key Local private key identifier.
             * @param peer_key Encoded peer public key.
             * @param[out] output Buffer receiving the shared secret.
             * @param[out] output_length Number of bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t raw_key_agreement(psa_algorithm_t algorithm, psa_key_id_t private_key, std::span<const uint8_t> peer_key, std::span<uint8_t> output, size_t &output_length) = 0;

            /**
             * @brief Destroy an imported key and release its resources.
             *
             * @param key_id Key identifier to destroy.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t destroy_key(psa_key_id_t key_id) = 0;

            /**
             * @brief Encrypt a complete message using a PSA cipher.
             *
             * @param key_id Key identifier.
             * @param algorithm Cipher algorithm.
             * @param input Plaintext.
             * @param[out] output Buffer receiving the generated IV, if the algorithm uses one, followed by the ciphertext.
             * @param[out] output_length Number of bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t cipher_encrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length) = 0;

            /**
             * @brief Decrypt a complete message using a PSA cipher.
             *
             * @param key_id Key identifier.
             * @param algorithm Cipher algorithm.
             * @param input IV, if the algorithm uses one, followed by the ciphertext.
             * @param[out] output Buffer receiving plaintext.
             * @param[out] output_length Number of bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t cipher_decrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length) = 0;

            /**
             * @brief Start multipart encryption on an initialized PSA operation.
             *
             * @param[in,out] operation Operation to configure.
             * @param key_id Encryption key identifier.
             * @param algorithm Cipher algorithm.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t cipher_encrypt_setup(psa_cipher_operation_t &operation, psa_key_id_t key_id, psa_algorithm_t algorithm) = 0;

            /**
             * @brief Set the IV for a configured multipart cipher operation.
             *
             * @param[in,out] operation Configured operation.
             * @param iv Initialization vector bytes.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t cipher_set_iv(psa_cipher_operation_t &operation, std::span<const uint8_t> iv) = 0;

            /**
             * @brief Process input through a multipart cipher operation.
             *
             * @param[in,out] operation Active operation.
             * @param input Input bytes.
             * @param[out] output Buffer receiving processed bytes.
             * @param[out] output_length Number of bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t cipher_update(psa_cipher_operation_t &operation, std::span<const uint8_t> input, std::span<uint8_t> output, size_t &output_length) = 0;

            /**
             * @brief Finish a multipart cipher operation and write any remaining bytes.
             *
             * @param[in,out] operation Active operation.
             * @param[out] output Buffer receiving the final bytes.
             * @param[out] output_length Number of bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t cipher_finish(psa_cipher_operation_t &operation, std::span<uint8_t> output, size_t &output_length) = 0;

            /**
             * @brief Abort a multipart cipher operation and release its resources.
             *
             * @param[in,out] operation Operation to reset.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t cipher_abort(psa_cipher_operation_t &operation) = 0;

            /**
             * @brief Encrypt and authenticate a complete message.
             *
             * @param key_id Key identifier.
             * @param algorithm AEAD algorithm, including the tag length.
             * @param nonce Nonce bytes.
             * @param additional_data Authenticated data excluded from the ciphertext.
             * @param plaintext Plaintext bytes.
             * @param[out] ciphertext Buffer receiving ciphertext followed by the tag.
             * @param[out] ciphertext_length Number of bytes written on success, including the tag.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t aead_encrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> nonce, std::span<const uint8_t> additional_data, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_length) = 0;

            /**
             * @brief Decrypt and authenticate a complete message.
             *
             * @param key_id Key identifier.
             * @param algorithm AEAD algorithm, including the tag length.
             * @param nonce Nonce bytes.
             * @param additional_data Authenticated data excluded from the ciphertext.
             * @param ciphertext Ciphertext followed by the tag.
             * @param[out] plaintext Buffer receiving authenticated plaintext.
             * @param[out] plaintext_length Number of plaintext bytes written on success.
             * @return PSA_SUCCESS on success, PSA_ERROR_INVALID_SIGNATURE if authentication fails, otherwise a PSA error status.
             */
            virtual psa_status_t aead_decrypt(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> nonce, std::span<const uint8_t> additional_data, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_length) = 0;

            /**
             * @brief Compute a message authentication code.
             *
             * @param key_id Authentication key identifier.
             * @param algorithm MAC algorithm.
             * @param input Message bytes.
             * @param[out] mac Buffer receiving the MAC.
             * @param[out] mac_length Number of MAC bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t mac_compute(psa_key_id_t key_id, psa_algorithm_t algorithm, std::span<const uint8_t> input, std::span<uint8_t> mac, size_t &mac_length) = 0;

            /**
             * @brief Fill a buffer with cryptographically secure random bytes.
             *
             * @param[out] output Buffer to fill.
             * @return PSA_SUCCESS on success, otherwise a PSA error status.
             */
            virtual psa_status_t generate_random(std::span<uint8_t> output) = 0;
    };

}  // namespace crypto

#endif
