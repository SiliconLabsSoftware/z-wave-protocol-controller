#include "crypto_impl.hpp"

#include "psa/crypto.h"
#include "psa/util.hpp"
#include "psa/psa_resources.hpp"

namespace crypto
{

    sl_status_t Crypto::aes_ccm_encrypt(std::span<const uint8_t, crypto::aes_key_size> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_len) const
    {
        detail::ScopedKey imported_key(psa_wrappers_);
        const psa_algorithm_t algorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_len);
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = imported_key.import_key(PSA_KEY_USAGE_ENCRYPT, algorithm, key, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.aead_encrypt(imported_key.id(), algorithm, nonce, aad, plaintext, ciphertext, ciphertext_len);
        return psa_status_to_sl_status(status);
    }

    sl_status_t Crypto::aes_ccm_decrypt(std::span<const uint8_t, crypto::aes_key_size> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_len) const
    {
        detail::ScopedKey imported_key(psa_wrappers_);
        const psa_algorithm_t algorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_len);
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = imported_key.import_key(PSA_KEY_USAGE_DECRYPT, algorithm, key, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.aead_decrypt(imported_key.id(), algorithm, nonce, aad, ciphertext, plaintext, plaintext_len);
        return psa_status_to_sl_status(status);
    }

}  // namespace crypto
