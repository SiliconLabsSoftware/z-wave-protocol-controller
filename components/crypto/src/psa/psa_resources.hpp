#ifndef PSA_RESOURCES_HPP
#define PSA_RESOURCES_HPP

#include "psa_wrappers.hpp"

namespace crypto
{
    /**
     * @brief Own an imported PSA key and destroy it when leaving scope.
     */
    class ScopedKey
    {
        public:
            /**
             * @brief Create a guard that owns no key yet; psa must outlive the guard.
             */
            explicit ScopedKey(PsaWrappersInterface &psa) : psa_(psa) {}

            /**
             * @brief Destroy the owned key, if any; cleanup errors are ignored.
             */
            ~ScopedKey()
            {
                if (owned_) {
                    psa_.destroy_key(key_id_);
                }
            }

            ScopedKey(const ScopedKey &)            = delete;
            ScopedKey &operator=(const ScopedKey &) = delete;
            ScopedKey(ScopedKey &&)                 = delete;
            ScopedKey &operator=(ScopedKey &&)      = delete;

            /**
             * @brief Import a key and take ownership only on success.
             *
             * @param usage Permitted key operations.
             * @param algorithm Permitted algorithm.
             * @param key Encoded key bytes.
             * @param type PSA key type.
             * @param bits Key size in bits.
             * @return PSA_SUCCESS on success, PSA_ERROR_BAD_STATE if a key is already owned, otherwise a PSA error.
             */
            psa_status_t import_key(psa_key_usage_t usage, psa_algorithm_t algorithm, std::span<const uint8_t> key, psa_key_type_t type, size_t bits)
            {
                if (owned_) {
                    return PSA_ERROR_BAD_STATE;
                }

                psa_key_id_t key_id = PSA_KEY_ID_NULL;
                const auto status   = psa_.import_key(usage, algorithm, key, key_id, type, bits);
                if (status == PSA_SUCCESS) {
                    key_id_ = key_id;
                    owned_  = true;
                }
                return status;
            }

            /**
             * @brief Get the owned key identifier, or PSA_KEY_ID_NULL before a successful import.
             */
            psa_key_id_t id() const noexcept
            {
                return key_id_;
            }

        private:
            PsaWrappersInterface &psa_;
            psa_key_id_t key_id_ = PSA_KEY_ID_NULL;
            bool owned_          = false;
    };

    /**
     * @brief Abort an unfinished PSA cipher operation when leaving scope.
     */
    class ScopedCipherOperation
    {
        public:
            /**
             * @brief Create a guard with no active operation; psa must outlive the guard.
             */
            explicit ScopedCipherOperation(PsaWrappersInterface &psa) : psa_(psa) {}

            /**
             * @brief Abort an active operation; cleanup errors are ignored.
             */
            ~ScopedCipherOperation()
            {
                if (active_) {
                    psa_.cipher_abort(operation_);
                }
            }

            ScopedCipherOperation(const ScopedCipherOperation &)            = delete;
            ScopedCipherOperation &operator=(const ScopedCipherOperation &) = delete;
            ScopedCipherOperation(ScopedCipherOperation &&)                 = delete;
            ScopedCipherOperation &operator=(ScopedCipherOperation &&)      = delete;

            /**
             * @brief Start a guarded multipart encryption operation.
             *
             * @param key_id Imported encryption key identifier.
             * @param algorithm Cipher algorithm.
             * @return PSA_SUCCESS on success, PSA_ERROR_BAD_STATE if already active, otherwise a PSA error.
             */
            psa_status_t encrypt_setup(psa_key_id_t key_id, psa_algorithm_t algorithm)
            {
                if (active_) {
                    return PSA_ERROR_BAD_STATE;
                }

                const auto status = psa_.cipher_encrypt_setup(operation_, key_id, algorithm);
                active_           = status == PSA_SUCCESS;
                return status;
            }

            /**
             * @brief Access the guarded operation for setting its IV or processing data.
             */
            psa_cipher_operation_t &get() noexcept
            {
                return operation_;
            }

            /**
             * @brief Finish encryption. On success, the destructor no longer aborts the operation.
             *
             * @param[out] output Buffer receiving any remaining ciphertext.
             * @param[out] output_length Number of bytes written on success.
             * @return PSA_SUCCESS on success, otherwise a PSA error; on failure the destructor still aborts the operation.
             */
            psa_status_t finish(std::span<uint8_t> output, size_t &output_length)
            {
                const auto status = psa_.cipher_finish(operation_, output, output_length);
                if (status == PSA_SUCCESS) {
                    active_ = false;
                }
                return status;
            }

        private:
            PsaWrappersInterface &psa_;
            psa_cipher_operation_t operation_ = PSA_CIPHER_OPERATION_INIT;
            bool active_                      = false;
    };
}  // namespace crypto

#endif
