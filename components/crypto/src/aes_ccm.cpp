#include "aes_ccm.hpp"

#include "psa/crypto.h"

namespace crypto
{

    sl_status_t Crypto::aes_ccm_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_len) const
    {
        psa_key_id_t key_id;
        const psa_algorithm_t algorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_len);
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_wrappers_.psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_ENCRYPT, algorithm, key.data(), &key_id, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE);
        if (status != PSA_SUCCESS) {
            return psa_wrappers_.psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.aead_encrypt(key_id, algorithm, nonce.data(), nonce.size(), aad.data(), aad.size(), plaintext.data(), plaintext.size(), ciphertext.data(), ciphertext.size(), &ciphertext_len);
        psa_wrappers_.destroy_key(key_id);
        return psa_wrappers_.psa_status_to_sl_status(status);
    }

    sl_status_t Crypto::aes_ccm_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_len) const
    {
        psa_key_id_t key_id;
        const psa_algorithm_t algorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_len);
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_wrappers_.psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_DECRYPT, algorithm, key.data(), &key_id, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE);
        if (status != PSA_SUCCESS) {
            return psa_wrappers_.psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.aead_decrypt(key_id, algorithm, nonce.data(), nonce.size(), aad.data(), aad.size(), ciphertext.data(), ciphertext.size(), plaintext.data(), plaintext.size(), &plaintext_len);
        psa_wrappers_.destroy_key(key_id);
        return psa_wrappers_.psa_status_to_sl_status(status);
    }

    sl_status_t aes_ccm_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_len)
    {
        return Crypto::instance().aes_ccm_encrypt(key, tag_len, nonce, aad, plaintext, ciphertext, ciphertext_len);
    }

    sl_status_t aes_ccm_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_len)
    {
        return Crypto::instance().aes_ccm_decrypt(key, tag_len, nonce, aad, ciphertext, plaintext, plaintext_len);
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_aes_ccm_encrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_len)
{
    return crypto::aes_ccm_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE),
                                   tag_len,
                                   std::span<const uint8_t>(nonce, nonce_len),
                                   std::span<const uint8_t>(aad, aad_len),
                                   std::span<const uint8_t>(plaintext, plaintext_len),
                                   std::span<uint8_t>(ciphertext, ciphertext_size),
                                   *ciphertext_len);
}

sl_status_t crypto_aes_ccm_decrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len)
{
    return crypto::aes_ccm_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE),
                                   tag_len,
                                   std::span<const uint8_t>(nonce, nonce_len),
                                   std::span<const uint8_t>(aad, aad_len),
                                   std::span<const uint8_t>(ciphertext, ciphertext_len),
                                   std::span<uint8_t>(plaintext, plaintext_size),
                                   *plaintext_len);
}

}  // extern "C"
