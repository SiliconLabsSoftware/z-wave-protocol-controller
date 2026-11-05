#ifndef PSA_RESOURCES_HPP
#define PSA_RESOURCES_HPP

#include "psa_wrappers_interface.hpp"

namespace crypto::detail
{
    class ScopedKey
    {
        public:
            explicit ScopedKey(PsaWrappersInterface &psa) : psa_(psa) {}

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

            psa_key_id_t id() const noexcept
            {
                return key_id_;
            }

        private:
            PsaWrappersInterface &psa_;
            psa_key_id_t key_id_ = PSA_KEY_ID_NULL;
            bool owned_          = false;
    };

    class ScopedCipherOperation
    {
        public:
            explicit ScopedCipherOperation(PsaWrappersInterface &psa) : psa_(psa) {}

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

            psa_status_t encrypt_setup(psa_key_id_t key_id, psa_algorithm_t algorithm)
            {
                if (active_) {
                    return PSA_ERROR_BAD_STATE;
                }

                const auto status = psa_.cipher_encrypt_setup(operation_, key_id, algorithm);
                active_           = status == PSA_SUCCESS;
                return status;
            }

            psa_cipher_operation_t &get() noexcept
            {
                return operation_;
            }

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
}  // namespace crypto::detail

#endif
